#include "KinectSkeletonMapper.h"
#include "..\helpers\FBX_helpers.h"

// Constant definitions
const char * KinectSkeletonMapper::c_SkelRootIdPatternPreffix = "Skel%d:";
const char *KinectSkeletonMapper::c_jointTypePropertyDefaultName = "JointType";
const std::shared_ptr<HierarchyNodeDefinition> KinectSkeletonMapper::c_defaultNodeHierarchy = GetDefaultHierarchyNodeDefinition();
const JointType KinectSkeletonMapper::c_kinectRootJointType = JointType_SpineBase;
const float KinectSkeletonMapper::c_rotationContinuityMaxOffset = 180;
const float KinectSkeletonMapper::c_positionalScalingFactor = 60;
const char *KinectSkeletonMapper::c_DefaultRootJointName = "Reference";


/// <summary>
/// Map current frame of Kinect Body to FBX scene
/// </summary>
/// <param name="pScene">FBX Scene</param>
/// <param name="frameTime">Current frame time</param>
/// <param name="kBody">Kinect Body</param>
void KinectSkeletonMapper::map(FbxScene* pScene, INT64 frameTime ,IBody *kBody) {

	FbxString bodyRootName = getPreffixedNodeName(kBody, c_DefaultRootJointName);

	FbxNode *skelNode;
	// Check whether skeleton has already been added to the scene
	skelNode = pScene->FindNodeByName(bodyRootName);

	// Get Kinect Joint Information
	Joint joints[JointType_Count];
	JointOrientation orientations[JointType_Count];

	HRESULT hr;
	hr = kBody->GetJoints(_countof(joints), joints);
	// If we successfully retrieved the Joint array
	if (!SUCCEEDED(hr)) {
		return;
	}

	hr = kBody->GetJointOrientations(_countof(orientations), orientations);
	// If we successfully retrieved the JointOrientation array
	if (!SUCCEEDED(hr)) {
		return;
	}


	if (!skelNode) {
		// Initialize body
		skelNode = init(pScene, kBody);

		// Set translation scale value
		//setTransScaling(skelNode, joints);

		// Set body initial alignment
		setInitialAlignmentRules(skelNode, joints, orientations);
	}

	// Add key information to curves
	addAnimationKeys(pScene, skelNode, frameTime, joints, orientations);

};

/// <summary>
/// Initialize body , by adding its corresponding skeleton to the FBX scene
/// </summary>
/// <param name="pScene">FBX scene</param>
/// <param name="kBody">Kinect body to be initialized</param>
/// <returns>Pointer to skeleton root node</returns>
FbxNode * KinectSkeletonMapper::init(FbxScene*  pScene, IBody *kBody) {


	FbxString bodyRootName = getPreffixedNodeName(kBody, c_DefaultRootJointName);

	// Create skeleton root Node
	FbxSkeleton* lSkeletonRootAttribute = FbxSkeleton::Create(pScene, bodyRootName);
	lSkeletonRootAttribute->SetSkeletonType(FbxSkeleton::eRoot);
	FbxNode* lSkeletonRoot = FbxNode::Create(pScene, bodyRootName);
	lSkeletonRoot->SetNodeAttribute(lSkeletonRootAttribute);

	// Add skeleton root Node to scene
	pScene->GetRootNode()->AddChild(lSkeletonRoot);

	// Create Joint Hierarchy
	createHierarchy(pScene, kBody, lSkeletonRoot, c_defaultNodeHierarchy);

	// Keyframes for T-pose at time 0
	keyInCurrentOrientation(pScene, lSkeletonRoot);

	return lSkeletonRoot;
}


