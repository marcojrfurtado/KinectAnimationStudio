#pragma once

#include "../common/stdafx.h"

/*
  Class used to load FBX files for transmission over network
*/
class FBXTransmitter {

public:
	/// <summary>
	/// Constructor
	/// </summary>
	FBXTransmitter(FbxManager *fManager = NULL);

	/// <summary>
	/// Destructor
	/// </summary>
	~FBXTransmitter();

	/// <summary>
	/// Init FBX SDK manager
	/// </summary>
	/// <param name="fManager">Pointer to FBX SDK Manager</param>
	void initFBXSDKManager(FbxManager *fManager) { p_sdkManager = fManager; };

	/// <summary>
	/// Send file to the server
	/// </summary>
	/// <param name="inputFileName">MoCap file to be sent over the network</param>
	void transmit(char *inputFileName);

private:
	// Constant Definitions

	// Default file format for output
	const char *c_FBXBinaryFileDesc = "FBX binary(*.fbx)";

	// Object Variables

	// Default SDK manager
	FbxManager *p_sdkManager;
};