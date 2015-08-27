#include "FBXJointConverter.h"

// Constant definition
char * FBXJointConverter::c_setPrefix = "MarkerSet";
char * FBXJointConverter::c_markerPrefix = "Marker";
char * FBXJointConverter::c_separator = ":";
FbxAMatrix FBXJointConverter::c_mIdentity = FbxAMatrix();
std::vector<FbxTime> FBXJointConverter::c_emptyVector = std::vector<FbxTime>();



/// <summary>
/// Creates absolute markers from hierarchical skeleton
/// </summary>
/// <param name="pScene">FBX Scene</param>
/// <param name="sNode">Skeleton root node</param>
/// <param name="parentTrans">Parent transformation ( defaults to identity )</param>
/// <param name="onlyCopyTPose">Don't animate markers, only copy T-Pose information</param>
/// <return>Node representing marker Set</return>
FbxNode* FBXJointConverter::toAbsoluteMarkers(FbxScene *pScene, FbxNode *sNode, bool gobalTransformationEnable, bool onlyCopyTPose)  {

	const char *nodeName = sNode->GetName();

	// Create marker set
	char setName[100];
	sprintf_s(setName, "%s%s%s", c_setPrefix, c_separator, nodeName);
	FbxNode *markerSet = FbxNode::Create(pScene, setName);
	markerSet->SetNameSpace(c_setPrefix);
	
	// Add set to scene
	pScene->GetRootNode()->AddChild(markerSet);

	// Create markers for each joint
	createMarkersHierarchy(pScene, markerSet, sNode);

	// Get Animation Layer
	FbxAnimStack *pStack = pScene->GetCurrentAnimationStack();
	FbxAnimLayer *pLayer = pStack->GetMember<FbxAnimLayer>();

	if (pLayer) {
		// Time to animate the markers
		// For each keyframe, animate markers accordingly
		std::vector<FbxTime> keyTimeVec;

		// Collect keyframe times
		FbxAnimCurve *rootTranslationCurveX = sNode->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_X, false);
		extractKeyTimesFromCurve(rootTranslationCurveX, keyTimeVec);


		// For each key time, animate markers
		for (auto &it : keyTimeVec) {
			animatePositionalMarkers(pLayer, it, markerSet, sNode, gobalTransformationEnable);
			// Only get time at position 0
			if (onlyCopyTPose)
				break;
		}
		
		UI_Printf("Markers were animated.");

		// Turn markers into another skeleton ( TESTING )
		//FbxNode *bip2skel = fromAbsoluteMarkers(pScene, sNode, "Bip02", keyTimeVec);

		UI_Printf("Skeleton has been created.");

		UI_Printf("Post processing has been finished.");

	}


	// Return  set of markers that was created
	return markerSet;

}

/// <summary>
/// Creates hierarchical Skeleton from absolute markers
/// </summary>
/// <param name="pScene">FBX Scene</param>
/// <param name="sNode">Skeleton used as a reference to build the hierarchy</param>
/// <param name="newSkelName">Name of new skeleton to be created</param>
/// <param name="markerSet">Markers to be used</param>
/// <param name="keyTimeVec">Vector of key times to use, if empty, will be initialized from markers</param>
/// <return>Pointer to our newly created skeleton</return>
FbxNode* FBXJointConverter::fromAbsoluteMarkers(FbxScene *pScene, FbxNode *refNode, char *newSkelName, double fps, bool enableGlobalTransformation, FbxNode *markerSet, std::vector<FbxTime> &keyVec)  {


	// Find markers
	if (!markerSet)
		markerSet = findMarkerSet(pScene, refNode);

	if (!markerSet) {
		// Unable to find marker set on the scene
		UI_Printf("Set of positional markers could not be found on the scene. Make sure they are being created.");
		return NULL;
	}

	// If not initialized
	/*if (keyVec.size() == 0) {

		// Get Animation Layer
		FbxAnimStack *pStack = pScene->GetCurrentAnimationStack();
		FbxAnimLayer *pLayer = pStack->GetMember<FbxAnimLayer>();


		// Collect keyframe times
		FbxAnimCurve *rootTranslationCurveX = markerSet->GetChild(0)->LclRotation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_X, false);
		extractKeyTimesFromCurve(rootTranslationCurveX, keyVec);
	}*/

	/// Create a new skeleton
	FbxSkeleton *newSkelAttribute = FbxSkeleton::Create(pScene, newSkelName);
	FbxNode *newSkelNode = FbxNode::Create(pScene, newSkelName);
	newSkelNode->SetNodeAttribute(newSkelAttribute);

	// Add to scene
	pScene->GetRootNode()->AddChild(newSkelNode);

	// Copy hierarchy
	copySkeleton(pScene, refNode, newSkelNode);


	// Get Animation Layer
	FbxAnimStack *pStack = pScene->GetCurrentAnimationStack();
	FbxAnimLayer *pLayer = pStack->GetMember<FbxAnimLayer>();

	if (pLayer) {
		// For each key time,Animate new skeleton
		bool hasKeys;
		int keyTimeIndex = 0;
		FbxTime keyTime;
		do {
			keyTime.SetSecondDouble(double(keyTimeIndex) / fps);
			hasKeys = animateJointsFromMarkers(pLayer, keyTime, markerSet, newSkelNode, enableGlobalTransformation);
			keyTimeIndex++;
		} while (hasKeys);
	}


	// Reset name after animating
	newSkelAttribute->SetName(newSkelName);
	newSkelNode->SetName(newSkelName);

	return newSkelNode;
}