/// <summary>
/// Add keys for orientation at time t
/// </summary>
/// <param name="pScene">Fbx Scene</param>
/// <param name="pNode">Fbx root Node</param>
void KinectSkeletonMapper::keyInCurrentOrientation(FbxScene *pScene, FbxNode *pNode, FbxTime time) {

	FbxAnimStack *baseAnimStack = pScene->GetCurrentAnimationStack();

	// Anim Stack invalid
	if (!baseAnimStack)
		return;

	FbxAnimLayer *baseAnimLayer = baseAnimStack->GetMember<FbxAnimLayer>();


	// Retrieve node joint type
	JointType nodeJointType = getJointTypeProperty(pNode);

	// Translation only for the root joint
	if (nodeJointType == c_kinectRootJointType) {
		FbxDouble3 trans = pNode->LclTranslation.Get();
		applyTransformation(baseAnimLayer, pNode, trans, time, true);
	}

	FbxDouble3 rot = pNode->LclRotation.Get();
	applyTransformation(baseAnimLayer, pNode, rot, time, false);

	int childCount = pNode->GetChildCount();
	// Repeat for children
	for (int i = 0; i < childCount; i++) {
		FbxNode *childNode = pNode->GetChild(i);
		keyInCurrentOrientation(pScene, childNode, time);
	}
}


/// <summary>
/// Recursive function that creates a FBX node hierarchy on the scene, based on a given definitons
/// </summary>
/// <param name="pScene">Current FBX scene</param>
/// <param name="kBody">Kinect Body</param>
/// <param name="fNode">Current FBX node</param>
/// <param name="hNode">Hierarchical definition</param>
void KinectSkeletonMapper::createHierarchy(FbxScene *pScene, IBody *kBody ,FbxNode *fNode, const std::shared_ptr<HierarchyNodeDefinition> &hNode) {

	// Create new limb attribute
	FbxString nodeName = getPreffixedNodeName(kBody, hNode->m_fNodeName);
	FbxSkeleton* lSkeletonLimbAttribute = FbxSkeleton::Create(pScene, nodeName);
	lSkeletonLimbAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);


	// Create new limb node
	FbxNode* lSkeletonLimb = FbxNode::Create(pScene, nodeName);
	lSkeletonLimb->SetNodeAttribute(lSkeletonLimbAttribute);

	// Set joint initial orientation
	lSkeletonLimb->LclTranslation.Set(hNode->m_translation);
	lSkeletonLimb->LclRotation.Set(hNode->m_rotation);

	// Pre Rotation is active for this joint
	if (hNode->m_preRot != FbxDouble3()) {
		lSkeletonLimb->SetRotationActive(true);
		lSkeletonLimb->SetPreRotation(FbxNode::eSourcePivot, hNode->m_preRot);
	}

	// Set color attribute ( yellow )
	lSkeletonLimbAttribute->SetLimbNodeColor(FbxColor(1,1,0));

	// Set joint ype
	if ( hNode->m_kTwin < JointType_Count)
		setJointTypeProperty(lSkeletonLimb, hNode->m_kTwin);

	// Add node to hierarchy
	fNode->AddChild(lSkeletonLimb);

	// Traverse node children to add remaining nodes
	for (auto &it : hNode->m_children) {
		createHierarchy(pScene, kBody, lSkeletonLimb, it);
	}



}


/// <summary>
/// Gets pre-fixed name of a node
/// </summary>
/// <param name="inKBody">Kinect body for which the preffix will be retrieved</param>
/// <param name="nodeName">Node name without preffix</param>
/// <param name="outName">Variable where name will be written to</param>
FbxString KinectSkeletonMapper::getPreffixedNodeName(IBody *inKBody, const FbxString nodeName) {

	UINT64 bodyTrackingId;
	char nameBuffer[20];

	// Get body tracking  id
	inKBody->get_TrackingId(&bodyTrackingId);
	sprintf_s(nameBuffer, c_SkelRootIdPatternPreffix, bodyTrackingId);
	
	FbxString prefixedName(nameBuffer);
	prefixedName += nodeName;

	return prefixedName;
}

/// <summary>
/// Gets joint type property, based on Kinect's JointType
/// </summary>
/// <param name="fNode">Node to have type set</param>
/// <return>Type of the corresponding node, in case property is not found, returns JointType_Count</return>
JointType KinectSkeletonMapper::getJointTypeProperty(FbxNode *fNode) {

	// Node is not valid
	if (!fNode)
		return JointType_Count;

	JointType propertyReturn;


	FbxProperty lProperty = fNode->FindProperty(c_jointTypePropertyDefaultName);
	if (lProperty.IsValid()) {
		int jointTypeNum = (int)lProperty.Get<FbxInt>();
		propertyReturn = JointType(jointTypeNum);
	}
	else {
		propertyReturn = JointType_Count;
	}

	return propertyReturn;
}


