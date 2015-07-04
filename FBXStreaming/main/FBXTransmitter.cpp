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
	initWSock();

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
/// Initialize WinSock
/// </summary>
void FBXTransmitter::initWSock() {

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
/// Send import file to the server
/// </summary>
void FBXTransmitter::transmit() {

	if (p_isTransmitting) {
		UI_Printf("Currently encoding a file, please try again later.");
		return;
	}

	// Start transmission mode
	p_isTransmitting = true;

	SOCKET s;

	//create socket
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		UI_Printf("socket() failed with error code : %d", WSAGetLastError());
		p_isTransmitting = false;
		return;
	}


	//start communication

	printf("Sending file: %s", p_importFileName);

	/* initialize random seed: */
	//srand(1000);
	
	// Check if server mode is enabled, if so, do not transmit
	if (p_serverMode) {
		UI_Printf("WARNING: Server mode has been enabled, client mode is disabled.");
		p_isTransmitting = false;
		return;
	}

	// Create a scene
	FbxScene* lScene = FbxScene::Create(p_sdkManager, "");
	
	// Load File
	bool r = LoadScene(p_sdkManager, lScene, p_importFileName);

	// Check if load was succesful
	if (!r) {
		UI_Printf("	Problem found when trying to load the scene. Make sure %s is a valid FBX file", p_importFileName);
		lScene->Destroy();
		p_isTransmitting = false;
		return;
	}

	UI_Printf(" File %s has been succesfully loaded.", p_importFileName);

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


		// Encode file on the background
		std::async(std::launch::async,
			[lScene,markerSet,s,this] {
			encodeAnimation(lScene, markerSet, s);
			return true;
		});

		// Just do it for the first skeleton found
		break;
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

	
}

/// <summary>
/// Start Server. Client mode will not work
/// </summary>
void FBXTransmitter::startServer() {
	
	if (p_serverMode) {
		UI_Printf("Server has already been started.");
		return;
	}

	if (p_isTransmitting) {
		UI_Printf("This instance is currently transmitting a file. Please try again another time.");
		return;
	}


	// enable flag
	p_serverMode = true;

	std::async(std::launch::async,
		[this] {
		backgroundListenServer();
		return true;
	});

	
}


