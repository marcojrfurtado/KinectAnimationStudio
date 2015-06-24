#pragma once

#include "..\stdafx.h"
#include "HierarchyNodeDefinition.h"

/*
 Static class responsible for mapping Kinect skeleton frames to FBX scene
*/
class KinectSkeletonMapper {
public:
	/// <summary>
	/// Map current frame of Kinect Body to FBX scene
	/// </summary>
	/// <param name="pScene">FBX scene</param>
	/// <param name="frameTime">Current frame time</param>
	/// <param name="kBody">Kinect body to be mapped</param>
	static void map(FbxScene* pScene, INT64 frameTime, IBody *kBody);


	/// <summary>
	/// Applies post processing filters to our motion data
	/// </summary>
	/// <param name="pScene">FBX  scene</param>
	static void applyPostProcessingFilters(FbxScene*  pScene);
private:

	// Constants:
	// Define how skeletons should be identified in the FBX scene
	static const char *c_SkelRootIdPatternPreffix;
	// Define node hierarchy
	static const std::shared_ptr<HierarchyNodeDefinition> c_defaultNodeHierarchy;
	// Default name used by us to store Kinect's Joint type in FBX file
	static const char *c_jointTypePropertyDefaultName;
	// Default Kinect root skeleton joint
	static const JointType c_kinectRootJointType;
	// If rotation difference is bigger than the following constant, we consider the rotation to be non-continuous
	static const float c_rotationContinuityMaxOffset;

	// We use this to scale the translation of the root joint when mapping
	static const float  c_positionalScalingFactor;

	// Private methods

	/// <summary>
	/// Initialize body , by adding its corresponding skeleton to the FBX scene
	/// </summary>
	/// <param name="pScene">FBX scene</param>
	/// <param name="kBody">Kinect body to be initialized</param>
	/// <returns>Pointer to skeleton root node</returns>
	static FbxNode * init(FbxScene*  pScene, IBody *kBody);

	/// <summary>
	/// A new frame has been received, add animation corresponding to the motion
	/// </summary>
	/// <param name="pScene">FBX scene</param>
	/// <param name="frameTime">Current frame time</param>
	/// <param name="kBody">Kinect body to have animation extracted</param>
	static void addAnimationKeys(FbxScene*  pScene, FbxNode *rootNode, INT64 frameTime, IBody *kBody);

	/// <summary>
	/// A new frame has been received, add animation corresponding to the motion
	/// </summary>
	/// <param name="pScene">FBX scene</param>
	/// <param name="rootNode">FBX root node</param>
	/// <param name="frameTime">Current frame time</param>
	/// <param name="joints">Kinect joint position array</param>
	/// <param name="orientations">Kinect joint orientation array</param>
	static void addAnimationKeys(FbxScene*  pScene, FbxNode *rootNode, INT64 frameTime, Joint *joints, JointOrientation *orientations);

	/// <summary>
	/// Recursive function that creates a FBX node hierarchy on the scene, based on a given definitons
	/// </summary>
	/// <param name="pScene">Current FBX scene</param>
	/// <param name="kBody">Kinect Body</param>
	/// <param name="fNode">Current FBX node</param>
	/// <param name="hNode">Hierarchical definition</param>
	static void createHierarchy(FbxScene *pScene, IBody *kBody, FbxNode *fNode, const std::shared_ptr<HierarchyNodeDefinition> &hNode);


	/// <summary>
	/// Gets pre-fixed name of a node
	/// </summary>
	/// <param name="inKBody">Kinect body for which the preffix will be retrieved</param>
	/// <param name="nodeName">Node name without preffix</param>
	/// <param name="outName">Variable where name will be written to</param>
	/// <return>VName with prefix</return>
	static FbxString getPreffixedNodeName(IBody *inKBody, const FbxString nodeName);


	/// <summary>
	/// Sets joint type property, based on Kinect's JointType
	/// </summary>
	/// <param name="fNode">Node to have type set</param>
	/// <param name="kType">Kinect joint type</param>
	static void setJointTypeProperty(FbxNode *fNode, JointType kType);

