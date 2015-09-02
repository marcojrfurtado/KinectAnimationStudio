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
p_LDPC_offset(ldpc_offset),
p_fps(0)
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

		if (p_enableLDPC) {
			char parityPath[512];
			GetLocalFile(c_defaultParityFileName, parityPath, 512);
			
			// Check if file exists
			if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(parityPath) && GetLastError() == ERROR_FILE_NOT_FOUND) {
				// If it does not exist, let us generate parity
				// it++ construsts Parity Matrix
				itpp::RNG_reset(4);
				const int ratio_weight = 1;
				itpp::LDPC_Parity_Regular tempP(N_DATA_BIT + N_PARITY_BIT, ratio_weight*1, ratio_weight* ( float(N_DATA_BIT + N_PARITY_BIT)/N_PARITY_BIT ), "rand", "200 6");
				//H.generate(N_DATA_BIT + N_PARITY_BIT, 2, 14, "rand", "200 6");
				tempP.cycle_removal_MGW(6);
				tempP.save_alist(parityPath);

			}

			// Force RNG to be seeeded to common value before creating G
			itpp::RNG_reset(0);

			// FIX: ITPP generated matrix is not same as saved/loaded
			H.load_alist(parityPath);

			// it++ constructs Generator Matrix;
			G.construct(&H);
			
		}
	}

	if (offset.compare("ERROR") != 0) {
		p_LDPC_offset = atoi(offset.c_str());
	}

}


/// <summary>
/// Destructor
/// </summary>
FBXCoding::~FBXCoding() { }


/// <summary>
/// Creates a map the relates node pointsers and their IDs
/// </summary>
void FBXCoding::initializeJointIdMap(FbxNode *parentNode) {

	int childCount = parentNode->GetChildCount();

	for (int i = 0; i < childCount; i++) {
		FbxNode* childI = parentNode->GetChild(i);
		p_jointMap[getCustomIdProperty(childI)] = childI;

		// Check if child has any translation curve
		if (childI->LclTranslation.IsAnimated())
			p_jointMap[TRANSLATION_CUSTOM_ID] = childI;
	}

}

