#pragma once

#include "../common/stdafx.h"
#include "keyFramePackets.h"
#include "../config/ConfigFileParser.h"


class FBXCoding {

public:
	
	/// <summary>
	/// Constructor
	/// </summary>
	/// <param name="enableInterleaving">Interleave packets before sending</param>
	/// <param name="latency">Interleaving latency window</param>
	FBXCoding(bool enableInterleaving = true, int latency = 10);

	/// <summary>
	/// Destructor
	/// </summary>
	~FBXCoding();

	/// <summary>
	/// Encode keyframes from markers
	/// </summary>
	void encodeAnimation(FbxScene *lScene, FbxNode *markerSet, SOCKET s);

	/// <summary>
	/// Decode keyframes from a certain packet
	/// </summary>
	void decodePacket(FbxScene *lScene, std::map<short, FbxNode *> jointMap, PACKET *p, int keyframeNum);

	// Sets socket address used by encoder and decoder
	void setSockAddress(struct sockaddr_in new_addr) {
		p_sock_addr = new_addr;
	}

private:

	// True, if we are interleaving packets
	bool  p_isInterleavingMode;

	// Latency window, used for interleaving packets
	int p_latencyWindow;

	// Address used to send packets
	struct sockaddr_in p_sock_addr;

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



};