/// <summary>
/// Recursive function that adds a positional marker for each joint in the hierarchy
/// </summary>
/// <param name="pScene">FBX Scene</param>
/// <param name="markerSet">Set to have markers added to, if NULL, markers are added to scene</param>
/// <param name="cNode">Current joint node in the hierarchy</param>
int FBXJointConverter::createMarkersHierarchy(FbxScene *pScene, FbxNode *markerSet, FbxNode *cNode, int idGen) {



	const char *name = cNode->GetName();

	// If cNode is animatable, create marker
	if (isAnimatable(cNode)) {
		char markerName[100];
		sprintf_s(markerName, "%s%s%s", c_markerPrefix, c_separator, cNode->GetName());
		FbxMarker *cMarkerAttribute = FbxMarker::Create(pScene, markerName);
		FbxNode *markerNode = FbxNode::Create(pScene, markerName);
		markerNode->SetNameSpace(c_markerPrefix);


		markerNode->SetNodeAttribute(cMarkerAttribute);

		setCustomIdProperty(markerNode, idGen);
		idGen++;

		// Get Animation Layer
		FbxAnimStack *pStack = pScene->GetCurrentAnimationStack();
		FbxAnimLayer *pLayer = pStack->GetMember<FbxAnimLayer>();
		// Initialize curves, based on reference node
		if (pLayer) {
			copyCurveInformation(pLayer, cNode, markerNode);
			applyTransformationMatrix(markerNode, pLayer, FbxTime(0), c_mIdentity, false);
			addVirtualMarkerPosition(pLayer, markerNode, pScene);
			
		}

		// Add to hierarchy
		if (markerSet != NULL) {
			markerSet->AddChild(markerNode);
		}
		else {
			pScene->GetRootNode()->AddChild(markerNode);
		}
	}
	

	// Repeat for children
	int childCount = cNode->GetChildCount();
	for (int i = 0; i < childCount; i++)
		idGen = createMarkersHierarchy(pScene, markerSet, cNode->GetChild(i), idGen);

	return idGen;

}


