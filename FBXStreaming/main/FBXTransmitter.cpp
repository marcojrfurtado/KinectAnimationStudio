#include "FBXTransmitter.h"
#include "FBXJointConverter.h"
#include "keyFramePackets.h"

/// <summary>
/// Constructor
/// </summary>
/// <param name="fManager">Pointer to FBX SDK Manager</param>
/// <param name="tPort">Transmitter port, which server binds to, and client connects to</param>
/// <param name="cHostName">Nameof the host for which client will be connected to</param>
/// <param name="exportFileName">File where server writes information</param>
FBXTransmitter::FBXTransmitter(FbxManager *fManager, int tPort, char *cHostName, char *exportFileName) : p_sdkManager(fManager),
p_transmitterPort(tPort),
p_clientHostName(cHostName)
{
	// Initialize export file name
	p_exportFileName = _strdup(exportFileName);

	// Initialise winsock
	initSockets();

	// Initialize address structure
	updateSocketAddr();


}

/// <summary>
/// Destructor
/// </summary>
FBXTransmitter::~FBXTransmitter() {
	// Destroy host name
	if (p_clientHostName != NULL)
		free(p_clientHostName);


	// Clean up sockets
	WSACleanup();
}
/// <summary>
/// Initialize sockets structure
/// </summary>
void FBXTransmitter::initSockets() {

	// Declare and initialize variables
	WSADATA wsaData;



	// Initialize Winsock
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		UI_Printf("failed to initialize winsock.\n");
		return;
	}
	UI_Printf("Winsock initialised.\n");
	// Initialize host name
	p_clientHostName = _strdup("127.0.0.1");

}

/// <summary>
/// Send file to the server
/// </summary>
/// <param name="inputFileName">MoCap file to be sent over the network</param>
void FBXTransmitter::transmit(char *inputFileName) {


	SOCKET s;

	//create socket
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		UI_Printf("socket() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}


	//start communication

	printf("Sending file: %s", inputFileName);

	/* initialize random seed: */
	//srand(1000);
	
	// Check if server mode is enabled, if so, do not transmit
	if (p_serverMode) {
		UI_Printf("WARNING: Server mode has been enabled, client mode is disabled.");
		return;
	}

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
		FbxNode *markerSet = FBXJointConverter::toAbsoluteMarkers(lScene, pNode);

		// Apply unroll filter to ir
		FbxAnimCurveFilterUnroll postProcFilter;
		applyUnrollFilterHierarchically(postProcFilter, markerSet);

		encodePacket(lScene, markerSet, s);
		// Save Scene
	//	int lFileFormat = p_sdkManager->GetIOPluginRegistry()->FindReaderIDByDescription(c_FBXBinaryFileDesc);
	//	SaveScene(p_sdkManager, lScene, "output.fbx", lFileFormat, false);

		//FbxAnimStack *animStack = lScene->GetCurrentAnimationStack();
		//FbxAnimLayer *animLayer = animStack->GetMember<FbxAnimLayer>();

		//FbxAnimCurve *rootCurveX = pNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);

		//std::vector<FbxTime> keyTimeVec;
		//FBXJointConverter::extractKeyTimesFromCurve(rootCurveX, keyTimeVec);


		// Back to skeleton
		//FbxNode *skel2 = FBXJointConverter::fromAbsoluteMarkers(lScene, pNode, "Bip02", keyTimeVec);


		//dropKeys(lScene, skel2);

	}

	/*
	// Save Scene
	int lFileFormat = p_sdkManager->GetIOPluginRegistry()->FindReaderIDByDescription(c_FBXBinaryFileDesc);
	
	r = SaveScene(p_sdkManager, lScene, "output.fbx", lFileFormat,false);
	if (!r) {
		UI_Printf(" Problem when trying to save scene.");
	}
	*/
	lScene->Destroy();

	// Close socket after transmitting
	closesocket(s);
	
}