void FBXTransmitter::backgroundListenServer() {
	// Try loading export file
	FbxScene* lScene = FbxScene::Create(p_sdkManager, "");
	// Load File
	bool r = LoadScene(p_sdkManager, lScene, p_importFileName);

	// Check if load was succesful
	if (!r) {
		UI_Printf("	Problem found when trying to load the scene. Make sure %s is a valid FBX file", p_importFileName);
		lScene->Destroy();
		p_serverMode = false;
		return;
	}

	FbxNode *skel = FBXJointConverter::findAnySkeleton(lScene);

	// Check if load was succesful
	if (!skel) {
		UI_Printf("No skeleton found");
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

	std::map<short, FbxNode *> decodeMap;
	initializeJointIdMap(set, decodeMap);

	// UDP Server
	SOCKET s;
	struct sockaddr_in server;
	struct sockaddr  s_add_incoming;
	int recv_byte_len, s_add_incoming_len = sizeof(s_add_incoming);
	const int buf_len = PACKET_SIZE / sizeof(PACKET);
	PACKET recv_intermediate_buf[buf_len+1];

	std::mutex *bufferMutex = new std::mutex;
	std::mutex *decodeMutex = new std::mutex;

	// Count number of packets received
	int packetCount = 0;
	
	// Result form timeout recv
	int timeoutRecvReturn;
	

	//Create a socket
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		UI_Printf("Could not create socket : %d", WSAGetLastError());
		lScene->Destroy();
		p_serverMode = false;
		return;
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
		lScene->Destroy();
		closesocket(s);
		p_serverMode = false;
		return;
	}
	UI_Printf("Bind done\n");

	std::vector<std::future<bool>> futures;

	while (true) {

		fflush(stdout);


		//try to receive some data, this is a blocking call
		memset(recv_intermediate_buf, '\0', buf_len+1);

		// Only start checking timeout once first packet has been decoded
		if (packetCount > 0) {
			timeoutRecvReturn = recvfromTimeOutUDP(s, c_serverTimeoutSec, c_serverTimeoutUSec);
			if (timeoutRecvReturn == SOCKET_ERROR) {
				UI_Printf("select() failed with error code : %d", WSAGetLastError());
				lScene->Destroy();
				closesocket(s);
				p_serverMode = false;
				return;
			}
			else if (timeoutRecvReturn == 0) {
				// Stop listening, last packet was probably dropped
				break;
			}
		}

		// Receive packet
		if ((recv_byte_len = recvfrom(s, (char *)recv_intermediate_buf, buf_len * sizeof(PACKET), 0, (struct sockaddr *) &s_add_incoming, &s_add_incoming_len)) == SOCKET_ERROR)
		{
			UI_Printf("recvfrom() failed with error code : %d", WSAGetLastError());
			lScene->Destroy();
			closesocket(s);
			p_serverMode = false;
			return;
		}

		

		//clear the buffer by filling null, it might have previously received data
		{
			std::unique_lock<std::mutex> lock(*bufferMutex);
			//memset(p_serverbuf, '\0', buf_len+1);
			memcpy(p_serverbuf, recv_intermediate_buf, recv_byte_len);
		}


		// One more packet has been received
		packetCount++;

		// When receiving a Datagram of size 0, we finish the transmission
		if (recv_byte_len == 0)
			break;

		//auto task = 
		futures.push_back(
			std::async(std::launch::async,
				[recv_byte_len, lScene, decodeMap, bufferMutex, buf_len, decodeMutex, this] {
				// Calculate number of keyframes within packet
				const int num_key_received = recv_byte_len / sizeof(PACKET);
				PACKET *decodeBuf = new PACKET[buf_len+1];
			
				{
					std::unique_lock<std::mutex> lock(*bufferMutex);
					memcpy(decodeBuf, p_serverbuf, recv_byte_len);
				}
			
				// Decode incoming packet
				{
					std::unique_lock<std::mutex> lock(*decodeMutex);
					decodePacket(lScene, decodeMap, decodeBuf, num_key_received);
				}
				delete decodeBuf;
				return true;
			})
		);

		//futures.push_back(task);

	}
	
	for (auto &it:futures) {
		it.wait();
	}
	
	delete bufferMutex;
	delete decodeMutex;

	UI_Printf("Decode has finished. %d packets were received during transmission",packetCount);
	UI_Printf("Ready to convert data to a hierarchical skeleton.");

	FBXJointConverter::fromAbsoluteMarkers(lScene, skel, (char *) skel->GetName());

	UI_Printf("Data has been converted.");
	// Save Scene
	int lFileFormat = p_sdkManager->GetIOPluginRegistry()->FindReaderIDByDescription(c_FBXBinaryFileDesc);

	r = SaveScene(p_sdkManager, lScene, p_exportFileName, lFileFormat, false);
	if (!r) {
		UI_Printf(" Problem when trying to save scene.");
	}
	else {
		UI_Printf("Scene has been succesfully saved to %s", p_exportFileName);
	}

	lScene->Destroy();
	closesocket(s);

	p_serverMode = false;
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
		if (random < lossRate) {

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


void FBXTransmitter::encodeAnimation(FbxScene *lScene, FbxNode *markerSet, SOCKET s) {
	// Maximum number of keys a package can store
	const int Max_key_num = PACKET_SIZE / sizeof(PACKET);

	PACKET *p = new PACKET[Max_key_num];
	int pIndex = 0;

	// We are ignoring the key at index 0, as this
	int keyI = 1;

	// Number of packets sent
	int packetSentCount = 0;

	FbxAnimStack *animStack = lScene->GetCurrentAnimationStack();
	FbxAnimLayer *animLayer = animStack->GetMember<FbxAnimLayer>();

	int keyTotal = getKeyCount(markerSet->GetChild(0), lScene);

	int childCount = markerSet->GetChildCount();

	// Variable used to alert the user about how many keys have already been processed
	int encodedKeyCount = keyI;
	while (keyI < keyTotal) {
		if (( ( (float) (keyI - encodedKeyCount)) / keyTotal) > 0.2) {
			encodedKeyCount = keyI;
			UI_Printf("%f%% of the keys have been encoded.", (((float)encodedKeyCount) / keyTotal) * 100);
		}

		for (int ci = 0; ci < childCount; ci++) {
			int oldPIndex = pIndex;

			// Encode rotation curves
			pIndex = encodeKeyFrame(animLayer, markerSet->GetChild(ci), keyI, p, pIndex, s, false);
			if (oldPIndex == (Max_key_num-1) && pIndex == 0) // packet has just been sent out
				packetSentCount++;
			
			oldPIndex = pIndex;
			// Encode translation curves - if they exist
			pIndex = encodeKeyFrame(animLayer, markerSet->GetChild(ci), keyI, p, pIndex, s, true);
			if (oldPIndex == (Max_key_num-1) && pIndex == 0)
				packetSentCount++;

			// Sleep for a while, before sending more
			// TODO - fix concurrency issues. Make decoder faster
			Sleep(5);
		}
		keyI++;
	}

	Sleep(100);
	if (pIndex > 0) {
		if (sendto(s, (const char *)p, pIndex*sizeof(PACKET), 0, (struct sockaddr *) &p_sock_addr, sizeof(p_sock_addr)) == SOCKET_ERROR)
		{
			UI_Printf("failed to send with error code : %d", WSAGetLastError());
		}
		packetSentCount++;
	}


	if (sendto(s, NULL, 0, 0, (struct sockaddr *) &p_sock_addr, sizeof(p_sock_addr)) == SOCKET_ERROR)
	{
		UI_Printf("failed to send with error code : %d", WSAGetLastError());
	}
	packetSentCount++;

	UI_Printf("Encoding has finished. %d packages were sent.", packetSentCount);

	delete p;

	// Destroy scene
	lScene->Destroy();

	// Close socket after transmitting
	closesocket(s);

	// Disable transmission mode ( this enables other files to be transmitted )
	p_isTransmitting = false;
}


/// <summary>
/// Encodes keys for curves from a given node at a given time
/// </summary>
/// <param name="animLayer">FBX Anim layer</param>
/// <param name="tgtNode">Node to have curves extracted</param>
/// <param name="keyIndex">Index of </param>
/// <param name="zCurve">Z curve</param>
/// <param name="p">Packet to be sent</param>
/// <param name="pIndex">Index of current keyframe</param>
/// <param name="s">Socket used to send packages</param>
/// <param name="isTranslation">Are these translation curves?</param>
/// <return>Updated pIndex</return>
int FBXTransmitter::encodeKeyFrame(FbxAnimLayer *animLayer, FbxNode *tgtNode, int keyIndex ,PACKET *p, int pIndex, SOCKET s, bool isTranslation) {

	// Maximum number of keys a package can store
	const int Max_key_num = PACKET_SIZE / sizeof(PACKET);


	FbxAnimCurve *xCurve;
	FbxAnimCurve *yCurve; 
	FbxAnimCurve *zCurve;

	if (isTranslation) {
		xCurve = tgtNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
		yCurve = tgtNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
		zCurve = tgtNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);
	}
	else {
		xCurve = tgtNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
		yCurve = tgtNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
		zCurve = tgtNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);
	}

	// Nothing to be processed, just leave
	if (!xCurve && !yCurve && !zCurve)
		return pIndex;

	
	if ( isTranslation)
		p[pIndex].joint_id = TRANSLATION_CUSTOM_ID;
	else
		p[pIndex].joint_id = getCustomIdProperty(tgtNode);

	if (xCurve) {
		p[pIndex].x = xCurve->KeyGet(keyIndex).GetValue();
		p[pIndex].time = zCurve->KeyGet(keyIndex).GetTime().GetMilliSeconds();
	}
	if (yCurve) {
		p[pIndex].y = yCurve->KeyGet(keyIndex).GetValue();
		p[pIndex].time = zCurve->KeyGet(keyIndex).GetTime().GetMilliSeconds();

	}
	if (zCurve) {
		p[pIndex].z = zCurve->KeyGet(keyIndex).GetValue();
		p[pIndex].time = zCurve->KeyGet(keyIndex).GetTime().GetMilliSeconds();
	}

	// Increment index
	pIndex++;

	// Send data and clear the buffer
	if (pIndex >= Max_key_num) {
		if (sendto(s, (const char *)p, Max_key_num*sizeof(PACKET), 0, (struct sockaddr *) &p_sock_addr, sizeof(p_sock_addr)) == SOCKET_ERROR)
		{
			UI_Printf("failed to send with error code : %d", WSAGetLastError());
		}
		memset(p, NULL, Max_key_num);
		pIndex = 0;
	}

	// Return updated index
	return pIndex;
}