void FBXCoding::encodeAnimation(FbxScene *lScene, FbxNode *markerSet, SOCKET s) {
	int Max_key_num;
	int fragmentSize;
	char *out_buf;

	if (p_enableLDPC) {
		// Maximum number of keys a package can store
		Max_key_num = PACKET_SIZE / sizeof(PACKET_LDPC);
		fragmentSize = sizeof(PACKET_LDPC);
		out_buf = (char *) new PACKET_LDPC[Max_key_num];
	}
	else if (p_enableVirtualMarkers) {
		// Maximum number of keys a package can store
		Max_key_num = PACKET_SIZE / sizeof(VIRTUAL_MARKER_PACKET);
		fragmentSize = sizeof(VIRTUAL_MARKER_PACKET);
		out_buf = (char *) new VIRTUAL_MARKER_PACKET[Max_key_num];
	}
	else {
		// Maximum number of keys a package can store
		Max_key_num = PACKET_SIZE / sizeof(REGULAR_PACKET);
		fragmentSize = sizeof(REGULAR_PACKET);
		out_buf = (char *) new REGULAR_PACKET[Max_key_num];
	}

	int pIndex = 0;

	// Vector used to store key indexes
	std::vector<int> keyIvec;
	keyIvec.reserve(p_latencyWindow);

	// We are ignoring the key at index 0, as this
	int keyI = 0;

	// Number of packets sent
	int packetSentCount = 0;

	FbxAnimStack *animStack = lScene->GetCurrentAnimationStack();
	FbxAnimLayer *animLayer = animStack->GetMember<FbxAnimLayer>();

	int keyTotal = (p_enableVirtualMarkers) ? getKeyCount(markerSet->GetChild(0)->GetChild(0), lScene) : getKeyCount(markerSet->GetChild(0), lScene);

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
			std::vector<int> shuffleKeyIndexArray(keyTotal*childCount);

			for (int i = 0; i < childCount; i++) {
				std::copy(keyIvec.begin(), keyIvec.end(), shuffleKeyIndexArray.begin() + i * keyIvec.size());
			}
			// If Interleaving is enabled shuffle vector
			if (p_isInterleavingMode){
				for (int i = 0; i < childCount; i++) {
					std::random_shuffle(shuffleKeyIndexArray.begin() + i * keyIvec.size(), (shuffleKeyIndexArray.begin() + i * keyIvec.size()) + keyIvec.size());
				}
			}
			for (int keyi = 0; keyi < keyIvec.size(); keyi++) {
				//UI_Printf("now encoding key: %d", *it);
				for (int ci = 0; ci < childCount; ci++) {
					int oldPIndex = pIndex;
					int shuffleKeyIndex = shuffleKeyIndexArray[ci * keyIvec.size() + keyi];

					// Encode rotation curves
					pIndex = encodeKeyFrame(keyTotal, animLayer, markerSet->GetChild(ci), shuffleKeyIndex, out_buf, pIndex, s, false);
					if (oldPIndex == (Max_key_num - 1) && pIndex == 0) // packet has just been sent out
						packetSentCount++;

					oldPIndex = pIndex;
					// Encode translation curves - if they exist
					pIndex = encodeKeyFrame(keyTotal, animLayer, markerSet->GetChild(ci), shuffleKeyIndex, out_buf, pIndex, s, true);
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
		if (sendto(s,  out_buf, pIndex*fragmentSize, 0, (struct sockaddr *) &p_sock_addr, sizeof(p_sock_addr)) == SOCKET_ERROR)
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

	delete out_buf;

	// Destroy scene
	lScene->Destroy();

	// Close socket after transmitting
	closesocket(s);



}


/// <summary>
/// Decode keyframes from a certain packet
/// </summary>
/// <param name="lScene">FBX Scene</param>
/// <param name="p">Raw data received from channel</param>
/// <param name="numBytesRecv">Number of bytes received</param>
void FBXCoding::decodePacket(FbxScene *lScene, char *p, int numBytesRecv) {


	// Retrieve animationlayer
	FbxAnimStack *animStack = lScene->GetCurrentAnimationStack();
	FbxAnimLayer *animLayer = animStack->GetMember<FbxAnimLayer>();

	// Calculate number of keyframes within packet
	int num_key_received;
	if (isLDPCEnabled()) {
		num_key_received = numBytesRecv / sizeof(PACKET_LDPC);

	}
	else if (p_enableVirtualMarkers) {
		num_key_received = numBytesRecv / sizeof(VIRTUAL_MARKER_PACKET);

	}		
	else {
		num_key_received = numBytesRecv / sizeof(REGULAR_PACKET);
	}



	// Iterate on incoming packets
	for (int i = 0; i < num_key_received; i++) {
		if (!isLDPCEnabled()) {

			if ( p_enableVirtualMarkers )
				decodeVirtualMarkersFrag(animLayer, ((VIRTUAL_MARKER_PACKET *)p)[i]);
			else
				decodeFragment(animLayer, ((REGULAR_PACKET *)p)[i]);
		}
		else {
			decodeLDPCFragment(animLayer, ((PACKET_LDPC *)p)[i]);
		}
	}


}

/// <summary>
/// Decode LDPC packet fragment
/// </summary>
/// <param name="animLayer">FBX Animation layer</param>
/// <param name="jointMap">Map of joints and its corresponding identifiers</param>
/// <param name="frag">Fragment to be decoded</param>
void FBXCoding::decodeLDPCFragment(FbxAnimLayer *animLayer,  PACKET_LDPC &frag) {


	// Every LDPC_PACKET is also a PACKET, so we need to decode the rest of it
	decodeFragment(animLayer, frag);

	static std::bitset<N_PARITY_BIT> zeroBitsSet;

	// If all parity bits are 0, it probably means that parity was not encoded.
	// This happens if keyIndex + offset > total key count
	if (frag.bits == zeroBitsSet)
		return;

	// Decode parity part
	p_ldpc_parity_map[std::pair<short, FbxLongLong>(frag.joint_id, frag.time)] = frag.bits;
	
}

/// <summary>
/// Decode packet fragment
/// </summary>
/// <param name="animLayer">FBX Animation layer</param>
/// <param name="jointMap">Map of joints and its corresponding identifiers</param>
/// <param name="frag">Fragment to be decoded</param>
void FBXCoding::decodeFragment(FbxAnimLayer *animLayer, REGULAR_PACKET &frag){

	//Extract frame rate
	int frag_fps = (frag.joint_id > 100) ? frag.joint_id / 100 : 0;
	int frag_joint_id = (frag.joint_id > 100) ? frag.joint_id % 100 : frag.joint_id;

	if (p_fps == 0) {
		if (frag_fps != 0) {
			p_fps = frag_fps;
			UI_Printf(" Decoded Motion Clip FPS is equal to %lf", p_fps);
		}
	}

	auto it = p_jointMap.find(frag_joint_id);
	if (it == p_jointMap.end()){
		UI_Printf(" Decoding error. Unable to find node with id %d in the scene.", frag_joint_id);
		return;
	}

	FbxNode *tgtMarker = it->second;

	FbxAnimCurve *curveX, *curveY, *curveZ;
	FbxAnimCurveDef::EInterpolationType interpType;

	// Check what type of curve we received
	if (frag.joint_id == TRANSLATION_CUSTOM_ID) {
		curveX = tgtMarker->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
		curveY = tgtMarker->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
		curveZ = tgtMarker->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);
		interpType = FbxAnimCurveDef::eInterpolationCubic;
	}
	else {
		curveX = tgtMarker->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
		curveY = tgtMarker->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
		curveZ = tgtMarker->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);
		interpType = FbxAnimCurveDef::eInterpolationCubic;
	}


	FbxTime kTime;
	kTime.SetMilliSeconds(frag.time);

	if (curveX) {
		curveX->KeyModifyBegin();

		// Start adding key
		// TODO: Speed up this operation using index
		int keyIndex = curveX->KeyInsert(kTime);
		curveX->KeySetValue(keyIndex, frag.x);
		curveX->KeySetInterpolation(keyIndex, interpType);

		curveX->KeyModifyEnd();
	}

	if (curveY) {
		curveY->KeyModifyBegin();

		// Start adding key
		// TODO: Speed up this operation using index
		int keyIndex = curveY->KeyInsert(kTime);
		curveY->KeySetValue(keyIndex, frag.y);
		curveY->KeySetInterpolation(keyIndex, interpType);

		curveY->KeyModifyEnd();
	}

	if (curveZ) {
		curveZ->KeyModifyBegin();

		// Start adding key
		// TODO: Speed up this operation using index
		int keyIndex = curveZ->KeyInsert(kTime);
		curveZ->KeySetValue(keyIndex, frag.z);
		curveZ->KeySetInterpolation(keyIndex, interpType);

		curveZ->KeyModifyEnd();
	}


}