/// <summary>
/// Start Server. Client mode will not work
/// </summary>
void FBXTransmitter::startServer() {

	// Result struct
	struct addrinfo *result = NULL;
	
	// enable flag
	p_serverMode = true;


	// Try loading export file
	FbxScene* lScene = FbxScene::Create(p_sdkManager, "");
	// Load File
	bool r = LoadScene(p_sdkManager, lScene, p_exportFileName);

	// Check if load was succesful
	if (!r) {
		UI_Printf("	Problem found when trying to load the scene. Make sure %s is a valid FBX file", p_exportFileName);
		lScene->Destroy();
		p_serverMode = false;
		return;
	}
	
	//Find marker set in the scene
	FbxNode *set = FBXJointConverter::findAnyMarkerSet(lScene);

	if (set == NULL) {
		UI_Printf(" Unable to find marker set in the export file.");
		lScene->Destroy();
		p_serverMode = false;
		return;
	}

	std::map<FbxUInt64, FbxNode *> decodeMap;
	initializeJointIdMap(set, decodeMap);


	/*SOCKET ListenSocket = createDefaultListeningSocket();
	if (ListenSocket == INVALID_SOCKET) {
		p_serverMode = false;
		return;
	}

	// Incoming client socket
	SOCKET ClientSocket = INVALID_SOCKET;


	// Accept client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return ;
	}*/


	// UDP Server
	SOCKET s;
	struct sockaddr_in server, si_other;
	int slen, recv_byte_len;
	PACKET buf[PACKET_SIZE];

	slen = sizeof(si_other);

	//Create a socket
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		UI_Printf("Could not create socket : %d", WSAGetLastError());
	}
	UI_Printf("Socket created.\n");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(p_transmitterPort);

	//Bind
	if (bind(s, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
	{
		UI_Printf("Bind failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	UI_Printf("Bind done\n");

	//keep listening for data

	while (true) {

		UI_Printf("Waiting for file...");
		fflush(stdout);

		//clear the buffer by filling null, it might have previously received data
		memset(buf, '\0', PACKET_SIZE);

		//try to receive some data, this is a blocking call
		if ((recv_byte_len = recvfrom(s, (char *)buf, PACKET_SIZE, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
		{
			UI_Printf("recvfrom() failed with error code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}

		UI_Printf("bytes recieved, %d", recv_byte_len);
		// Calculate number of keyframes within packet
		int num_key_received = recv_byte_len / sizeof(PACKET);


		// Decode incoming packet
		decodePacket(lScene, decodeMap, buf, num_key_received);
	}

	// Save Scene
	int lFileFormat = p_sdkManager->GetIOPluginRegistry()->FindReaderIDByDescription(c_FBXBinaryFileDesc);

	r = SaveScene(p_sdkManager, lScene, "output.fbx", lFileFormat, false);
	if (!r) {
		UI_Printf(" Problem when trying to save scene.");
	}

	lScene->Destroy();


	closesocket(s);

	
}



/// <summary>
/// Creates a listening socket.
/// </summary>
/// <return>On success, returns copy of socket. On error, it returns an INVALID_SOCKET</return>
SOCKET FBXTransmitter::createDefaultListeningSocket() {
	SOCKET s = INVALID_SOCKET;
	
	
	struct addrinfo *result = NULL,  hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	char portStr[10];
	sprintf_s(portStr, "%d", p_transmitterPort);
	// Resolve the local address and port to be used by the server
	int iResult = getaddrinfo(NULL, portStr, &hints, &result);
	if (iResult != 0) {
		UI_Printf("Failed to obtain binding address information");
		WSACleanup();
		return s;
	}

	// Create a SOCKET for the server to listen for client connections
	s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (s == INVALID_SOCKET) {
		UI_Printf("Problem while trying to create listening socket. Server will not be started. Error code is %d", WSAGetLastError());
		WSACleanup();
		return s;
	}


	// Bind to socket
	iResult = bind(s, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		UI_Printf("Binding to socket failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(s);
		WSACleanup();
		return INVALID_SOCKET;
	}

	iResult = listen(s, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		UI_Printf("Listen failed with error: %d\n", WSAGetLastError());
		closesocket(s);
		WSACleanup();
		return INVALID_SOCKET;
	}

	UI_Printf("Listening Socket has been succesfully created at port %d", p_transmitterPort);

	// free result
	freeaddrinfo(result);

	return s;
};

void FBXTransmitter::dropKeys(FbxScene *lScene, FbxNode *curNode) {

	// Lose 60 percent
	int lossRate = 9;


	FbxAnimStack *animStack = lScene->GetCurrentAnimationStack();
	FbxAnimLayer *animLayer = animStack->GetMember<FbxAnimLayer>();


	// Obtain translation curves
	FbxAnimCurve *transXCurve = curNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
	FbxAnimCurve *transYCurve = curNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
	FbxAnimCurve *transZCurve = curNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);

	int translationKeyCount = 0;
	// Ignore curves that have no or a single key
	if (transXCurve && transXCurve->KeyGetCount() > 1) {
		translationKeyCount = transXCurve->KeyGetCount();
	}

	// Iterate on translation keys
	for (int j = 1; j < translationKeyCount; j++) {
		int random = rand() % 10;
		// Drop packet
	//	UI_Printf("Number %d", random);
		if (random < lossRate) {
	//		UI_Printf("Dropping translation key at index %d for marker %d", j, i);

			if (transXCurve) {
				transXCurve->KeyModifyBegin();
				//Let's drop key from x curve
				transXCurve->KeyRemove(j);
				transXCurve->KeyModifyEnd();
			}

			if (transYCurve) {
				transYCurve->KeyModifyBegin();
				//Let's drop key from y curve
				transYCurve->KeyRemove(j);
				transYCurve->KeyModifyEnd();
			}

			if (transZCurve) {
				transZCurve->KeyModifyBegin();
				//Let's drop key from z curve
				transZCurve->KeyRemove(j);
				transZCurve->KeyModifyEnd();
			}
		}
	}


	// Obtain rotation curves
	FbxAnimCurve *rotXCurve = curNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
	FbxAnimCurve *rotYCurve = curNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
	FbxAnimCurve *rotZCurve = curNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);

	// Obtain number of keys
	int rotationKeyCount = 0;
	if (rotXCurve) {
		rotationKeyCount = rotXCurve->KeyGetCount();
	}

	// Iterate on rotation keys
	for (int j = 1; j < rotationKeyCount; j++) {
		int random = rand() % 10;
		// Drop packet
	//	UI_Printf("Number %d", random);
		if (random < lossRate) {
//			UI_Printf("Dropping rotation key at index %d for marker %d", j, i);


			if (rotXCurve) {
				rotXCurve->KeyModifyBegin();
				//Let's drop key from x curve
				rotXCurve->KeyRemove(j);
				rotXCurve->KeyModifyEnd();
			}

			if (transYCurve) {
				rotYCurve->KeyModifyBegin();
				//Let's drop key from y curve
				rotYCurve->KeyRemove(j);
				rotYCurve->KeyModifyEnd();
			}

			if (rotZCurve) {
				rotZCurve->KeyModifyBegin();
				//Let's drop key from z curve
				rotZCurve->KeyRemove(j);
				rotZCurve->KeyModifyEnd();
			}
		}
	}



	// Get number of markers
	int childCount = curNode->GetChildCount();

	// Iterate on markers
	for (int i = 0; i < childCount; i++) {

		// Current marker
		FbxNode *childI = curNode->GetChild(i);

		// Repeat for child
		dropKeys(lScene, childI);
	}


}


void FBXTransmitter::encodePacket(FbxScene *lScene, FbxNode *markerSet, SOCKET s) {
	int Max_key_num = PACKET_SIZE / sizeof(PACKET);
	PACKET *p = new PACKET[Max_key_num];
	int pIndex = 0;
	int keyI = 0;

	FbxAnimStack *animStack = lScene->GetCurrentAnimationStack();
	FbxAnimLayer *animLayer = animStack->GetMember<FbxAnimLayer>();

	int keyTotal = getKeyCount(markerSet->GetChild(0), lScene);

	int childCount = markerSet->GetChildCount();

	while (keyI < keyTotal) {
		for (int ci = 0; ci < childCount; ci++) {
			// Obtain rotation curves
			FbxAnimCurve *rotXCurve = markerSet->GetChild(ci)->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
			FbxAnimCurve *rotYCurve = markerSet->GetChild(ci)->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
			FbxAnimCurve *rotZCurve = markerSet->GetChild(ci)->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);
			p[pIndex].joint_id = getCustomIdProperty(markerSet->GetChild(ci));
			if (rotXCurve) {
				p[pIndex].x = rotXCurve->KeyGet(keyI).GetValue();
			}
			if (rotYCurve) {
				p[pIndex].y = rotYCurve->KeyGet(keyI).GetValue();
			}
			if (rotZCurve) {
				p[pIndex].z = rotZCurve->KeyGet(keyI).GetValue();
				p[pIndex].time = rotZCurve->KeyGet(keyI).GetTime().GetMilliSeconds();
			}
			pIndex++;
					
			// Send data and clear the buffer
			if (pIndex >= Max_key_num) {
				if (sendto(s, (const char *)p, Max_key_num*sizeof(PACKET), 0, (struct sockaddr *) &si_other, sizeof(si_other)) == SOCKET_ERROR)
				{
					UI_Printf("failed to send with error code : %d", WSAGetLastError());
					//exit(EXIT_FAILURE);
				}
				memset(p,NULL,Max_key_num);
				pIndex = 0;
			}
		}
		keyI++;
	}

	if (sendto(s, (const char *) p, pIndex*sizeof(PACKET), 0, (struct sockaddr *) &si_other, sizeof(si_other)) == SOCKET_ERROR)
	{
		UI_Printf("failed to send with error code : %d", WSAGetLastError());
		//exit(EXIT_FAILURE);
	}

	if (sendto(s, NULL, 0, 0, (struct sockaddr *) &si_other, sizeof(si_other)) == SOCKET_ERROR)
	{
		UI_Printf("failed to send with error code : %d", WSAGetLastError());
		//exit(EXIT_FAILURE);
	}
	delete p;
}

void FBXTransmitter::updateSocketAddr() {
	//setup address structure
	memset((char *)&si_other, 0, sizeof(si_other));
	//int result;
	//char name[20];
	//result = InetPton(AF_INET, p_clientHostName, &si_other.sin_addr);
	//InetNtop(AF_INET, &si_other.sin_addr, name, 20);
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(p_transmitterPort);
	//si_other.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	InetPton(AF_INET, p_clientHostName, &si_other.sin_addr);
}


/// <summary>
/// Decode keyframes from a certain packet
/// </summary>
void FBXTransmitter::decodePacket(FbxScene *lScene, std::map<FbxUInt64,FbxNode *> jointMap, PACKET *p, int keyframeNum) {


	// Retrieve animationlayer
	FbxAnimStack *animStack = lScene->GetCurrentAnimationStack();
	FbxAnimLayer *animLayer = animStack->GetMember<FbxAnimLayer>();
	


	// Iterate on incoming packets
	for (int i = 0; i < keyframeNum; i++) {

		auto it = jointMap.find(p[i].joint_id);
		if (it == jointMap.end()){
			UI_Printf(" Decoding error. Unable to find node with id %d in the scene.", p[i].joint_id);
			continue;
		}

		FbxNode *tgtMarker = it->second;

		FbxAnimCurve *rotCurveX = tgtMarker->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
		FbxAnimCurve *rotCurveY = tgtMarker->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
		FbxAnimCurve *rotCurveZ = tgtMarker->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);

		if (rotCurveX) {
			rotCurveX->KeyModifyBegin();

			// Start adding key
			// TODO: Speed up this operation using index
			int keyIndex = rotCurveX->KeyAdd(p[i].time);
			rotCurveX->KeySetValue(keyIndex, p[i].x);

			rotCurveX->KeyModifyEnd();
		}

		if (rotCurveY) {
			rotCurveY->KeyModifyBegin();

			// Start adding key
			// TODO: Speed up this operation using index
			int keyIndex = rotCurveY->KeyAdd(p[i].time);
			rotCurveY->KeySetValue(keyIndex, p[i].y);

			rotCurveY->KeyModifyEnd();
		}

		if (rotCurveZ) {
			rotCurveZ->KeyModifyBegin();

			// Start adding key
			// TODO: Speed up this operation using index
			int keyIndex = rotCurveZ->KeyAdd(p[i].time);
			rotCurveZ->KeySetValue(keyIndex, p[i].z);

			rotCurveZ->KeyModifyEnd();
		}


	}


}


/// <summary>
/// Creates a map the relates node pointsers and their IDs
/// </summary>
void FBXTransmitter::initializeJointIdMap(FbxNode *parentNode, std::map<FbxUInt64, FbxNode *> &idMap) {

	int childCount = parentNode->GetChildCount();

	for (int i = 0; i < childCount; i++) {
		FbxNode* childI = parentNode->GetChild(i);
		idMap[getCustomIdProperty(childI)] = childI;
	}

}