/// <summary>
/// Sets joint type property, based on Kinect's JointType
/// </summary>
/// <param name="fNode">Node to have type set</param>
/// <param name="kType">Kinect joint type</param>
void KinectSkeletonMapper::setJointTypeProperty(FbxNode *fNode, JointType kType) {
	FbxProperty lProperty = fNode->FindProperty(c_jointTypePropertyDefaultName);

	if (!lProperty.IsValid()) {
		lProperty = FbxProperty::Create(fNode, fbxsdk::FbxIntDT, c_jointTypePropertyDefaultName, c_jointTypePropertyDefaultName);
	}
	lProperty.Set(FbxInt(kType));
}


/// <summary>
/// A new frame has been received, add animation corresponding to the motion
/// </summary>
/// <param name="pScene">FBX scene</param>
/// <param name="rootNode">FBX root node</param>
/// <param name="frameTime">Current frame time</param>
/// <param name="kBody">Kinect body to have animation extracted</param>
void KinectSkeletonMapper::addAnimationKeys(FbxScene*  pScene, FbxNode *rootNode, INT64 frameTime, IBody *kBody) {
	// Get Kinect Joint Information
	Joint joints[JointType_Count];
	JointOrientation orientations[JointType_Count];

	HRESULT hr;
	hr = kBody->GetJoints(_countof(joints), joints);
	// If we successfully retrieved the Joint array
	if (!SUCCEEDED(hr)) {
		return;
	}

	hr = kBody->GetJointOrientations(_countof(orientations), orientations);
	// If we successfully retrieved the JointOrientation array
	if (!SUCCEEDED(hr)) {
		return;
	}
	addAnimationKeys(pScene, rootNode, frameTime, joints, orientations);
}
/// <summary>
/// A new frame has been received, add animation corresponding to the motion
/// </summary>
/// <param name="pScene">FBX scene</param>
/// <param name="rootNode">FBX root node</param>
/// <param name="frameTime">Current frame time</param>
/// <param name="joints">Kinect joint position array</param>
/// <param name="orientations">Kinect joint orientation array</param>
void KinectSkeletonMapper::addAnimationKeys(FbxScene*  pScene, FbxNode *rootNode, INT64 frameTime, Joint *joints, JointOrientation *orientations) {

	FbxAnimStack *baseAnimStack = pScene->GetCurrentAnimationStack();

	// Anim Stack invalid
	if (!baseAnimStack)
		return;

	FbxAnimLayer *baseAnimLayer = baseAnimStack->GetMember<FbxAnimLayer>();

	//Anim layer invalid
	if (!baseAnimLayer)
		return;

	int childCount = rootNode->GetChildCount();
	for (int i = 0; i < childCount; i++) {
		FbxNode *childNode = rootNode->GetChild(i);
		animateHierarchy(baseAnimLayer, childNode, frameTime, joints, orientations);
	}


}

