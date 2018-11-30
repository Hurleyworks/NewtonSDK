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
#include "dAnimationEffectorBlendTwoWay.h"

dAnimationEffectorBlendTwoWay::dAnimationEffectorBlendTwoWay(dAnimationCharacterRig* const character,
	dAnimationEffectorBlendNode* const node0,
	dAnimationEffectorBlendNode* const node1)
	:dAnimationEffectorBlendNode(character)
	,m_node0(node0)
	,m_node1(node1)
	,m_pose(character)
	,m_param(0.0f)
{
	m_param = 0.5f;
}

dAnimationEffectorBlendTwoWay::~dAnimationEffectorBlendTwoWay()
{
	delete m_node0;
	delete m_node1;
}

void dAnimationEffectorBlendTwoWay::Evaluate(dAnimationPose& output, dFloat timestep)
{
	m_node0->Evaluate(output, timestep);
	m_node1->Evaluate(m_pose, timestep);

	dAnimationPose::dListNode* destNode = output.GetFirst();
	for (dAnimationPose::dListNode* sourceNode = m_pose.GetFirst(); sourceNode; sourceNode = sourceNode->GetNext()) {
		const dAnimationTransform& srcFrame = sourceNode->GetInfo();
		dAnimationTransform& dstFrame = destNode->GetInfo();
		dAssert(srcFrame.m_effector == dstFrame.m_effector);
		dstFrame.m_rotation = dstFrame.m_rotation.Slerp(srcFrame.m_rotation, m_param);
		dstFrame.m_posit = dstFrame.m_posit + (srcFrame.m_posit - dstFrame.m_posit).Scale(m_param);
		destNode = destNode->GetNext();
	}
}