void FBXTransmitter::updateSocketAddr() {
	//setup address structure
	memset((char *)&p_sock_addr, 0, sizeof(p_sock_addr));
	//int result;
	//char name[20];
	//result = InetPton(AF_INET, p_clientHostName, &si_other.sin_addr);
	//InetNtop(AF_INET, &si_other.sin_addr, name, 20);
	p_sock_addr.sin_family = AF_INET;
	p_sock_addr.sin_port = htons(p_transmitterPort);
	//si_other.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	InetPton(AF_INET, p_clientHostName, &p_sock_addr.sin_addr);
}


/// <summary>
/// Decode keyframes from a certain packet
/// </summary>
void FBXTransmitter::decodePacket(FbxScene *lScene, std::map<short, FbxNode *> jointMap, PACKET *p, int keyframeNum) {


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

		FbxAnimCurve *curveX, *curveY, *curveZ;
		FbxAnimCurveDef::EInterpolationType interpType;

		// Check what type of curve we received
		if (p[i].joint_id == TRANSLATION_CUSTOM_ID) {
			curveX = tgtMarker->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
			curveY = tgtMarker->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
			curveZ = tgtMarker->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);
			interpType = FbxAnimCurveDef::eInterpolationLinear;
		} else {
			curveX = tgtMarker->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
			curveY = tgtMarker->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
			curveZ = tgtMarker->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);
			interpType = FbxAnimCurveDef::eInterpolationCubic;
		}


		FbxTime kTime; 
		kTime.SetMilliSeconds(p[i].time);

		if (curveX) {
			curveX->KeyModifyBegin();

			// Start adding key
			// TODO: Speed up this operation using index
			int keyIndex = curveX->KeyInsert(kTime);
			curveX->KeySetValue(keyIndex, p[i].x);
			curveX->KeySetInterpolation(keyIndex, interpType);

			curveX->KeyModifyEnd();
		}

		if (curveY) {
			curveY->KeyModifyBegin();

			// Start adding key
			// TODO: Speed up this operation using index
			int keyIndex = curveY->KeyInsert(kTime);
			curveY->KeySetValue(keyIndex, p[i].y);
			curveY->KeySetInterpolation(keyIndex, interpType);

			curveY->KeyModifyEnd();
		}

		if (curveZ) {
			curveZ->KeyModifyBegin();

			// Start adding key
			// TODO: Speed up this operation using index
			int keyIndex = curveZ->KeyInsert(kTime);
			curveZ->KeySetValue(keyIndex, p[i].z);
			curveZ->KeySetInterpolation(keyIndex, interpType);

			curveZ->KeyModifyEnd();
		}


	}


}