void FBXCoding::decodeVirtualMarkersFrag(FbxAnimLayer *animLayer, VIRTUAL_MARKER_PACKET &frag) {

	int frag_fps = (frag.joint_id > 100) ? frag.joint_id / 100 : 0;
	int frag_joint_id = (frag.joint_id > 100) ? frag.joint_id % 100 : frag.joint_id;

	if (p_fps == 0) {
		if (frag_fps != 0) {
			p_fps = frag_fps;
			UI_Printf(" Decoded Motion Clip FPS is equal to %lf", p_fps);
		}
	}
	
	auto it = p_jointMap.find(frag_joint_id);
	if (it == p_jointMap.end()){
		UI_Printf(" Decoding error. Unable to find node with id %d in the scene.", frag_joint_id);
		return;
	}

	FbxNode *tgtMarker = it->second;

	FbxTime kTime;
	kTime.SetMilliSeconds(frag.time);


	FbxNode *a = tgtMarker->GetChild(0);
	FbxNode *b = tgtMarker->GetChild(1);
	FbxNode *c = tgtMarker->GetChild(2);

    FbxAnimCurve *aXCurve = a->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
	FbxAnimCurve *aYCurve = a->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
	FbxAnimCurve *aZCurve = a->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);

	FbxAnimCurve *bXCurve = b->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
	FbxAnimCurve *bYCurve = b->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
	FbxAnimCurve *bZCurve = b->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);

	FbxAnimCurve *cXCurve = c->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
	FbxAnimCurve *cYCurve = c->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
	FbxAnimCurve *cZCurve = c->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);


	applyTransformationVectorToCurve(aXCurve, kTime, frag.a[0], FbxAnimCurveDef::eInterpolationCubic);
	applyTransformationVectorToCurve(aYCurve, kTime, frag.a[1], FbxAnimCurveDef::eInterpolationCubic);
	applyTransformationVectorToCurve(aZCurve, kTime, frag.a[2], FbxAnimCurveDef::eInterpolationCubic);

	applyTransformationVectorToCurve(bXCurve, kTime, frag.b[0], FbxAnimCurveDef::eInterpolationCubic);
	applyTransformationVectorToCurve(bYCurve, kTime, frag.b[1], FbxAnimCurveDef::eInterpolationCubic);
	applyTransformationVectorToCurve(bZCurve, kTime, frag.b[2], FbxAnimCurveDef::eInterpolationCubic);

	applyTransformationVectorToCurve(cXCurve, kTime, frag.c[0], FbxAnimCurveDef::eInterpolationCubic);
	applyTransformationVectorToCurve(cYCurve, kTime, frag.c[1], FbxAnimCurveDef::eInterpolationCubic);
	applyTransformationVectorToCurve(cZCurve, kTime, frag.c[2], FbxAnimCurveDef::eInterpolationCubic);


}

