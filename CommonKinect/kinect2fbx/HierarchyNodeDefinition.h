#pragma once

#include "..\stdafx.h"


// Definte MotionBuilder joints name, which follows convention
#ifndef _MBUILDER_CONVENTION_JOINT_NAME_
#define _MBUILDER_CONVENTION_JOINT_NAME_

#define HIPS_JOINT_NAME  "Hips"

#define LEFT_UP_LEG_JOINT_NAME  "LeftUpLeg"
#define LEFT_LEG_JOINT_NAME  "LeftLeg"
#define LEFT_FOOT_JOINT_NAME  "LeftFoot"
#define LEFT_TOEBASE_JOINT_NAME "LeftToeBase"


#define RIGHT_UP_LEG_JOINT_NAME  "RightUpLeg"
#define RIGHT_LEG_JOINT_NAME  "RightLeg"
#define RIGHT_FOOT_JOINT_NAME  "RightFoot"
#define RIGHT_TOEBASE_JOINT_NAME "RightToeBase"

#define SPINE_JOINT_NAME  "Spine"
#define SPINE_ONE_JOINT_NAME "Spine1"

#define LEFT_SHOULDER_JOINT_NAME "LeftShoulder"
#define LEFT_ARM_JOINT_NAME  "LeftArm"
#define LEFT_FORE_ARM_JOINT_NAME  "LeftForeArm"
#define LEFT_HAND_JOINT_NAME  "LeftHand"

#define RIGHT_SHOULDER_JOINT_NAME "RightShoulder"
#define RIGHT_ARM_JOINT_NAME  "RightArm"
#define RIGHT_FORE_ARM_JOINT_NAME  "RightForeArm"
#define RIGHT_HAND_JOINT_NAME  "RightHand"

#define NECK_JOINT_NAME  "Neck"
#define HEAD_JOINT_NAME  "Head"

#endif


/*
	Generic hierarchy of body joints
*/
struct HierarchyNodeDefinition {

	/// <summary>
	/// Constructor
	/// </summary>
	HierarchyNodeDefinition(const char *name = NULL, const JointType twinType = JointType_Count);

	// Name of current node
	FbxString m_fNodeName;

	// Joint translation information (not related to animation)
	FbxDouble3 m_translation;

	// Joint rotation information (not related to animation)
	FbxDouble3 m_rotation;

	// Kinect Corresponding joint
	JointType m_kTwin;

	// Children Nodes
	std::vector<std::shared_ptr<HierarchyNodeDefinition>> m_children;
};

/// <summary>
/// Returns a node hierarchy that follows the MotionBuilder naming convention
/// </summary>
/// <returns>Returns a reference to the default hierarchy</returns>
std::shared_ptr<HierarchyNodeDefinition>     GetDefaultHierarchyNodeDefinition();


