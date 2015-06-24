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
	/// <param name="fManager">Pointer to FBX SDK Manager</param>
	/// <param name="tPort">Transmitter port, which server binds to, and client connects to</param>
	/// <param name="cHostName">Nameof the host for which client will be connected to</param>
	FBXTransmitter(FbxManager *fManager = NULL, int tPort = 27015, char *cHostName = NULL );

	/// <summary>
	/// Destructor
	/// </summary>
	~FBXTransmitter();

	/// <summary>
	/// Initialize sockets structure
	/// </summary>
	void initSockets();

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

	/// <summary>
	/// Start Server. Client mode will not work
	/// </summary>
	void startServer();

	/// <summary>
	/// Sets transmitter port
	/// </summary>
	/// <param name="setPort">Sets port used by transmitter</param>
	void setPort(int port) { p_transmitterPort = port; };

	/// <summary>
	/// Get transmitter port
	/// </summary>
	/// <return>Port used by transmitter</return>
	int getPort() { return p_transmitterPort; };

	/// <summary>
	/// Sets target address
	/// </summary>
	/// <param name="new_address">Setsnew address</param>
	void setConnectAddress(const char *new_address) { 
		if (p_clientHostName != NULL)
			free(p_clientHostName);
		p_clientHostName = _strdup(new_address);
	};

	/// <summary>
	/// Gets target address
	/// </summary>
	/// <return>host name that we will connect to</return>
	char* getConnectAddress() { return p_clientHostName; };

private:
	// Constant Definitions

	// Default file format for output
	const char *c_FBXBinaryFileDesc = "FBX binary(*.fbx)";

	// Object Variables

	// Default SDK manager
	FbxManager *p_sdkManager;

	// Defines whether server mode has been enabled
	std::atomic_bool p_serverMode;

	// Transmitter port ( used by both client and server )
	int p_transmitterPort;

	// Nome of client for which host connects to
	char *p_clientHostName;


	// Private methods

	/// <summary>
	/// Creates a listening socket.
	/// </summary>
	/// <return>On success, returns socket identifier. On error, it returns an INVALID_SOCKET</return>
	SOCKET createDefaultListeningSocket();
};