/// <summary>
/// Recursively add animation keys to each node in the hierarchy
/// </summary>
/// <param name="pLayer">FBX  animation layer</param>
/// <param name="fNode">FBX node to be animated</param>
/// <param name="frameTime">Current frame time</param>
/// <param name="kJoints">Kinect joints to bild animation</param>
/// <param name="kJointOrientations">Kinect joint orientations to build animation</param>
/// <param name="accumulator">Auxiliary matrix that helps converting from absolute orientation to relative. Defaults to identity</param>
void KinectSkeletonMapper::animateHierarchy(FbxAnimLayer*  pLayer, FbxNode *fNode, INT64 frameTime, Joint *kJoints, JointOrientation *kOrientations, FbxAMatrix accumullator) {

	// Retrieve node joint type
	JointType nodeJointType = getJointTypeProperty(fNode);

	// Number of children this node has
	int childCount = fNode->GetChildCount();


	// Check if joint type is valid
	if (nodeJointType < JointType_Count && childCount > 0 ) {
		
		
		// This is the root joint in Kinect
		if (nodeJointType == c_kinectRootJointType) {
			addTranslationKeys(pLayer, fNode, frameTime, kJoints[nodeJointType]);
		}

		// Set the orientation of this joint 
		FbxNode *childNode = fNode->GetChild(0);
		JointType childJointType = getJointTypeProperty(childNode);

		if (childJointType < JointType_Count) {
			if ( childCount == 1  ) {
				if (!isOrientationNull(kOrientations[childJointType])) {
					// Orientation for Kinect joints is always related to the parent bone
					addRotationKeys(pLayer, fNode, frameTime, kOrientations[childJointType], accumullator);
				}
				else { // The trickiest case, no orientation information for the last bone
					// We can estimate it ourselves, but there will not be roll rotation
					FbxQuaternion ori = estimateBoneOri(childNode, kJoints[nodeJointType], kJoints[childJointType]);
					addRotationKeys(pLayer, fNode, frameTime, ori, accumullator);
				}
			}
			else {
				addRotationKeys(pLayer, fNode, frameTime, kOrientations[nodeJointType], accumullator);
			}
		}

	}



	// Add animations keys for the rest of hierarchy
	for (int i = 0; i < childCount; i++) {
		FbxNode *childNode = fNode->GetChild(i);
		animateHierarchy(pLayer, childNode, frameTime, kJoints, kOrientations,accumullator);
	}

}

/// <summary>
/// Extracts translation information from kinect, and add it as a key to our animation layer
/// </summary>
/// <param name="pLayer">FBX  animation layer</param>
/// <param name="fNode">FBX node to be animated</param>
/// <param name="frameTime">Current frame time</param>
/// <param name="kJoint">Kinect joint to have position extracted from</param>
void KinectSkeletonMapper::addTranslationKeys(FbxAnimLayer*  pLayer, FbxNode *fNode, INT64 frameTime, Joint &kJoint) {

	// Define X, Y and Z coordinates
	FbxDouble3 originalPos = fNode->LclTranslation.Get();
	float scalingFactor = c_positionalScalingFactor;
	//float scalingFactor = getTranslationScaleProperty(fNode);
	float Xpos, Ypos, Zpos;
	Xpos = ((-kJoint.Position.X)*scalingFactor) + ((float)originalPos[0]);
	Ypos = (kJoint.Position.Y*scalingFactor) + ((float)originalPos[1]);
	Zpos = ((kJoint.Position.Z)*scalingFactor) + ((float)originalPos[2]);


	// Set key timestamp
	FbxTime ltime;
	ltime.SetMilliSeconds(frameTime);

	applyTransformation(pLayer, fNode, FbxDouble3(Xpos, Ypos, Zpos), ltime, true);
}


/// <summary>
/// Extracts rotation information from kinect, and add it as a key to our animation layer
/// </summary>
/// <param name="pLayer">FBX  animation layer</param>
/// <param name="fNode">FBX node to be animated</param>
/// <param name="frameTime">Current frame time</param>
/// <param name="kOrientation">Kinect joint orientation</param>
/// <param name="accumullator">Used to compute orientations based on parent joint</param>
void KinectSkeletonMapper::addRotationKeys(FbxAnimLayer*  pLayer, FbxNode *fNode, INT64 frameTime, JointOrientation &kOrientation, FbxAMatrix &accumullator) {

	// Joint orientation, represented as a quaternion
	fbxsdk::FbxQuaternion qRot;

	//Extract quaternion from kinect
	qRot = fbxsdk::FbxQuaternion(kOrientation.Orientation.x,
		kOrientation.Orientation.y, kOrientation.Orientation.z, kOrientation.Orientation.w);

	addRotationKeys(pLayer, fNode, frameTime, qRot, accumullator);
}