/// <summary>
/// Encodes keys for curves from a given node at a given time
/// </summary>
/// <param name="animLayer">FBX Anim layer</param>
/// <param name="tgtNode">Node to have curves extracted</param>
/// <param name="keyIndex">Index of current key</param>
/// <param name="zCurve">Z curve</param>
/// <param name="p">Outgoing packet buffer</param>
/// <param name="pIndex">Index of current keyframe</param>
/// <param name="s">Socket used to send packages</param>
/// <param name="isTranslation">Are these translation curves?</param>
/// <return>Updated pIndex</return>
int FBXCoding::encodeKeyFrame(int keyTotal, FbxAnimLayer *animLayer, FbxNode *tgtNode, int keyIndex, char *p, int pIndex, SOCKET s, bool isTranslation) {


	FbxAnimCurve *xCurve = NULL;
	FbxAnimCurve *yCurve = NULL;
	FbxAnimCurve *zCurve = NULL;

	FbxAnimCurve *tgtCurve = NULL; 

	float kXVal = 0, kYVal = 0, kZVal = 0;
	FbxDouble3 aVal, bVal, cVal;


	if (!p_enableVirtualMarkers) {

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

		tgtCurve = (xCurve) ? xCurve : (yCurve) ? yCurve : zCurve;

		kXVal = (xCurve) ? xCurve->KeyGet(keyIndex).GetValue() : 0;
		kYVal = (yCurve) ? yCurve->KeyGet(keyIndex).GetValue() : 0;
		kZVal = (zCurve) ? zCurve->KeyGet(keyIndex).GetValue() : 0;

	}
	else {

		if (tgtNode->GetChildCount() != 3)
			return pIndex;

		FbxNode *a, *b, *c;
		a = tgtNode->GetChild(0);
		b = tgtNode->GetChild(1);
		c = tgtNode->GetChild(2);



		aVal = getKeyValueFromMarker(a, animLayer, keyIndex);
		bVal = getKeyValueFromMarker(b, animLayer, keyIndex);
		cVal = getKeyValueFromMarker(c, animLayer, keyIndex);

		tgtCurve  = a->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X, false);//(xCurve) ? xCurve : (yCurve) ? yCurve : zCurve;
	}

	FbxTime kTime = getKeyTimeFromCurve(tgtCurve, keyIndex);



	// Calculate FPS
	if (p_fps == 0) {
		// Get any curve that is not null
		p_fps = computeFPS(tgtCurve);

		if (p_fps != 0)
			UI_Printf(" Motion Clip FPS is equal to %lf", p_fps);
	}


	// Maximum number of keys a package can store
	int Max_key_num;
	int bytes_sent;

	if (!p_enableVirtualMarkers) {

		if (!xCurve || !yCurve || !zCurve)
			return pIndex;


		if (p_enableLDPC) {
			PACKET_LDPC &p_at_pos = ((PACKET_LDPC *)p)[pIndex];
			Max_key_num = PACKET_SIZE / sizeof(PACKET_LDPC);
			bytes_sent = Max_key_num*sizeof(PACKET_LDPC);

			encodeCommonKeyAttributes(p_at_pos, keyIndex, tgtNode, xCurve, yCurve, zCurve, isTranslation);
			encodeLDPCAttributes(keyTotal, p_at_pos, kXVal, kYVal, kZVal, keyIndex);
		}
		else {
			REGULAR_PACKET &p_at_pos = ((REGULAR_PACKET *)p)[pIndex];
			Max_key_num = PACKET_SIZE / sizeof(REGULAR_PACKET);
			bytes_sent = Max_key_num*sizeof(REGULAR_PACKET);

			encodeCommonKeyAttributes(p_at_pos, keyIndex, tgtNode, xCurve, yCurve, zCurve, isTranslation);
		}

	}
	else { // Using virtual markers
		VIRTUAL_MARKER_PACKET &p_at_pos = ((VIRTUAL_MARKER_PACKET *)p)[pIndex];
		Max_key_num = PACKET_SIZE / sizeof(VIRTUAL_MARKER_PACKET);
		bytes_sent = Max_key_num*sizeof(VIRTUAL_MARKER_PACKET);

		encodeCommonKeyAttributes(p_at_pos, tgtNode, kTime, aVal, bVal, cVal);
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


/// <summary>
/// Encodes Common key attributes
/// </summary>
/// <param name="animLayer">FBX Anim layer</param>
/// <param name="tgtNode">Node to have curves extracted</param>
/// <param name="p">Outgoing packet buffer</param>
/// <param name="pIndex">Index of current keyframe</param>
/// <param name="s">Socket used to send packages</param>
/// <param name="isTranslation">Are these translation curves?</param>
/// <return>Updated pIndex</return>
void FBXCoding::encodeCommonKeyAttributes(REGULAR_PACKET &outP, int keyIndex,  FbxNode *tgtNode, FbxAnimCurve *xCurve, FbxAnimCurve *yCurve, FbxAnimCurve *zCurve, bool isTranslation) {
	outP.joint_id = (p_fps == 0) ? getCustomIdProperty(tgtNode) : (p_fps * 100) + getCustomIdProperty(tgtNode);

	
	outP.isTranslation = isTranslation;

	if (xCurve) {
		outP.x = xCurve->KeyGet(keyIndex).GetValue();
		outP.time = zCurve->KeyGet(keyIndex).GetTime().GetMilliSeconds();
	}
	if (yCurve) {
		outP.y = yCurve->KeyGet(keyIndex).GetValue();
		outP.time = zCurve->KeyGet(keyIndex).GetTime().GetMilliSeconds();

	}
	if (zCurve) {
		outP.z = zCurve->KeyGet(keyIndex).GetValue();
		outP.time = zCurve->KeyGet(keyIndex).GetTime().GetMilliSeconds();
	}

}

void FBXCoding::encodeCommonKeyAttributes(VIRTUAL_MARKER_PACKET &outP, FbxNode *mNode ,FbxTime keyTime, FbxDouble3 aVal, FbxDouble3 bVal, FbxDouble3 cVal) {
	


	outP.joint_id = (p_fps == 0) ? getCustomIdProperty(mNode) : (p_fps * 100) + getCustomIdProperty(mNode);

	outP.time = keyTime.GetMilliSeconds();

	outP.a[0] = aVal[0];
	outP.a[1] = aVal[1];
	outP.a[2] = aVal[2];

	outP.b[0] = bVal[0];
	outP.b[1] = bVal[1];
	outP.b[2] = bVal[2];

	outP.c[0] = cVal[0];
	outP.c[1] = cVal[1];
	outP.c[2] = cVal[2];
}

/// <summary>
/// Encodes LDPC parity
/// </summary>
void FBXCoding::encodeLDPCAttributes(int keyTotal, PACKET_LDPC &outP, float x, float y, float z, int keyIndex) {
	//float x, y, z;
	static itpp::LDPC_Code ldpc_encode(&H, &G);
	// Get X, Y, Z, for offset keyframe and encode them when LDPC is enabled


	int keyOffset = keyIndex + p_LDPC_offset;
	if (keyOffset < keyTotal) {
		//	UI_Printf("key offset of %d is: %d ", keyIndex, keyOffset);
		//x = (xCurve)? xCurve->KeyGet(keyOffset).GetValue() :0.0f;
		//y = (yCurve)? yCurve->KeyGet(keyOffset).GetValue() :0.0f;
		//z = (zCurve)? zCurve->KeyGet(keyOffset).GetValue() :0.0f;

		itpp::bvec bitsin = tobvec(x);
		bitsin = concat(bitsin, tobvec(y));
		bitsin = concat(bitsin, tobvec(z));

		itpp::bvec bitsout;
		ldpc_encode.encode(bitsin, bitsout);
		bvec2Bitset(bitsout, outP);

		//std::string bvecStringIn = itpp::to_str(bitsin);
		//UI_Printf("bitsin: %s", bvecStringIn.c_str());
	}
	else {
		// Ensure parity bits are 0
		outP.bits = std::bitset<N_PARITY_BIT>();
	}
	// Printing the bits (checking to see if extracting the parity bits)
	/*
	std::string bvecStringIn = itpp::to_str(bitsin);
	std::string bvecStringOut = itpp::to_str(bitsout);

	UI_Printf("bitsin: %x", bvecStringIn.c_str());
	UI_Printf("bitsout: %s", bvecStringOut.c_str());
	UI_Printf("Parity bits: %s", ((PACKET_LDPC *) p)[pIndex].bits.to_string().c_str());
	*/
}


/// <summary>
/// Converts a bvec to bitset
/// </summary>
/// <param name="bin_list">bvec to be converted</param>
/// <param name="p">LDPC packet</param>
void FBXCoding::bvec2Bitset(itpp::bvec bin_list, PACKET_LDPC &p) {
	int eulerbitwidth = 3 * sizeof(float) * 8;
	
	// encoding only the mantissa
	if (c_encode_only_mantissa){
		eulerbitwidth = 3 * MANTISSA_WIDTH;
	}

	for (int i = eulerbitwidth; i < eulerbitwidth + N_PARITY_BIT; i++) {
		p.bits[i%N_PARITY_BIT] = bin_list[i];
	}
}


/// <summary>
/// Converts a float to bvec
/// </summary>
/// <param name="f">float value to be converted</param>
/// <param name="returnOnlyExponent">false to get mantissa, true to get the exponent and sign of the float</param>
itpp::bvec FBXCoding::tobvec(float f, bool returnOnlyExponent) {
	union
	{
		float input;   // assumes sizeof(float) == sizeof(int)
		int   output;
	}    data;

	data.input = f;

	std::bitset<sizeof(float) * CHAR_BIT> result(data.output);

	itpp::bvec resultBvec;

	// When we want only the signed and exponent of the float we add the offset
	// to access from result to resltBvec
	int offset = 0;

	if (returnOnlyExponent) {
		resultBvec.set_length(SIGN_WIDTH + EXPONENT_WIDTH);
		// To get the signed and exponent bits
		offset = MANTISSA_WIDTH;
	} else if (c_encode_only_mantissa) {
		resultBvec.set_length(MANTISSA_WIDTH);
	}
	else {
		resultBvec.set_length(sizeof(float) * CHAR_BIT);
	}


	for (int i = 0; i < resultBvec.length(); i++) {
		resultBvec[i] = result[i+offset];
	}

	return resultBvec;
}


/// <summary>
/// Converts bvec to float
/// </summary>
float FBXCoding::tofloat(itpp::bvec &input) {

	std::bitset<sizeof(float) * CHAR_BIT>   tempBSet;

	for (int i = 0; i < input.length(); i++) {
		tempBSet[i] = input[i];
	}

	union
	{
		unsigned long input;   // assumes sizeof(float) == sizeof(int)
		float   output;
	}    data;


	data.input = tempBSet.to_ulong();
	return data.output;

}

/// <summary>
/// Converts parity bitset to bvec
/// </summary>
itpp::bvec FBXCoding::tobvec(std::bitset<N_PARITY_BIT> bset) {

	itpp::bvec resultBvec;
	resultBvec.set_length(N_PARITY_BIT);

	for (int i = 0; i < resultBvec.length(); i++) {
		resultBvec[i] = bset[i];
	}

	return resultBvec;

}
/// <summary>
/// Recovers missing information in the animation by using LDPC parity data
/// </summary>
void FBXCoding::startLDPCRecovery(FbxScene *lScene) {


	if (!isLDPCEnabled())
		return;

	UI_Printf("LDPC is enabled. Starting recovery of missing packets.");



	if (p_fps == 0) {
		UI_Printf(" LDPC Reconstruction failed: Unable to estimate FPS.");
		return;
	}

	FbxAnimStack *animStack = lScene->GetCurrentAnimationStack();
	FbxAnimLayer *animLayer = animStack->GetMember<FbxAnimLayer>();


	// Number of recovered keyframes
	int recoveredCount = 0;

	// Number of skipped keyframes
	int skipCount = 0;

	// Number of processed keyframes
	int nProcessedKeys = 0;
	size_t nTotalKeys = p_ldpc_parity_map.size();
	double showRatio = 0;

	static itpp::LDPC_Code decoder(&H, &G);
	decoder.set_exit_conditions(500);

	// Counter for possible inacuraccies in LDPC prediction
	int LDPC_Interpolated_match = 0;

	for (auto &it : p_ldpc_parity_map) {

		// Let the user know the progress
		double procRatio = (nProcessedKeys++) / double(nTotalKeys);
		if (procRatio - showRatio > 0.2) {
			UI_Printf(" Recovery status: %lf%% of keyframes have been processed.", procRatio*100.0);
			showRatio = procRatio;
		}



		auto &key_pair = it.first;
		auto &value_parity = it.second;

		auto &node_it = p_jointMap.find(key_pair.first);
		if (node_it == p_jointMap.end())
			continue;

		FbxNode *tgtNode = node_it->second;

		FbxAnimCurve *xCurve, *yCurve, *zCurve, *tgtCurve = NULL;

		if (key_pair.first == TRANSLATION_CUSTOM_ID) {
			xCurve = tgtNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
			yCurve = tgtNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
			zCurve = tgtNode->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);
		}
		else {
			xCurve = tgtNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
			yCurve = tgtNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Y);
			zCurve = tgtNode->LclRotation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_Z);
		}

	
		tgtCurve = (xCurve) ? (xCurve) : (yCurve) ? (yCurve) : (zCurve);
		//UI_Printf("key count: %d", tgtCurve->KeyGetCount());
		if (tgtCurve == NULL)
			continue;
		
		static bool reEstimateFPS = false;
		if (reEstimateFPS) {
			FbxAnimCurveKey k1, k2;
			k1 = tgtCurve->KeyGet(0);
			k2 = tgtCurve->KeyGet(1);
			p_fps = decodeFPSReestimate(p_fps, k1.GetTime().GetMilliSeconds(),k2.GetTime().GetMilliSeconds());
			reEstimateFPS = false;
		}


		// Target key time
		FbxTime keyTime;
		double offsetTime = computeOffsetTime(key_pair.second, p_LDPC_offset, p_fps);
		keyTime.SetSecondDouble(offsetTime);

		// X,Y and Z values extracted from curve
		float xInterpVal = 0.0, yInterpVal = 0.0, zInterpVal = 0.0;

		if (xCurve) {
			xInterpVal = xCurve->Evaluate(keyTime);
		}
		if (yCurve) {
			yInterpVal = yCurve->Evaluate(keyTime);
		}
		if (zCurve) {
			zInterpVal = zCurve->Evaluate(keyTime);
		}

		// Offset  
		double nearestKeyIndex = tgtCurve->KeyFind(keyTime);
		bool acceptableDiff = true;


		// Reach the end of clip
		if (nearestKeyIndex >= (tgtCurve->KeyGetCount() - 1)) {
			nearestKeyIndex = tgtCurve->KeyGetCount() - 1;
		}

		acceptableDiff = calcMinKeyDiff(keyTime, nearestKeyIndex, tgtCurve);
		//UI_Printf("key_pair - id: %d, time: %lld", key_pair.first, key_pair.second);
		//UI_Printf("result of modf: %f", modf(keyIndex, &temp));
		// There is no key for the given time - reconstruct at that point
		if (!acceptableDiff) {
			recoveredCount++;
			//UI_Printf("Reconstructing key at time %lf", keyTime.GetSecondDouble());
				
			itpp::vec encodedLLR = encodeCurveLDPC(xInterpVal, yInterpVal, zInterpVal, value_parity);
			itpp::bvec decodedVec = decoder.decode(encodedLLR);

			std::string sLLR = itpp::to_str(encodedLLR);
			std::string s =  itpp::to_str(decodedVec);


			itpp::bvec xVec;
			itpp::bvec yVec;
			itpp::bvec zVec;
			if (!c_encode_only_mantissa) {
				xVec = decodedVec.split(sizeof(float) * 8);
				yVec = decodedVec.split(sizeof(float) * 8);
				zVec = decodedVec;
			}
			else {
				itpp::bvec xComp = tobvec(xInterpVal, true);
				itpp::bvec yComp = tobvec(yInterpVal, true);
				itpp::bvec zComp = tobvec(zInterpVal, true);
				
				xVec = decodedVec.split(MANTISSA_WIDTH);
				xVec = itpp::concat(xVec, xComp);
				std::string x = itpp::to_str(xVec);

				yVec = decodedVec.split(MANTISSA_WIDTH);
				yVec = itpp::concat(yVec, yComp);
				std::string y = itpp::to_str(yVec);

				zVec = decodedVec;
				zVec = itpp::concat(zVec, zComp); 
				std::string z = itpp::to_str(zVec);

			}

			float zNewVal = tofloat(zVec), yNewVal = tofloat(yVec), xNewVal = tofloat(xVec);
			
			// Check that the interpolated values or not above the threshold
			/*
			if (std::max(abs(xInterpVal - xNewVal), std::max(abs(yInterpVal - yNewVal), abs(zInterpVal - zNewVal))) > LDPC_threshold) {
				skipCount++;
				UI_Printf("Over theshold. Skipped %d keyframe(s)", skipCount);
				continue;
			}
			*/

			// Set up an alert, in case interpolated value matches LDCP calculated
			if ((abs(xInterpVal - xNewVal) <= 0.1) && (abs(yInterpVal - yNewVal) <= 0.1) && (abs(zInterpVal - zNewVal) <= 0.1)) {
				LDPC_Interpolated_match++;
			}

			// Create new keys
			bool isTranslation = (key_pair.first == TRANSLATION_CUSTOM_ID);
			if (xCurve) {
				insertKeyCurve(xCurve, keyTime, xNewVal, isTranslation);
			}
			if (yCurve) {
				insertKeyCurve(yCurve, keyTime, yNewVal, isTranslation);
			}
			if (zCurve) {
				insertKeyCurve(zCurve, keyTime, zNewVal, isTranslation);
			}
		}
	}


	UI_Printf("LDPC recovery has been performed. %d keyframes have been recovered.",recoveredCount);

	if (LDPC_Interpolated_match > 0) {
		UI_Printf(" WARNING: %d recovered keyframes are nearly identical to the interpolated value.", LDPC_Interpolated_match);
	}

}


