#include "FBXCoding.h"


/// <summary>
/// Constructor
/// </summary>
/// <param name="enableInterleaving">Interleave packets before sending</param>
/// <param name="latency">Interleaving latency window</param>
FBXCoding::FBXCoding(bool enableInterleaving, int latency, bool enableLDPC, int ldpc_offset) :
p_isInterleavingMode(enableInterleaving),
p_latencyWindow(latency), 
p_enableLDPC(enableLDPC),
p_LDPC_offset(ldpc_offset)
{ 
	ConfigFileParser &parser = ConfigFileParser::getInstance();
	std::string latentsize = parser.getParameter(LATENCY_WINDOW);
	std::string interleave = parser.getParameter(ENABLE_INTERLEAVING);
	std::string ldpc = parser.getParameter(ENABLE_LDPC);
	std::string offset = parser.getParameter(LDPC_OFFSET);

	//std::string global = parser.getParameter(ENABLE_GLOBAL_TRANSFORMATION);

	if (latentsize.compare("ERROR") != 0) {
		p_latencyWindow = atoi(latentsize.c_str());
	}

	if (interleave.compare("ERROR") != 0) {
		std::istringstream(interleave) >> p_isInterleavingMode;
	}

	if (ldpc.compare("ERROR") != 0) {
		std::istringstream(ldpc) >> p_enableLDPC;
		
		// it++ construsts Parity Matrix
		H.generate(104, 2, 26, "rand", "104 8");

		// it++ constructs Generator Matrix;
		G.construct(&H);
		//C.save_code("ldpc_encode.codec");
	}

	if (offset.compare("ERROR") != 0) {
		p_LDPC_offset = atoi(offset.c_str());
	}

}


/// <summary>
/// Destructor
/// </summary>
FBXCoding::~FBXCoding() { }

void FBXCoding::encodeAnimation(FbxScene *lScene, FbxNode *markerSet, SOCKET s) {
	int Max_key_num;
	PACKET *p;
	size_t fragmentSize;

	if (p_enableLDPC) {
		// Maximum number of keys a package can store
		Max_key_num = PACKET_SIZE / sizeof(PACKET_LDPC);
		fragmentSize = sizeof(PACKET_LDPC);
		p = new PACKET_LDPC[Max_key_num];
	}
	else {
		// Maximum number of keys a package can store
		Max_key_num = PACKET_SIZE / sizeof(PACKET);
		fragmentSize = sizeof(PACKET);
		p = new PACKET[Max_key_num];
	}

	int pIndex = 0;

	// Vector used to store key indexes
	std::vector<int> keyIvec;
	keyIvec.reserve(p_latencyWindow);

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
		if ((((float)(keyI - encodedKeyCount)) / keyTotal) > 0.2) {
			encodedKeyCount = keyI;
			UI_Printf("%f%% of the keys have been encoded.", (((float)encodedKeyCount) / keyTotal) * 100);
		}
		
		// If vector is the requested size of latency window then shuffle it. 
		// If not add it to the vector keyIvec
		if (keyIvec.size() != p_latencyWindow) {
			keyIvec.push_back(keyI);
			//UI_Printf("%d has been added to the vector", keyI);
		}
		if (keyIvec.size() == p_latencyWindow || keyI == (keyTotal-1)) {
			
			// If Interleaving is enabled shuffle vector
			if (p_isInterleavingMode){
				std::random_shuffle(keyIvec.begin(), keyIvec.end());
			}
			for (std::vector<int>::iterator it = keyIvec.begin(); it != keyIvec.end(); ++it) {
				//UI_Printf("now encoding key: %d", *it);
				for (int ci = 0; ci < childCount; ci++) {
					int oldPIndex = pIndex;

					// Encode rotation curves
					pIndex = encodeKeyFrame(animLayer, markerSet->GetChild(ci), *it, p, pIndex, s, false);
					if (oldPIndex == (Max_key_num - 1) && pIndex == 0) // packet has just been sent out
						packetSentCount++;

					oldPIndex = pIndex;
					// Encode translation curves - if they exist
					pIndex = encodeKeyFrame(animLayer, markerSet->GetChild(ci), *it, p, pIndex, s, true);
					if (oldPIndex == (Max_key_num - 1) && pIndex == 0)
						packetSentCount++;


					// Sleep for a while, before sending more
					// TODO - fix concurrency issues. Make decoder faster
					Sleep(5);
			
				}
				
			}

			keyIvec.clear();

		}
		keyI++;
	}

	Sleep(100);
	if (pIndex > 0) {
		if (sendto(s, (const char *)p, pIndex*fragmentSize, 0, (struct sockaddr *) &p_sock_addr, sizeof(p_sock_addr)) == SOCKET_ERROR)
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



}


/// <summary>
/// Decode keyframes from a certain packet
/// </summary>
void FBXCoding::decodePacket(FbxScene *lScene, std::map<short, FbxNode *> jointMap, PACKET *p, int keyframeNum) {


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
		}
		else {
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
int FBXCoding::encodeKeyFrame(FbxAnimLayer *animLayer, FbxNode *tgtNode, int keyIndex, PACKET *p, int pIndex, SOCKET s, bool isTranslation) {

	// Maximum number of keys a package can store
	int Max_key_num;
	int bytes_sent;
	if (p_enableLDPC) {
		Max_key_num = PACKET_SIZE / sizeof(PACKET_LDPC);
		bytes_sent = Max_key_num*sizeof(PACKET_LDPC);
	}
	else {
		Max_key_num = PACKET_SIZE / sizeof(PACKET);
		bytes_sent = Max_key_num*sizeof(PACKET);

	}

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


	if (isTranslation)
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
	
	if (p_enableLDPC){
		itpp::LDPC_Code ldpc_encode(&H, &G);
		itpp::bvec bitsin = itpp::randb(ldpc_encode.get_nvar() - ldpc_encode.get_ncheck());
		itpp::bvec bitsout;
		ldpc_encode.encode(bitsin, bitsout);
		bvec2Bitset(bitsout, (PACKET_LDPC*) p, pIndex);
		std::string bvecStringIn = itpp::to_str(bitsin);
		std::string bvecStringOut = itpp::to_str(bitsout);
		UI_Printf("bitsin: %x", bvecStringIn.c_str());
		UI_Printf("bitsout: %s", bvecStringOut.c_str());
		UI_Printf("Parity bits: %s", ((PACKET_LDPC *) p)[pIndex].bits.to_string().c_str());
	}
	
	// Increment index
	pIndex++;

	// Send data and clear the buffer
	if (pIndex >= Max_key_num) {
		if (sendto(s, (const char *)p, bytes_sent, 0, (struct sockaddr *) &p_sock_addr, sizeof(p_sock_addr)) == SOCKET_ERROR)
		{
			UI_Printf("failed to send with error code : %d", WSAGetLastError());
		}
		memset(p, NULL, Max_key_num);
		pIndex = 0;
	}

	// Return updated index
	return pIndex;
}

void FBXCoding::bvec2Bitset(itpp::bvec bin_list, PACKET_LDPC *p, int pIndex) {
	for (int i = 96; i < 104; i++) {
		p[pIndex].bits[i%8] = bin_list[i];
	}
}