/// <summary>
/// Recursive function that animate psoitional markers created from skeleton
/// </summary>
/// <param name="pLayer">FBX Animation layer</param>
/// <param name="kTime">Time for which corresponding key will be created</param>
/// <param name="mSet">Marker Set</param>
/// <param name="cNode">Current skeleton node</param>
/// <param name="parentTrans">Parent transformation ( defaults to identity )</param>
void FBXJointConverter::animatePositionalMarkers(FbxAnimLayer *pLayer, FbxTime kTime, FbxNode *mSet, FbxNode *cNode, bool enableGlobalTransformation, FbxAMatrix parentTrans) {

	FbxAMatrix cTransformation;
	
	if (enableGlobalTransformation) {
		// Find transformation for current node
		cTransformation = parentTrans;
	}
	
	// Only perform this step if joint is animatable
	if (isAnimatable(cNode)) {
		cTransformation = cTransformation *extractTransformationMatrix(cNode, pLayer, kTime);

		// Find corresponding marker
		FbxNode *cMarkerNode = findMarker(mSet, cNode);

		// If marker has been found, apply transformation to it
		if (cMarkerNode) {
			//applyTransformationMatrix(cMarkerNode, pLayer, kTime, cTransformation, false);
			applyTransformationVirtualMarkers(cMarkerNode, pLayer, kTime, cTransformation);
		}
	}
	


	//Repeat for children
	int childCount = cNode->GetChildCount();
	for (int i = 0; i < childCount; i++)
		animatePositionalMarkers(pLayer, kTime, mSet, cNode->GetChild(i), enableGlobalTransformation ,cTransformation);


}

/// <summary>
/// Recursive function that animates skeleton from positonal markers. Used for curve reconstruction
/// </summary>
/// <param name="pLayer">FBX Animation layer</param>
/// <param name="kTime">Time for which corresponding key will be created</param>
/// <param name="mSet">Marker Set</param>
/// <param name="tgtNode">Node to be animated</param>
/// <param name="parentTrans">Parent transformation ( defaults to identity )</param>
bool FBXJointConverter::animateJointsFromMarkers(FbxAnimLayer *pLayer, FbxTime kTime, FbxNode *mSet, FbxNode *tgtNode, bool enableGlobalTransformation, FbxAMatrix parentTrans) {
	


	// Transformation to be passed on to our children. By default, it should be our own transformaiton
	FbxAMatrix childReferenceTransformation = parentTrans;

	// Find corresponding marker
	FbxNode *tgtMarkerNode = findMarker(mSet, tgtNode->GetNameOnly());

	// Return value. False if this curve no longer has any keys past kTime
	bool returnHasKeys = false;

	// If marker has been found, animate it
	if (tgtMarkerNode && isAnimatable(tgtNode) ) {

		returnHasKeys = hasMoreKeys(kTime, tgtMarkerNode->GetChild(0),pLayer);

		// Find transformation for current node. Extract transformation from marker in order to compute it.
		//FbxAMatrix markerTrans = extractTransformationMatrix(tgtMarkerNode, pLayer, kTime);
		FbxAMatrix markerTrans = extractTransformationFromVirtualMarkers(tgtMarkerNode, pLayer, kTime);
		FbxAMatrix tgtTransformation;
		if (enableGlobalTransformation) {
			// Compute absolute position of marker
//			if (parentTrans != c_mIdentity)
//				markerTrans.SetTOnly(parentTrans.GetT());

			FbxAMatrix invParentTrans = parentTrans.Inverse();
			tgtTransformation = invParentTrans *  markerTrans; // Marker has global trans
		}
		else {
			tgtTransformation = markerTrans; // marker has local trans
		}

		// Apply transformation to node
		if ( returnHasKeys )
			applyTransformationMatrix(tgtNode, pLayer, kTime, tgtTransformation, false, tgtMarkerNode);

		// Pass information to children
		childReferenceTransformation = markerTrans;
	}



	//Repeat for children
	int childCount = tgtNode->GetChildCount();
	for (int i = 0; i < childCount; i++)
		returnHasKeys |= animateJointsFromMarkers(pLayer, kTime, mSet, tgtNode->GetChild(i), enableGlobalTransformation, childReferenceTransformation);

	return returnHasKeys;
}

/// <summary>
/// Extract key times from curve, and add them to provided vector
/// </summary>
/// <param name="srcCurve">Curve where key information is located</param>
/// <param name="out_time">Vector where information will be written to</param>
void FBXJointConverter::extractKeyTimesFromCurve(FbxAnimCurve *srcCurve, std::vector<FbxTime> &out_time) {

	// Get key count
	int keyCount = srcCurve->KeyGetCount();
	out_time.resize(keyCount);

	// Collect each key
	for (int i = 0; i < keyCount; i++) {
		FbxAnimCurveKey cKey =  srcCurve->KeyGet(i);
		FbxTime cTime = cKey.GetTime();
		out_time[i] = cTime;
	}

}