/// <summary>
/// Encode curve data and parity, so we can use LDPC to decode it and fix missing values
/// </summary>
itpp::vec FBXCoding::encodeCurveLDPC(float xIntVal, float yIntVal, float zIntVal, std::bitset<N_PARITY_BIT> &parityVal) {
	int eulerbitwidth = 3 * sizeof(float) * 8;
	if (c_encode_only_mantissa){
		eulerbitwidth = 3 * MANTISSA_WIDTH;
	}
	
	itpp::vec encodedLLR(eulerbitwidth + N_PARITY_BIT); // the size of return vector should be 3 floats + parity

	itpp::bvec xVec = tobvec(xIntVal);
	itpp::bvec yVec = tobvec(yIntVal);
	itpp::bvec zVec = tobvec(zIntVal);
	//itpp::bvec parityVec = tobvec(parityVal);
	itpp::bvec eulerVec = itpp::concat(xVec, yVec, zVec);
	//itpp::bvec inputVec = itpp::concat(xVec, yVec, zVec,parityVec);

//	static itpp::BPSK bpsk;



	float weight = 64/(eulerbitwidth/3);

	// We are not sure about our float data (LLR = 0)
	for (int i = 0; i < eulerVec.length(); i++) {
		int iWeight = i % (eulerbitwidth / 3);
		if (iWeight >= MANTISSA_WIDTH)
			iWeight += 4;
		if (eulerVec[i] == 0)
			encodedLLR[i] =  1  +(float(iWeight) * weight);
		else // == 1
			encodedLLR[i] = (-1) -(float(iWeight) * weight);
	}



	// We are certain that parity is correct
	// So if P(b=0) -> LLT(b)=+INF
	// And if P(b=1) -> LLT(b)=-INF
	for (int i = 0; i < parityVal.size(); i++) {
		if (parityVal[i] == 0)
			encodedLLR[i + eulerbitwidth] = std::numeric_limits<double>::infinity();
		else // == 1
			encodedLLR[i + eulerbitwidth] = -std::numeric_limits<double>::infinity();
	}

	std::string str = itpp::to_str(encodedLLR);


	return encodedLLR; 
	
}


/// <summary>
/// Recomputes FPS estimate, based on actual timestamps
/// </summary>
double FBXCoding::decodeFPSReestimate(double curFPS, FbxLongLong t1, FbxLongLong t2) {

	double frameDiff = double(t2 - t1) / ( 1000.0 / curFPS );

	double newFPS = (1000.0 * round(frameDiff)) / double(t2 - t1);

	UI_Printf("FPS was re-estimated. New value is equal to %lf", newFPS);

	return newFPS;
}

/// <summary>
/// Calculate Minimum key difference, returns true if difference is acceptable
/// </summary>
bool FBXCoding::calcMinKeyDiff(FbxTime key1Time, double key2Index, FbxAnimCurve *tgtCurve) {
	
	int roundKey = (int)round(key2Index);
	FbxTime key2Time;
	key2Time = tgtCurve->KeyGet(roundKey).GetTime();

	//UI_Printf("absolute min difference in miliseconds: %lld", abs(key2Time.GetMilliSeconds() - key1Time.GetMilliSeconds()));
	
	return abs(key2Time.GetMilliSeconds() - key1Time.GetMilliSeconds()) < (c_minKeyIndexDiff*(1000.0/p_fps));
}