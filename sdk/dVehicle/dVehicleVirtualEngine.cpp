/* Copyright (c) <2003-2016> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

#include "dStdafxVehicle.h"
#include "dVehicleChassis.h"
#include "dVehicleSingleBody.h"
#include "dVehicleVirtualEngine.h"

#define D_ENGINE_IDLE_GAS_VALUE dFloat (0.25f)

dVehicleVirtualEngine::dEngineMetricInfo::dEngineMetricInfo(const dEngineInfo& info)
	:dEngineInfo(info)
{
	const dFloat horsePowerToWatts = 735.5f;
	const dFloat rpmToRadiansPerSecunds = 0.105f;
	const dFloat poundFootToNewtonMeters = 1.356f;

	m_clutchTorque *= poundFootToNewtonMeters;
	m_idleTorque *= poundFootToNewtonMeters;
	m_peakTorque *= poundFootToNewtonMeters;

	m_rpmAtPeakTorque *= rpmToRadiansPerSecunds;
	m_rpmAtPeakHorsePower *= rpmToRadiansPerSecunds;
	m_rpmAtRedLine *= rpmToRadiansPerSecunds;
	m_rpmAtIdleTorque *= rpmToRadiansPerSecunds;

	m_peakHorsePower *= horsePowerToWatts;
	m_peakPowerTorque = m_peakHorsePower / m_rpmAtPeakHorsePower;

	dAssert(m_rpmAtIdleTorque > 0.0f);
	dAssert(m_rpmAtIdleTorque < m_rpmAtPeakHorsePower);
	dAssert(m_rpmAtPeakTorque < m_rpmAtPeakHorsePower);
	dAssert(m_rpmAtPeakHorsePower < m_rpmAtRedLine);

	dAssert(m_idleTorque > 0.0f);
	dAssert(m_peakTorque > m_peakPowerTorque);
	dAssert((m_peakTorque * m_rpmAtPeakTorque) < m_peakHorsePower);
}

dFloat dVehicleVirtualEngine::dEngineMetricInfo::GetTorque (dFloat rpm) const
{
	dAssert(rpm >= -0.1f);
	const int maxIndex = sizeof (m_torqueCurve) / sizeof (m_torqueCurve[0]);
	rpm = dClamp(rpm, dFloat(0.0f), m_torqueCurve[maxIndex - 1].m_rpm);

	for (int i = 1; i < maxIndex; i++) {
		if (rpm <= m_torqueCurve[i].m_rpm) {
			dFloat rpm1 = m_torqueCurve[i].m_rpm;
			dFloat rpm0 = m_torqueCurve[i - 1].m_rpm;

			dFloat torque1 = m_torqueCurve[i].m_torque;
			dFloat torque0 = m_torqueCurve[i - 1].m_torque;

			dFloat torque = torque0 + (rpm - rpm0) * (torque1 - torque0) / (rpm1 - rpm0);
			return torque;
		}
	}

	return m_torqueCurve[maxIndex - 1].m_torque;
}

dVehicleVirtualEngine::dVehicleVirtualEngine(dVehicleNode* const parent, const dEngineInfo& info, dVehicleDifferentialInterface* const differential)
	:dVehicleEngineInterface(parent, info, differential)
	,m_metricInfo(info)
	,m_blockJoint()
	,m_crankJoint()
	,m_gearBox()
	,m_omega(0.0f)
	,m_torque(0.0f)
	,m_viscousDragMaxForce(0.0f)
	,m_viscousDragCoefficient(0.0f)
{
	SetWorld(parent->GetWorld());

	dFloat inertia = 0.7f * m_info.m_mass * m_info.m_armatureRadius * m_info.m_armatureRadius;
	m_body.SetMass(m_info.m_mass);
	m_body.SetInertia(inertia, inertia, inertia);
	m_body.UpdateInertia();

	dVehicleSingleBody* const chassis = (dVehicleSingleBody*) ((dVehicleNode*)m_parent)->GetAsVehicle();
	// set the tire joint
	m_blockJoint.Init(&m_body, chassis->GetBody());

	m_gearBox.Init(&m_body, differential->GetBody());
	m_gearBox.SetOwners(this, differential);

	m_crankJoint.Init(&m_body, chassis->m_groundNode.GetBody());
	m_crankJoint.SetOwners(this, &chassis->m_groundNode);

	SetInfo(info);
}

dVehicleVirtualEngine::~dVehicleVirtualEngine()
{
}

void dVehicleVirtualEngine::InitEngineTorqueCurve()
{
	m_metricInfo = dEngineMetricInfo(m_info);

	m_metricInfo.m_torqueCurve[0] = dEngineTorqueNode(0.0f, m_metricInfo.m_idleTorque);
	m_metricInfo.m_torqueCurve[1] = dEngineTorqueNode(m_metricInfo.m_rpmAtIdleTorque, m_metricInfo.m_idleTorque);
	m_metricInfo.m_torqueCurve[2] = dEngineTorqueNode(m_metricInfo.m_rpmAtPeakTorque, m_metricInfo.m_peakTorque);
	m_metricInfo.m_torqueCurve[3] = dEngineTorqueNode(m_metricInfo.m_rpmAtPeakHorsePower, m_metricInfo.m_peakPowerTorque);
	m_metricInfo.m_torqueCurve[4] = dEngineTorqueNode(m_metricInfo.m_rpmAtRedLine, m_metricInfo.m_idleTorque);
}

void dVehicleVirtualEngine::SetInfo(const dEngineInfo& info)
{
	dVehicleEngineInterface::SetInfo(info);
	InitEngineTorqueCurve();

	//calculate viscous drag Coefficient
	dFloat omega = m_metricInfo.m_rpmAtIdleTorque;
	m_viscousDragMaxForce = D_ENGINE_IDLE_GAS_VALUE * m_metricInfo.GetTorque(0.0f);
	m_viscousDragCoefficient = dSqrt(m_viscousDragMaxForce / omega);

//	m_controller->SetAerodynamicsDownforceCoefficient(info.m_aerodynamicDownforceFactor, info.m_aerodynamicDownForceSurfaceCoeficident, info.m_aerodynamicDownforceFactorAtTopSpeed);
}

dComplementaritySolver::dBilateralJoint* dVehicleVirtualEngine::GetJoint()
{
	return &m_blockJoint;
}

void dVehicleVirtualEngine::Debug(dCustomJoint::dDebugDisplay* const debugContext) const
{
	dVehicleEngineInterface::Debug(debugContext);
}

dFloat dVehicleVirtualEngine::GetRpm() const
{
	return m_omega * 9.549f;
}

dFloat dVehicleVirtualEngine::GetRedLineRpm() const
{
	return m_metricInfo.m_rpmAtRedLine * 9.549f;
}

void dVehicleVirtualEngine::SetThrottle (dFloat throttle)
{
	throttle = dClamp(throttle, D_ENGINE_IDLE_GAS_VALUE, dFloat(1.0f));
	m_torque = m_metricInfo.GetTorque(dAbs(m_omega)) * throttle;
}

void dVehicleVirtualEngine::SetGear (int gear)
{
	gear = dClamp (gear, int (m_reverseGear), m_metricInfo.m_gearsCount);
	dFloat ratio = m_metricInfo.m_gearRatios[gear];
	m_gearBox.SetGearRatio(ratio);
}

void dVehicleVirtualEngine::SetClutch (dFloat clutch) 
{
	clutch = dClamp (clutch, dFloat (0.0f), dFloat (1.0f));
	m_gearBox.SetClutchTorque(clutch * m_metricInfo.m_clutchTorque);
}

int dVehicleVirtualEngine::GetKinematicLoops(dAnimationKinematicLoopJoint** const jointArray)
{
	jointArray[0] = &m_crankJoint;
	jointArray[1] = &m_gearBox;

int count = 0;
	return dVehicleEngineInterface::GetKinematicLoops(&jointArray[count]) + count;
}

void dVehicleVirtualEngine::ApplyExternalForce(dFloat timestep)
{
	dVehicleSingleBody* const chassisNode = (dVehicleSingleBody*)m_parent;
	dComplementaritySolver::dBodyState* const chassisBody = chassisNode->GetBody();

	dMatrix matrix(chassisBody->GetMatrix());
	matrix.m_posit = matrix.TransformVector(chassisBody->GetCOM());
	m_body.SetMatrix(matrix);

	m_body.SetVeloc(chassisBody->GetVelocity());
	m_body.SetOmega(chassisBody->GetOmega() + matrix.m_right.Scale (m_omega));
	m_body.SetForce(chassisNode->m_gravity.Scale(m_body.GetMass()));

	// calculate engine torque
	dFloat drag = dMin(m_viscousDragCoefficient * m_omega * m_omega, m_viscousDragMaxForce);
	dVector torque(matrix.m_right.Scale(m_torque - drag));
	m_body.SetTorque(torque);

	dVehicleEngineInterface::ApplyExternalForce(timestep);
}

void dVehicleVirtualEngine::Integrate(dFloat timestep)
{
	dVehicleEngineInterface::Integrate(timestep);

	dVehicleSingleBody* const chassis = (dVehicleSingleBody*)m_parent;
	dComplementaritySolver::dBodyState* const chassisBody = chassis->GetBody();

	const dMatrix chassisMatrix(chassisBody->GetMatrix());

	dVector omega(m_body.GetOmega());
	dVector chassisOmega(chassisBody->GetOmega());
	dVector localOmega(omega - chassisOmega);
	m_omega = chassisMatrix.m_right.DotProduct3(localOmega);

dTrace (("eng(%f)\n", m_omega));
}