/// <summary>
/// Extract transformation matrix from node at a given time
/// </summary>
/// <param name="cNode">Fbx Node to have information extracted from</param>
/// <param name="pLayer">FBX Animation layer</param>
/// <param name="kTime">Time for whic transformaiton is acquired</param>
/// <return>Transformation matrix</return>
FbxAMatrix FBXJointConverter::extractTransformationMatrix(FbxNode *cNode, FbxAnimLayer *pLayer, FbxTime kTime) {

	
	// Result matrix to be returned
	FbxAMatrix resultMat;

	// Transformation obtained from animation
	FbxDouble3 translation, rotationEuler;

	// Default values for Rotation and translation
	FbxDouble3 defaultTrans, defaultRot , defaultScale(1,1,1);
	defaultTrans = cNode->LclTranslation.Get();
	defaultRot = cNode->LclRotation.Get();

	// Get Translation curves
	FbxAnimCurve *tXCurve =  cNode->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_X, false);
	FbxAnimCurve *tYCurve = cNode->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Y, false);
	FbxAnimCurve *tZCurve = cNode->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Z, false);

	// Copy translation information
	if (tXCurve)
		translation[0] = tXCurve->Evaluate(kTime);
	if (tYCurve)
		translation[1] = tYCurve->Evaluate(kTime);
	if (tZCurve)
		translation[2] = tZCurve->Evaluate(kTime);


	// Get rotation curves
	FbxAnimCurve *rXCurve = cNode->LclRotation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_X, false);
	FbxAnimCurve *rYCurve = cNode->LclRotation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Y, false);
	FbxAnimCurve *rZCurve = cNode->LclRotation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Z, false);

	// Copy rotaiton information
	if (rXCurve)
		rotationEuler[0] = rXCurve->Evaluate(kTime);
	if (rYCurve)
		rotationEuler[1] = rYCurve->Evaluate(kTime);
	if (rZCurve)
		rotationEuler[2] = rZCurve->Evaluate(kTime);

	// Set transformation matrix
	// TODO: Add scale ( not necessary for now )
	resultMat.SetTRS(translation, rotationEuler, defaultScale);

	// Return matrix
	return resultMat;
}

/// <summary>
/// Applies transformation matrix to marker node
/// </summary>
/// <param name="cNode">Fbx Node to have transformation applied to/param>
/// <param name="pLayer">FBX Animation layer</param>
/// <param name="kTime">Time for transformation</param>
/// <param name="T">Transformation Matrix</param>
/// <param name="forceMode">Boolean value that defines whether we force the creation of animation curves, in case they do not exist (defaults to true).</param>
void FBXJointConverter::applyTransformationMatrix(FbxNode *cNode, FbxAnimLayer *pLayer, FbxTime kTime, FbxAMatrix &T, bool forceMode, FbxNode *refNode) {


	// Extract rotation ( in euler) and translation from matrix
	FbxDouble3 rotationEuler = T.GetR();
	FbxDouble3 translation = T.GetT();



	// Get Translation curves
	FbxAnimCurve *tXCurve = cNode->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_X, forceMode);
	FbxAnimCurve *tYCurve = cNode->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Y, forceMode);
	FbxAnimCurve *tZCurve = cNode->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Z, forceMode);

	bool applytXCurve = true, applytYCurve = true, applytZCurve = true;

	if (refNode != NULL) {
		FbxAnimCurve *reftXCurve = refNode->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_X, false);
		FbxAnimCurve *reftYCurve = refNode->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Y, false);
		FbxAnimCurve *reftZCurve = refNode->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Z, false);

		applytXCurve = (reftXCurve);// && (hasKeysAt(pLayer, refNode, kTime));
		applytYCurve = (reftYCurve);// && (hasKeysAt(pLayer, refNode, kTime));
		applytZCurve = (reftZCurve);// && (hasKeysAt(pLayer, refNode, kTime));
	}

	if (tXCurve && applytXCurve)
		applyTransformationVectorToCurve(tXCurve, kTime, (float)translation[0], FbxAnimCurveDef::eInterpolationLinear);

	if (tYCurve && applytYCurve)
		applyTransformationVectorToCurve(tYCurve, kTime, (float)translation[1], FbxAnimCurveDef::eInterpolationLinear);

	if (tZCurve && applytZCurve)
		applyTransformationVectorToCurve(tZCurve, kTime, (float)translation[2], FbxAnimCurveDef::eInterpolationLinear);

	// Repeat the same thing for rotation
	// Get Rotation curves
	FbxAnimCurve *rXCurve = cNode->LclRotation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_X, forceMode);
	FbxAnimCurve *rYCurve = cNode->LclRotation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Y, forceMode);
	FbxAnimCurve *rZCurve = cNode->LclRotation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Z, forceMode);

	bool applyrXCurve = true, applyrYCurve = true, applyrZCurve = true;

	if (refNode != NULL) {
		FbxAnimCurve *refrXCurve = refNode->LclRotation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_X, false);
		FbxAnimCurve *refrYCurve = refNode->LclRotation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Y, false);
		FbxAnimCurve *refrZCurve = refNode->LclRotation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Z, false);

		applyrXCurve = (refrXCurve);// && (hasKeysAt(pLayer, refNode, kTime));
		applyrYCurve = (refrYCurve);// && (hasKeysAt(pLayer, refNode, kTime));
		applyrZCurve = (refrZCurve);// && (hasKeysAt(pLayer, refNode, kTime));
	}

	if (rXCurve && applyrXCurve)
		applyTransformationVectorToCurve(rXCurve, kTime, (float)rotationEuler[0], FbxAnimCurveDef::eInterpolationCubic);

	if (rYCurve && applyrYCurve)
		applyTransformationVectorToCurve(rYCurve, kTime, (float)rotationEuler[1], FbxAnimCurveDef::eInterpolationCubic);

	if (rZCurve && applyrZCurve)
		applyTransformationVectorToCurve(rZCurve, kTime, (float)rotationEuler[2], FbxAnimCurveDef::eInterpolationCubic);

	// Finished creating keys



}