/// <summary>
/// Extracts rotation information from kinect, and add it as a key to our animation layer
/// </summary>
/// <param name="pLayer">FBX  animation layer</param>
/// <param name="fNode">FBX node to be animated</param>
/// <param name="frameTime">Current frame time</param>
/// <param name="qRot">Joint orientation as quaternion</param>
/// <param name="accumullator">Used to compute orientations based on parent joint</param>
void KinectSkeletonMapper::addRotationKeys(FbxAnimLayer*  pLayer, FbxNode *fNode, INT64 frameTime, FbxQuaternion qRot, FbxAMatrix &accumullator) {

	// Decompose it into euler angles ( make sure to convert from absolute orientation to relative )
	fbxsdk::FbxAMatrix localOriMat, qMat;
	qMat.SetQ(qRot);
	localOriMat = accumullator.Inverse() * qMat;
	FbxVector4 euler = localOriMat.GetR();

	// Make sure rotation is continuous (switched to post-processing filter)
	//if (rotationCurveX->KeyGetCount() > 0) {
	//	FbxVector4 previousEulerRotation = getEulerRotation(rotationCurveX->KeyGetCount()-1, rotationCurveX, rotationCurveY, rotationCurveZ);
	//	euler = makeRotationContinuous(previousEulerRotation, euler);
	//}

	// Set key timestamp
	FbxTime ltime;
	ltime.SetMilliSeconds(frameTime);

	applyTransformation(pLayer, fNode, euler, ltime, false);

	// Update accumulator, based on local orientation
	accumullator = accumullator * localOriMat;
}


/// <summary>
/// Applies post processing filters to our motion data
/// </summary>
/// <param name="pScene">FBX  scene</param>
void KinectSkeletonMapper::applyPostProcessingFilters(FbxScene*  pScene) {

	FbxAnimStack *baseAnimStack = pScene->GetCurrentAnimationStack();

	// Anim Stack invalid
	if (!baseAnimStack)
		return;

	FbxAnimLayer *baseAnimLayer = baseAnimStack->GetMember<FbxAnimLayer>();

	//Anim layer invalid
	if (!baseAnimLayer)
		return;

	// Apply unroll filter to the whole hierarchy
	// This will ensure we get rid of discontinuity errors caused by euler conversion ( Euler sucks! )
	FbxAnimCurveFilterUnroll lUnrollFilter;
	
	// Apply filter for each skeleton in scene
	int sceneChildrenCount = pScene->GetRootNode()->GetChildCount();
	for (int i = 0; i < sceneChildrenCount; i++)  {
		FbxNode *fNode =  pScene->GetRootNode()->GetChild(i);
		FbxNodeAttribute *fNodeAttribute =  fNode->GetNodeAttribute();  
		if (fNodeAttribute && (fNodeAttribute->GetAttributeType() == FbxNodeAttribute::eSkeleton ) ) {
			applyFilterHierarchically(lUnrollFilter, fNode->GetChild(0));
		}
	}

}


/// <summary>
/// Gets euler rotation for key with certain index
/// </summary>
/// <param name="keyIndex">Index of the desired key</param>
/// <param name="rotationCurveX">Rotation curve X</param>
/// <param name="rotationCurveY">Rotation curve Y</param>
/// <param name="rotationCurveZ">Rotation curve Z</param>
/// <return>Rotation in euler angles</return>
FbxVector4 KinectSkeletonMapper::getEulerRotation(int keyIndex, FbxAnimCurve *rotationCurveX, FbxAnimCurve *rotationCurveY, FbxAnimCurve *rotationCurveZ) {
	FbxVector4 eulerAngles;

	eulerAngles[0] = rotationCurveX->KeyGet(keyIndex).GetValue();
	eulerAngles[1] = rotationCurveY->KeyGet(keyIndex).GetValue();
	eulerAngles[2] = rotationCurveZ->KeyGet(keyIndex).GetValue();
	return eulerAngles;
}

/// <summary>
/// Make sure rotation in Euler angles is continuous
/// </summary>
/// <param name="previousEuler">Rotation for the previous frame</param>
/// <param name="currentEuler">Rotaion for the current frame</param>
/// <return>Continuous rotation, in euler angles</return>
FbxVector4 KinectSkeletonMapper::makeRotationContinuous(FbxVector4 previousEuler, FbxVector4 currentEuler) {
	FbxVector4 resultingRotation;

	// For each coordinate
	for (int i = 0; i < 3; i++) {
		resultingRotation[i] = currentEuler[i];

		// Curve is non-continuous, fix it
		while (abs(resultingRotation[i] - previousEuler[i]) > c_rotationContinuityMaxOffset) {
			if (resultingRotation[i] < previousEuler[i])
				resultingRotation[i] += c_rotationContinuityMaxOffset;
			else
				resultingRotation[i] -= c_rotationContinuityMaxOffset;
		} 

	}
	return resultingRotation;
}