/// <summary>
/// Creates a map the relates node pointsers and their IDs
/// </summary>
void FBXTransmitter::initializeJointIdMap(FbxNode *parentNode, std::map<short, FbxNode *> &idMap) {

	int childCount = parentNode->GetChildCount();

	for (int i = 0; i < childCount; i++) {
		FbxNode* childI = parentNode->GetChild(i);
		idMap[getCustomIdProperty(childI)] = childI;

		// Check if child has any translation curve
		if (childI->LclTranslation.IsAnimated())
			idMap[TRANSLATION_CUSTOM_ID] = childI;
	}

}


void FBXTransmitter::createModelBaseFile() {

	if (p_isTransmitting || p_serverMode) {
		UI_Printf("Base model files cannot be created when server or client are working.");
		return;
	}


	// Try loading export file
	FbxScene* lScene = FbxScene::Create(p_sdkManager, "");
	// Load File
	bool r = LoadScene(p_sdkManager, lScene, p_importFileName);

	// Check if load was succesful
	if (!r) {
		UI_Printf("Problem found when trying to load the scene. Make sure %s is a valid FBX file", p_importFileName);
		lScene->Destroy();
		p_serverMode = false;
		return;
	}

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
		FbxNode *markerSet = FBXJointConverter::toAbsoluteMarkers(lScene, pNode, true);

		break;
	}


	// Save Scene
	int lFileFormat = p_sdkManager->GetIOPluginRegistry()->FindReaderIDByDescription(c_FBXBinaryFileDesc);

	r = SaveScene(p_sdkManager, lScene, p_exportFileName, lFileFormat, false);
	if (!r) {
		UI_Printf(" Problem when trying to save scene.");
	}

	UI_Printf("Model file has been created.");

}


/// <summary>
/// Winsock 2 RECV function with timeout
/// </summary>
/// <param name="socket">Listening socket</param>
/// <param name="sec">Wait time, in sec</param>
/// <param name="usec">wait time, in millisecond</param>
/// <return>0 on timeout, >1 on data ready, -1 (SOCKET_ERROR) on error</return>
int FBXTransmitter::recvfromTimeOutUDP(SOCKET socket, long sec, long usec) {
	// Setup timeval variable

	timeval timeout;
	timeout.tv_sec = sec;
	timeout.tv_usec = usec;

	// Setup fd_set structure
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(socket, &fds);

	// Return value:
	// -1: error occurred
	// 0: timed out
	// > 0: data ready to be read
	return select(0, &fds, 0, 0, &timeout);
}