/// <summary>
/// Makes a copy of a skeleton hierarchy, without copying animation keys
/// </summary>
/// <param name="pScene">FBX Scene</param>
/// <param name="oriSkel">Skeleton that will be copied</param>
/// <param name="tgtSkel">Skeleton root to receive hierarchy</param>
void FBXJointConverter::copySkeleton(FbxScene *pScene, FbxNode *oriSkel, FbxNode *tgtSkel) {



	// Copy attributes
	tgtSkel->SetNodeAttribute(oriSkel->GetNodeAttribute());

	// Copy properties
	tgtSkel->Copy(*oriSkel);

	// Enable Quaternion interpolation
	tgtSkel->SetQuaternionInterpolation(FbxNode::EPivotSet::eSourcePivot, EFbxQuatInterpMode::eQuatInterpSlerp);

	// Get Animation Layer
	FbxAnimStack *pStack = pScene->GetCurrentAnimationStack();
	FbxAnimLayer *pLayer = pStack->GetMember<FbxAnimLayer>();
	// Initialize curves, based on reference node
	if (pLayer) {
		copyCurveInformation(pLayer, oriSkel, tgtSkel);
	}


	// For each child
	int childCount = oriSkel->GetChildCount();
	for (int i = 0; i < childCount; i++) {

		// Copy Node
		FbxNode *oriJointI = oriSkel->GetChild(i);
		FbxNode *tgtJointI = FbxNode::Create(pScene, oriJointI->GetNameOnly());

		// Add to our new skeleton
		tgtSkel->AddChild(tgtJointI);

		// repeat for child subnodes
		copySkeleton(pScene, oriJointI, tgtJointI);

	}


}

/// <summary>
/// Find set of markers corresponding to a reference skeleton
/// </summary>
/// <param name="pScene">FBX Scene</param>
/// <param name="refNode">Reference skeleton</param>
/// <param name="tgtSkel">Skeleton root to receive hierarchy</param>
FbxNode* FBXJointConverter::findMarkerSet(FbxScene *pScene, FbxNode *refNode) {
	// Find corresponding marker set
	char markerSetName[100];
	sprintf_s(markerSetName, "%s%s%s", c_setPrefix, c_separator ,refNode->GetName());
	FbxNode *cMarkerSetNode = pScene->GetRootNode()->FindChild(markerSetName);

	return cMarkerSetNode;
}



