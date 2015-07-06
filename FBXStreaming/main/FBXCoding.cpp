#include "FBXCoding.h"
#include "FBXTransmitter.h"
//#include "FBXJointConverter.h"
//#include "keyFramePackets.h"


/// <summary>
/// Constructor
/// </summary>
FBXCoding::FBXCoding(int latency) { latencyWindow = latency;}


/// <summary>
/// Destructor
/// </summary>
FBXCoding::~FBXCoding() { }

void FBXCoding::encodeAnimation(FbxScene *lScene, FbxNode *markerSet, SOCKET s) {
	// Maximum number of keys a package can store
	const int Max_key_num = PACKET_SIZE / sizeof(PACKET);

	PACKET *p = new PACKET[Max_key_num];
	int pIndex = 0;

	//checking how many keys were coded
	//int keySent = 1;

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
		if (keyIvec.size() != latencyWindow) {
			keyIvec.push_back(keyI);
			//UI_Printf("%d has been added to the vector", keyI);
		}
		if (keyIvec.size() == latencyWindow || keyI == (keyTotal-1)) {
			std::random_shuffle(keyIvec.begin(), keyIvec.end());
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
				
				//UI_Printf("Sent %d / %d keys", keySent, keyTotal);
				//keySent++;
			}

			keyIvec.clear();

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



}

/*
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
		if ((((float)(keyI - encodedKeyCount)) / keyTotal) > 0.2) {
			encodedKeyCount = keyI;
			UI_Printf("%f%% of the keys have been encoded.", (((float)encodedKeyCount) / keyTotal) * 100);
		}

		for (int ci = 0; ci < childCount; ci++) {
			int oldPIndex = pIndex;

			// Encode rotation curves
			pIndex = encodeKeyFrame(animLayer, markerSet->GetChild(ci), keyI, p, pIndex, s, false);
			if (oldPIndex == (Max_key_num - 1) && pIndex == 0) // packet has just been sent out
				packetSentCount++;

			oldPIndex = pIndex;
			// Encode translation curves - if they exist
			pIndex = encodeKeyFrame(animLayer, markerSet->GetChild(ci), keyI, p, pIndex, s, true);
			if (oldPIndex == (Max_key_num - 1) && pIndex == 0)
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
*/


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