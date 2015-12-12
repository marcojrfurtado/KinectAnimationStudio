#include "HierarchyNodeDefinition.h"


/// <summary>
/// Constructor
/// </summary>
HierarchyNodeDefinition::HierarchyNodeDefinition(const char *name, const JointType type ) {
	if (name)
		m_fNodeName = FbxString(name);

	// Define joint type
	m_kTwin = type;
}

/// <summary>
/// Returns a node hierarchy that follows the MotionBuilder naming convention
/// </summary>
/// <returns>Returns a reference to the default hierarchy</returns>
std::shared_ptr<HierarchyNodeDefinition>     GetDefaultHierarchyNodeDefinition() {
	// Create node list
	std::shared_ptr<HierarchyNodeDefinition> Hips(new HierarchyNodeDefinition(HIPS_JOINT_NAME, JointType_SpineBase)),
		LeftUpLeg(new HierarchyNodeDefinition(LEFT_UP_LEG_JOINT_NAME, JointType_HipLeft)),
		LeftLeg(new HierarchyNodeDefinition(LEFT_LEG_JOINT_NAME, JointType_KneeLeft)),
		LeftFoot(new HierarchyNodeDefinition(LEFT_FOOT_JOINT_NAME, JointType_AnkleLeft)),
		LeftToeBase(new HierarchyNodeDefinition(LEFT_TOEBASE_JOINT_NAME, JointType_FootLeft)),
		RightUpLeg(new HierarchyNodeDefinition(RIGHT_UP_LEG_JOINT_NAME, JointType_HipRight)),
		RightLeg(new HierarchyNodeDefinition(RIGHT_LEG_JOINT_NAME, JointType_KneeRight)),
		RightFoot(new HierarchyNodeDefinition(RIGHT_FOOT_JOINT_NAME, JointType_AnkleRight)),
		RightToeBase(new HierarchyNodeDefinition(RIGHT_TOEBASE_JOINT_NAME, JointType_FootRight)),
		Spine(new HierarchyNodeDefinition(SPINE_JOINT_NAME, JointType_SpineMid)),
		//Spine1(new HierarchyNodeDefinition(SPINE_ONE_JOINT_NAME)),
		LeftShoulder(new HierarchyNodeDefinition(LEFT_SHOULDER_JOINT_NAME, JointType_ShoulderLeft)),
		LeftArm(new HierarchyNodeDefinition(LEFT_ARM_JOINT_NAME, JointType_ShoulderLeft)),
		LeftForeArm(new HierarchyNodeDefinition(LEFT_FORE_ARM_JOINT_NAME,JointType_ElbowLeft)),
		LeftHand(new HierarchyNodeDefinition(LEFT_HAND_JOINT_NAME, JointType_WristLeft)),
		RightShoulder(new HierarchyNodeDefinition(RIGHT_SHOULDER_JOINT_NAME, JointType_ShoulderRight)),
		RightArm(new HierarchyNodeDefinition(RIGHT_ARM_JOINT_NAME, JointType_ShoulderRight)),
		RightForeArm(new HierarchyNodeDefinition(RIGHT_FORE_ARM_JOINT_NAME, JointType_ElbowRight)),
		RightHand(new HierarchyNodeDefinition(RIGHT_HAND_JOINT_NAME, JointType_WristRight)),
		Neck(new HierarchyNodeDefinition(NECK_JOINT_NAME, JointType_Neck)),
		Head(new HierarchyNodeDefinition(HEAD_JOINT_NAME, JointType_Head));

	// Define their relationship
	Hips->m_children = { LeftUpLeg, RightUpLeg, Spine };
	
	LeftUpLeg->m_children = { LeftLeg };
	LeftLeg->m_children = { LeftFoot };
	LeftFoot->m_children = { LeftToeBase };


	RightUpLeg->m_children = { RightLeg };
	RightLeg->m_children = { RightFoot };
	RightFoot->m_children = { RightToeBase };


	//Spine->m_children = { Spine1 };
	Spine->m_children = { Neck, RightShoulder, LeftShoulder };
	Neck->m_children = { Head };

	RightShoulder->m_children = { RightArm };
	RightArm->m_children = { RightForeArm };
	RightForeArm->m_children = { RightHand };


	LeftShoulder->m_children = { LeftArm };
	LeftArm->m_children = { LeftForeArm };
	LeftForeArm->m_children = { LeftHand };

	/////// Begin setting initial offset for each joint. 
	// This should remain static, independently of the user
	Hips->m_translation[1] = 90.23;

	LeftUpLeg->m_translation[0] = 10.89;
	LeftUpLeg->m_rotation[2] = 180;

	LeftLeg->m_translation[1] = 44.03;

	LeftFoot->m_translation[1] = 41.45;
	LeftFoot->m_rotation[0] = 90;

	LeftToeBase->m_translation = { 0.0, 5.18, 0.0 };


	RightUpLeg->m_translation[0] = -10.89;
	RightUpLeg->m_rotation[2] = 180;

	RightLeg->m_translation[1] = 44.03;

	RightFoot->m_translation[1] = 41.45;
	RightFoot->m_rotation[0] = 90;

	RightToeBase->m_translation = { 0.0, 5.18, 0 };

	Spine->m_translation = { 0.0, 18.77, 0.0 };


	Neck->m_translation = { 0.0, 24.33, 0.0 };
	Neck->m_rotation[1] = 180;

	Head->m_translation = { 0.0, 12.56, 0.0 };

	LeftShoulder->m_translation = { 20.0, 24.33, 0.0 };
	LeftShoulder->m_rotation[2] = -90;

	LeftForeArm->m_translation[1] = 25.0;

	LeftHand->m_translation[1] = 25.0;

	RightShoulder->m_translation = { -20.0, 24.33, 0.0 };
	RightShoulder->m_rotation[2] = 90;

	RightForeArm->m_translation[1] = 25.0;

	RightHand->m_translation[1] = 25.0;

	/////// Finish setting initial offset for each joint. 


	// Hips joint is our root, return it
	return Hips;
}