#pragma once

#include "../common/stdafx.h"
#include "keyFramePackets.h"
#include "FBXCoding.h"

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
	/// <param name="exportFileName">File where server writes information</param>
	FBXTransmitter(FbxManager *fManager = NULL, int tPort = 27015, char *cHostName = NULL, char *exportFileName = "output.fbx");

	/// <summary>
	/// Destructor
	/// </summary>
	~FBXTransmitter();

	/// <summary>
	/// Initialize sockets structure
	/// </summary>
	void initWSock();

	/// <summary>
	/// Init FBX SDK manager
	/// </summary>
	/// <param name="fManager">Pointer to FBX SDK Manager</param>
	void initFBXSDKManager(FbxManager *fManager) { p_sdkManager = fManager; };

	/// <summary>
	/// Send import file to the server
	/// </summary>
	void transmit();

	/// <summary>
	/// Start Server. Client mode will not work
	/// </summary>
	void startServer();

	/// <summary>
	/// Sets transmitter port
	/// </summary>
	/// <param name="setPort">Sets port used by transmitter</param>
	void setPort(int port) {	
		p_transmitterPort = port; 		
		updateSocketAddr();
	};

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
		updateSocketAddr();
	};

	/// <summary>
	/// Gets target address
	/// </summary>
	/// <return>host name that we will connect to</return>
	char* getConnectAddress() { return p_clientHostName; };


	/// <summary>
	/// Sets export file name
	/// </summary>
	void setExportName(char *newName) {
		if (p_exportFileName != NULL)
			free(p_exportFileName);
		p_exportFileName = _strdup(newName);
	}

	/// <summary>
	/// Sets import file name
	/// </summary>
	void setImporttName(char *newName) {
		if (p_importFileName != NULL)
			free(p_importFileName);
		p_importFileName = _strdup(newName);
	}

	/// <summary>
	/// Creates FBX file containing a skeleton. Used to extract animation to.
	/// </summary>
	void createModelBaseFile();

	/// <summary>
	/// Prints details about initial configuration
	/// </summary>
	void printSetupDetails() {
		UI_Printf(" Transmitter Initialization: Global Transformation mode set to %d", p_globalTransformationMode);
		UI_Printf(" Transmitter Initialization: Enable Virtual Markers %d", p_enableVirtualMarkers);
		p_coding.printSetupDetails();
	}
private:
	// Constant Definitions

	// Default file format for output
	const char *c_FBXBinaryFileDesc = "FBX binary(*.fbx)";
	// Server timeout
	const int c_serverTimeoutSec = 15;
	const int c_serverTimeoutUSec = 0;

	const int c_defaultCodingLatency = 6;

	// Object Variables

	// Default SDK manager
	FbxManager *p_sdkManager;

	// Defines whether server mode has been enabled
	std::atomic_bool p_serverMode;
	// Defines whether we are transmitting a file
	std::atomic_bool p_isTransmitting;

	// Transmitter port ( used by both client and server )
	int p_transmitterPort;

	// Nome of client for which host connects to
	char *p_clientHostName;

	// True if Global Transformation is enabled.
	bool  p_globalTransformationMode;

	PACKET p_serverbuf[(PACKET_SIZE / sizeof(PACKET)) + 1];


	// File where server will output keyframes
	char *p_exportFileName;

	// Name of import file
	char *p_importFileName;

	// address to send to
	struct sockaddr_in p_sock_addr;


	FBXCoding p_coding;

	// Enable virutal Markers
	bool p_enableVirtualMarkers;

	// Private methods

	/// <summary>
	/// Creates a listening socket.
	/// </summary>
	/// <return>On success, returns socket identifier. On error, it returns an INVALID_SOCKET</return>
	SOCKET createDefaultListeningSocket();

	/// <summary>
	/// Simulates packet loss
	/// </summary>
	void dropKeys(FbxScene *lScene, FbxNode *mSet);

	/// <summary>
	/// Update socket address structure
	/// </summary>
	void updateSocketAddr();

	/// <summary>
	/// Starts server as a background thread
	/// </summary>
	void backgroundListenServer();


	/// <summary>
	/// Winsock 2 RECV function with timeout
	/// </summary>
	/// <param name="socket">Listening socket</param>
	/// <param name="sec">Wait time, in sec</param>
	/// <param name="usec">wait time, in millisecond</param>
	/// <return>0 on timeout, >1 on data ready, -1 (SOCKET_ERROR) on error</return>
	int recvfromTimeOutUDP(SOCKET socket, long sec, long usec);

};