/// <summary>
/// Find marker inside marker set
/// </summary>
/// <param name="mSet">Markert Set node</param>
/// <param name="refNode">Reference skeleton</param>
FbxNode* FBXJointConverter::findMarker(FbxNode *mSet, FbxNode *refNode) {
	const char *refName = refNode->GetName();
	return findMarker(mSet,refName);

}

/// <summary>
/// Find marker inside marker set
/// </summary>
/// <param name="mSet">Markert Set node</param>
/// <param name="refName">Reference name for marker</param>
FbxNode* FBXJointConverter::findMarker(FbxNode *mSet, const char *refName) {
	// Find corresponding marker set
	char markerName[100];
	sprintf_s(markerName, "%s%s%s", c_markerPrefix, c_separator, refName);
	FbxNode *cMarkerNode = mSet->FindChild(markerName);

	return cMarkerNode;
}


/// <summary>
/// Creates key on Curve based on transformation vector
/// </summary>
/// <param name="tgtCurve">Fbx Curve to have key added to/param>
/// <param name="kTime">Time for transformation</param>
/// <param name="kVal">Key value</param>
/// <param name="interpolationType">Interpolation to be used by key</param>


/// <summary>
/// Copy curve information from one node to another. Keys are not copied, only curves are initialized.
/// </summary>
/// <param name="pLayer">FBX Animation layer</param>
/// <param name="srcNode">Node from which information is extracted</param>
/// <param name="tgtNode">Node for which information is copied to</param>
void FBXJointConverter::copyCurveInformation(FbxAnimLayer *pLayer, FbxNode *srcNode, FbxNode *tgtNode) {
	FbxAnimCurve *oriCurveTX = srcNode->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_X);
	FbxAnimCurve *oriCurveTY = srcNode->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Y);
	FbxAnimCurve *oriCurveTZ = srcNode->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Z);
	if (oriCurveTX)
		tgtNode->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
	if (oriCurveTY)
		tgtNode->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
	if (oriCurveTZ)
		tgtNode->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);

	FbxAnimCurve *oriCurveRX = srcNode->LclRotation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_X);
	FbxAnimCurve *oriCurveRY = srcNode->LclRotation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Y);
	FbxAnimCurve *oriCurveRZ = srcNode->LclRotation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Z);
	if (oriCurveRX)
		tgtNode->LclRotation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
	if (oriCurveRY)
		tgtNode->LclRotation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
	if (oriCurveRZ)
		tgtNode->LclRotation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);
}


/// <summary>
/// Find first set of markers in a scene
/// </summary>
/// <param name="pScene">FBX Scene</param>
/// <return>Reference to marker set node, if not found, returns NULL</return>
FbxNode* FBXJointConverter::findAnyMarkerSet(FbxScene *pScene) {

	FbxNode *rootNode = pScene->GetRootNode();

	//	Iterate on children
	int childCount = rootNode->GetChildCount();
	for (int i = 0; i < childCount; i++) {
		FbxNode *childI = rootNode->GetChild(i);

		FbxString childName = childI->GetName();
		if (strncmp(childName, c_setPrefix,strlen(c_setPrefix)) == 0)
			return childI;
	}
	return NULL;
}

FbxNode* FBXJointConverter::findAnySkeleton(FbxScene *pScene) {

	FbxNode *rootNode = pScene->GetRootNode();

	//	Iterate on children
	int childCount = rootNode->GetChildCount();
	for (int i = 0; i < childCount; i++) {
		FbxNode *childI = rootNode->GetChild(i);

		if (childI->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton) {
			return childI;
		}
	} 
	return NULL;
}