/// <summary>
/// Defines how translation values will be calculated
/// </summary>
/// <param name="fNode">Root FBX  node</param>
/// <param name="kJoints">Kinect joint position info</param>
void KinectSkeletonMapper::setTransScaling(FbxNode *fNode, Joint *joints) {

	int childCount = fNode->GetChildCount();

	if (childCount != 1)
		return;
	
	FbxNode *rootChild = fNode->GetChild(0);
	FbxNode *cChild = rootChild->GetChild(0);


	JointType  rType = getJointTypeProperty(rootChild);
	JointType  cType = getJointTypeProperty(cChild);

	// Invalid joint type
	if (rType >= JointType_Count || cType >= JointType_Count)
		return;

	CameraSpacePoint pr = joints[rType].Position;
	CameraSpacePoint pc = joints[cType].Position;
	
	// Estimate the difference between real distance and the modelled
	FbxVector4 modelV = cChild->LclTranslation.Get();
	FbxVector4 realV(pc.X - pr.X, pc.Y - pr.Y, pc.Z - pr.Z);
	double scale = modelV.Length() / realV.Length();

	setTranslationScaleProperty(rootChild, float(scale));
}


/// <summary>
/// Sets some initial alignment rules, based on assumptions about the sensor
/// </summary>
/// <param name="fNode">Root FBX  node</param>
/// <param name="kJoints">Kinect joint position info</param>
/// <param name="kOrientations">Kinect joint orientation info</param>
void KinectSkeletonMapper::setInitialAlignmentRules(FbxNode *fNode, Joint *joints, JointOrientation *orientations) {
	

	int childCount = fNode->GetChildCount();

	if (childCount != 1)
		return;

	FbxNode *rootChild = fNode->GetChild(0);

	JointType  jType = getJointTypeProperty(rootChild);

	// Invalid joint type
	if (jType >= JointType_Count)
		return;

	Vector4 jOri = orientations[jType].Orientation;

	FbxAMatrix initOriM;
	initOriM.SetQ(FbxQuaternion(jOri.x, jOri.y, jOri.z, jOri.w));

	// We need to account for reference node initial alignment
	FbxAMatrix refAlignM;
	refAlignM.SetR(FbxDouble3(0, 180, 0));

	// Obtain global orientation
	initOriM = refAlignM * initOriM;

	FbxVector4 initOriEuler = initOriM.GetR();
	initOriEuler[1] = initOriEuler[2] = 0; // We just need to compensate for the x axis

	// Let us compensate for inclination added by sensor
	rootChild->SetRotationActive(true);
	rootChild->SetPreRotation(FbxNode::eSourcePivot, initOriEuler);
}

/// <summary>
/// Estimates rotation between parent joint and hild joint. Used to compensate for missing Kinect info
/// </summary>
/// <param name="cNode">Child FBX Node</param>
/// <param name="pJoint">Parent Kinect Joint</param>
/// <param name="cJoint">Child Kinect Joint</param>
FbxQuaternion KinectSkeletonMapper::estimateBoneOri(FbxNode *cNode, const Joint &pJoint, const Joint &cJoint) {

	FbxVector4 ref = cNode->LclTranslation.Get();
	ref.Normalize();
	FbxVector4 goal(cJoint.Position.X - pJoint.Position.X, cJoint.Position.Y - pJoint.Position.Y, cJoint.Position.Z - pJoint.Position.Z);
	goal.Normalize();

	// We got our axis-angle ( Not supported by FBX, convert to quaternion )
	FbxVector4 axisAngle = ref.CrossProduct(goal);
	axisAngle.Normalize();
	double angle = acos(ref.DotProduct(goal));
	axisAngle[3] = angle;

		// To quaternion
	FbxQuaternion quat = axisAngleToQuat(axisAngle);
	return quat;
}