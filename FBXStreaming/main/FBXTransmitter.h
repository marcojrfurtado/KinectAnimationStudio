#pragma once

#include "../common/stdafx.h"
#include "keyFramePackets.h"

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
	FBXTransmitter(FbxManager *fManager = NULL, int tPort = 27015, char *cHostName = NULL , char *exportFileName = "output.fbx");

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

	void createModelBaseFile();

private:
	// Constant Definitions

	// Default file format for output
	const char *c_FBXBinaryFileDesc = "FBX binary(*.fbx)";
	// Server timeout
	const int c_serverTimeoutSec = 5;
	const int c_serverTimeoutUSec = 0;

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

	// address to send to
	struct sockaddr_in p_sock_addr;

	PACKET p_serverbuf[(PACKET_SIZE / sizeof(PACKET)) + 1];


	// File where server will output keyframes
	char *p_exportFileName;

	// Name of import file
	char *p_importFileName;

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
	/// Encode keyframes from markers
	/// </summary>
	void encodeAnimation(FbxScene *lScene, FbxNode *markerSet, SOCKET s);

	/// <summary>
	/// Decode keyframes from a certain packet
	/// </summary>
	void decodePacket(FbxScene *lScene, std::map<short, FbxNode *> jointMap, PACKET *p, int keyframeNum);

	/// <summary>
	/// Update socket address structure
	/// </summary>
	void updateSocketAddr();

	/// <summary>
	/// Creates a map the relates node pointsers and their IDs
	/// </summary>
	void initializeJointIdMap(FbxNode *parentNode, std::map<short, FbxNode *> &idMap);

	/// <summary>
	/// Starts server as a background thread
	/// </summary>
	void backgroundListenServer();
	
	/// <summary>
	/// Encodes keys for curves from a given node
	/// </summary>
	/// <param name="animLayer">FBX Anim layer</param>
	/// <param name="tgtNode">Node to have curves extracted</param>
	/// <param name="p">Packet to be sent</param>
	/// <param name="pIndex">Index of current keyframe</param>
	/// <param name="s">Socket used to send packages</param>
	/// <param name="isTranslation">Are these translation curves?</param>
	/// <return>Updated pIndex</return>
	int encodeKeyFrame(FbxAnimLayer *animLayer, FbxNode *tgtNode, int keyIndex, PACKET *p, int pIndex, SOCKET s, bool isTranslation = false);



	/// <summary>
	/// Winsock 2 RECV function with timeout
	/// </summary>
	/// <param name="socket">Listening socket</param>
	/// <param name="sec">Wait time, in sec</param>
	/// <param name="usec">wait time, in millisecond</param>
	/// <return>0 on timeout, >1 on data ready, -1 (SOCKET_ERROR) on error</return>
	int recvfromTimeOutUDP(SOCKET socket, long sec, long usec);

};