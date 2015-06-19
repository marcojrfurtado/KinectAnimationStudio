#include "FBXTransmitter.h"
#include "FBXJointConverter.h"

/// <summary>
/// Constructor
/// </summary>
FBXTransmitter::FBXTransmitter(FbxManager *fManager) : p_sdkManager(fManager){
}

/// <summary>
/// Destructor
/// </summary>
FBXTransmitter::~FBXTransmitter() {

}


/// <summary>
/// Send file to the server
/// </summary>
/// <param name="inputFileName">MoCap file to be sent over the network</param>
void FBXTransmitter::transmit(char *inputFileName) {

	// Create a scene
	FbxScene* lScene = FbxScene::Create(p_sdkManager, "");
	
	// Load File
	bool r = LoadScene(p_sdkManager, lScene, inputFileName);

	// Check if load was succesful
	if (!r) {
		UI_Printf("	Problem found when trying to load the scene. Make sure %s is a valid FBX file", inputFileName);
		lScene->Destroy();
		return;
	}

	UI_Printf(" File %s has been succesfully loaded.", inputFileName);

	// Get children in the scene
	int childCount = lScene->GetRootNode()->GetChildCount();

	// For each node that is a child of the root
	for (int i = 0; i < childCount; i++) {

		// Get scene child
		FbxNode *pNode = lScene->GetRootNode()->GetChild(i);

		if (pNode == NULL)
			continue;

		FbxNodeAttribute::EType pNodeAttType = pNode->GetNodeAttribute()->GetAttributeType();

		// If not skeleton root, ignore node
		if (pNodeAttType != FbxNodeAttribute::EType::eSkeleton)
			continue;

		// Convert to positional markers ( markers are added to the scene )
		FBXJointConverter::toAbsoluteMarkers(lScene, pNode);
	}

	// Save Scene
	int lFileFormat = p_sdkManager->GetIOPluginRegistry()->FindReaderIDByDescription(c_FBXBinaryFileDesc);
	
	r = SaveScene(p_sdkManager, lScene, "output.fbx", lFileFormat,false);
	if (!r) {
		UI_Printf(" Problem when trying to save scene.");
	}

	lScene->Destroy();
}