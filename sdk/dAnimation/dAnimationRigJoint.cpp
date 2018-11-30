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

#include "dAnimationStdAfx.h"
#include "dAnimationRigJoint.h"
#include "dAnimationCharacterRigManager.h"

dAnimationRigJoint::dAnimationRigJoint(dAnimationRigJoint* const parent)
	:dAnimationAcyclicJoint(parent)
	,m_root(parent ? parent->GetRoot() : NULL)
{
}

dAnimationRigJoint::~dAnimationRigJoint()
{
}


void dAnimationRigJoint::UpdateLocalTransforms (dAnimationCharacterRigManager* const manager) const
{
	dMatrix parentMatrixPool[128];
	const dAnimationRigJoint* stackPool[128];
	
	int stack = 1;
	stackPool[0] = this;
	parentMatrixPool[0] = dGetIdentityMatrix();

	while (stack) {
		dMatrix matrix;
		stack--;

		dMatrix parentMatrix(parentMatrixPool[stack]);
		const dAnimationRigJoint* const node = stackPool[stack];
		NewtonBody* const newtonBody = node->GetNewtonBody();

		if (newtonBody) {
			NewtonBodyGetMatrix(newtonBody, &matrix[0][0]);
			//manager->OnUpdateTransform(node, matrix * parentMatrix * bone.m_bindMatrix);
			manager->OnUpdateTransform(node, matrix * parentMatrix);

			parentMatrix = matrix.Inverse();
			for (dList<dAnimationAcyclicJoint*>::dListNode* child = node->m_children.GetFirst(); child; child = child->GetNext()) {
				stackPool[stack] = (dAnimationRigJoint*) child->GetInfo();
				parentMatrixPool[stack] = parentMatrix;
				stack++;
			}
		}
	}
}

void dAnimationRigJoint::Init(NewtonBody* const newtonBody)
{
	dComplementaritySolver::dBodyState* const body = dAnimationAcyclicJoint::GetBody();

	dAssert(body);
	dAssert(newtonBody == GetNewtonBody());

	dMatrix matrix(dGetIdentityMatrix());
	NewtonBodyGetCentreOfMass(newtonBody, &matrix.m_posit[0]);
	matrix.m_posit.m_w = 1.0f;
	body->SetLocalMatrix(matrix);

	// get data from engine rigid body and copied to the vehicle chassis body
	NewtonBodyGetMatrix(newtonBody, &matrix[0][0]);
	body->SetMatrix(matrix);

	dFloat mass;
	dFloat Ixx;
	dFloat Iyy;
	dFloat Izz;
	NewtonBodyGetMass(newtonBody, &mass, &Ixx, &Iyy, &Izz);
	body->SetMass(mass);
	body->SetInertia(Ixx, Iyy, Izz);
	body->UpdateInertia();
}

void dAnimationRigJoint::RigidBodyToStates()
{
	NewtonBody* const newtonBody = GetNewtonBody();
	dComplementaritySolver::dBodyState* const body = dAnimationAcyclicJoint::GetBody();

	dAssert (body);
	dAssert (newtonBody);

	dVector vector;
	dMatrix matrix;

	// get data from engine rigid body and copied to the vehicle chassis body
	NewtonBodyGetMatrix(newtonBody, &matrix[0][0]);
	body->SetMatrix(matrix);

	NewtonBodyGetVelocity(newtonBody, &vector[0]);
	body->SetVeloc(vector);

	NewtonBodyGetOmega(newtonBody, &vector[0]);
	body->SetOmega(vector);

	body->SetForce(dVector (0.0f));
	body->SetTorque(dVector (0.0f));

	body->UpdateInertia();

	for (dList<dAnimationAcyclicJoint*>::dListNode* child = m_children.GetFirst(); child; child = child->GetNext()) {
		dAnimationRigJoint* const rigJoint = child->GetInfo()->GetAsRigJoint();
		dAssert (rigJoint);
		rigJoint->RigidBodyToStates();
	}
}