	/// <summary>
	/// Gets joint type property, based on Kinect's JointType
	/// </summary>
	/// <param name="fNode">Node to get the type</param>
	static JointType getJointTypeProperty(FbxNode *fNode);

	/// <summary>
	/// Recursively add animation keys to each node in the hierarchy
	/// </summary>
	/// <param name="pLayer">FBX  animation layer</param>
	/// <param name="fNode">FBX node to be animated</param>
	/// <param name="kJoints">Kinect joints to bild animation</param>
	/// <param name="kJointOrientations">Kinect joint orientations to build animation</param>
	/// <param name="accumulator">Auxiliary matrix that helps converting from absolute orientation to relative. Defaults to identity</param>
	static void animateHierarchy(FbxAnimLayer*  pScene, FbxNode *fNode, INT64 frameTime, Joint *kJoints, JointOrientation *kOrientations, FbxAMatrix accumullator = getIdentityMat());


	/// <summary>
	/// Extracts translation information from kinect, and add it as a key to our animation layer
	/// </summary>
	/// <param name="pLayer">FBX  animation layer</param>
	/// <param name="fNode">FBX node to be animated</param>
	/// <param name="frameTime">Current frame time</param>
	/// <param name="kJoint">Kinect joint to have position extracted from</param>
	static void addTranslationKeys(FbxAnimLayer*  pLayer, FbxNode *fNode, INT64 frameTime, Joint &kJoint);

	/// <summary>
	/// Extracts rotation information from kinect, and add it as a key to our animation layer
	/// </summary>
	/// <param name="pLayer">FBX  animation layer</param>
	/// <param name="fNode">FBX node to be animated</param>
	/// <param name="frameTime">Current frame time</param>
	/// <param name="kOrientation">Kinect joint orientation</param>
	/// <param name="accumullator">Used to compute orientations based on parent joint</param>
	static void addRotationKeys(FbxAnimLayer*  pLayer, FbxNode *fNode, INT64 frameTime, JointOrientation &kOrientation, FbxAMatrix &accumullator);

	/// <summary>
	/// Finds euler rotation for key with certain index
	/// </summary>
	/// <param name="keyIndex">Index of the desired key</param>
	/// <param name="rotationCurveX">Rotation curve X</param>
	/// <param name="rotationCurveY">Rotation curve Y</param>
	/// <param name="rotationCurveZ">Rotation curve Z</param>
	/// <return>Rotation in euler angles</return>
	static FbxVector4 getEulerRotation(int keyIndex, FbxAnimCurve *rotationCurveX, FbxAnimCurve *rotationCurveY, FbxAnimCurve *rotationCurveZ);

	/// <summary>
	/// Make sure rotation in Euler angles is continuous
	/// </summary>
	/// <param name="previousEuler">Rotation for the previous frame</param>
	/// <param name="currentEuler">Rotaion for the current frame</param>
	/// <return>Continuous rotation, in euler angles</return>
	static FbxVector4 makeRotationContinuous(FbxVector4 previousEuler, FbxVector4 currentEuler);


	/// <summary>
	/// Sets some initial alignment rules, based on assumptions about the sensor
	/// </summary>
	/// <param name="fNode">Root FBX  node</param>
	/// <param name="kJoints">Kinect joint position info</param>
	/// <param name="kOrientations">Kinect joint orientation info</param>
	static void KinectSkeletonMapper::setInitialAlignmentRules(FbxNode *fNode, Joint *kJoints, JointOrientation *kOrientations);

	/// <summary>
	/// Get Identity Mat
	/// </summary>
	/// <return>Identity matrix</return>
	static FbxAMatrix getIdentityMat() {
		FbxAMatrix ident;
		ident.SetIdentity();
		return ident;
	}

	/// <summary>
	/// Checks whether orientation is null
	/// </summary>
	static inline bool isOrientationNull(JointOrientation ori) {
		return ori.Orientation.x == 0.0 &&
			ori.Orientation.y == 0.0 &&
			ori.Orientation.z == 0.0 &&
			ori.Orientation.w == 0.0;
	}

};