void FBXJointConverter::addVirtualMarkerPosition(FbxAnimLayer *pLayer, FbxNode *markerNode, FbxScene *pScene) {


	if (isAnimatable(markerNode)) {
		FbxMarker *cMarkerAttribute = FbxMarker::Create(pScene, "a");
		FbxNode *a = FbxNode::Create(pScene, "a");
		a->SetNameSpace(c_markerPrefix);
		a->SetNodeAttribute(cMarkerAttribute);

		cMarkerAttribute = FbxMarker::Create(pScene, "b");
		FbxNode *b = FbxNode::Create(pScene, "b");
		b->SetNameSpace(c_markerPrefix);
		b->SetNodeAttribute(cMarkerAttribute);

		cMarkerAttribute = FbxMarker::Create(pScene, "c");
		FbxNode *c = FbxNode::Create(pScene, "c");
		c->SetNameSpace(c_markerPrefix);
		c->SetNodeAttribute(cMarkerAttribute);

		markerNode->AddChild(a);
		markerNode->AddChild(b);
		markerNode->AddChild(c);

		a->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
		a->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
		a->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);

		b->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
		b->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
		b->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);

		c->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
		c->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
		c->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);


	}
}


void FBXJointConverter::applyTransformationVirtualMarkers(FbxNode *cMarkerNode, FbxAnimLayer *pLayer, FbxTime kTime, FbxAMatrix cTransformation) {
	static FbxMatrix conv = createConvMatrix();

	FbxMatrix transformM = cTransformation;

	FbxMatrix markersM;

	transformM = relocateTranslation(transformM);
	//markersM = cTransformation * conv ;
	markersM = conv * transformM;

	for (int i = 0; i < 3; i++) {
		int rowIndex = (i == 2) ? i + 1 : i;
		FbxNode *nodeI = cMarkerNode->GetChild(i);
		//FbxVector4 rowVal = markersM.GetRow(rowIndex);
		FbxVector4 rowVal = markersM.GetColumn(rowIndex);

		
		FbxAnimCurve *transXCurve = nodeI->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
		FbxAnimCurve *transYCurve = nodeI->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
		FbxAnimCurve *transZCurve = nodeI->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);


		applyTransformationVectorToCurve(transXCurve, kTime, rowVal[0], FbxAnimCurveDef::eInterpolationCubic);
		applyTransformationVectorToCurve(transYCurve, kTime, rowVal[1], FbxAnimCurveDef::eInterpolationCubic);
		applyTransformationVectorToCurve(transZCurve, kTime, rowVal[2], FbxAnimCurveDef::eInterpolationCubic);


	}

}

FbxAMatrix FBXJointConverter::extractTransformationFromVirtualMarkers(FbxNode *tgtMarkerNode, FbxAnimLayer *pLayer, FbxTime kTime) {
	FbxMatrix resultMatrix, markerMatrix;

	static FbxMatrix inverseConvMatrix = createConvInverse();

	FbxVector4 a = extractTransVectorFromCurves(tgtMarkerNode->GetChild(0), pLayer, kTime);
	FbxVector4 b = extractTransVectorFromCurves(tgtMarkerNode->GetChild(1), pLayer, kTime);
	FbxVector4 c = extractTransVectorFromCurves(tgtMarkerNode->GetChild(2), pLayer, kTime);

	//FbxVector4 d = a.CrossProduct(b);
	FbxVector4 d = (a - c).CrossProduct(b - c) + c;
	d[3] -= 1;

	/*markerMatrix.SetRow(0 ,a);
	markerMatrix.SetRow(1, b);
	markerMatrix.SetRow(2, d);
	markerMatrix.SetRow(3, c);*/
	markerMatrix.SetColumn(0, a);
	markerMatrix.SetColumn(1, b);
	markerMatrix.SetColumn(2, d);
	markerMatrix.SetColumn(3, c);



	resultMatrix = inverseConvMatrix * markerMatrix;


	resultMatrix = relocateTranslation(resultMatrix);

	

	return toAffine(resultMatrix);


}

FbxDouble3 	FBXJointConverter::extractTransVectorFromCurves(FbxNode *node, FbxAnimLayer *pLayer, FbxTime kTime) {

	FbxAnimCurve *xCurve = node->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
	FbxAnimCurve *yCurve = node->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
	FbxAnimCurve *zCurve = node->LclTranslation.GetCurve(pLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);

	FbxDouble3 retVec;

	if (xCurve)
		retVec[0] = xCurve->Evaluate(kTime);

	if (yCurve)
		retVec[1] = yCurve->Evaluate(kTime);

	if (zCurve)
		retVec[2] = zCurve->Evaluate(kTime);

	return retVec;

}