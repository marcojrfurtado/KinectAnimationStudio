#pragma once

#include "../common/stdafx.h"
#include "keyFramePackets.h"
#include "../config/ConfigFileParser.h"
#include "FBXJointConverter.h"


class FBXCoding {

public:
	
	/// <summary>
	/// Constructor
	/// </summary>
	/// <param name="enableInterleaving">Interleave packets before sending</param>
	/// <param name="latency">Interleaving latency window</param>
	FBXCoding(bool enableInterleaving = true, int latency = 10, bool enableLDPC = false, int ldpc_offset = 0);

	/// <summary>
	/// Destructor
	/// </summary>
	~FBXCoding();

	/// <summary>
	/// Creates a map the relates node pointsers and their IDs
	/// </summary>
	void initializeJointIdMap(FbxNode *parentNode);

	/// <summary>
	/// Encode keyframes from markers
	/// </summary>
	void encodeAnimation(FbxScene *lScene, FbxNode *markerSet, SOCKET s);

	/// <summary>
	/// Decode keyframes from a certain packet
	/// </summary>
	/// <param name="lScene">FBX Scene</param>
	/// <param name="jointMap"></param>
	/// <param name="p">Raw data received from channel</param>
	/// <param name="numBytesRecv">Number of bytes received</param>
	void decodePacket(FbxScene *lScene, char *p, int numBytesRecv);


	// Sets socket address used by encoder and decoder
	void setSockAddress(struct sockaddr_in new_addr) {
		p_sock_addr = new_addr;
	}

	/// <summary>
	/// Prints details about initial configuration
	/// </summary>
	void printSetupDetails() {
		UI_Printf(" Coder Initialization: Latency Window set to %d", p_latencyWindow);
		UI_Printf(" Coder Initialization: Interleaving packet mode set to %d", p_isInterleavingMode);
		UI_Printf(" Coder Initialization: LDPC packet mode set to %d", p_enableLDPC);
		UI_Printf(" Coder Initialization: LDPC offset set to %d", p_LDPC_offset);
		UI_Printf("size of float %d", sizeof(float));
		UI_Printf("size of short %d", sizeof(short));
	}

	/// <summary>
	/// Recovers missing information in the animation by using LDPC parity data
	/// </summary>
	void startLDPCRecovery(FbxScene *scene);

	/// <summary>
	/// Chekcs whether LDPC has been enabled
	/// </summary>
	bool isLDPCEnabled() { return p_enableLDPC;  }

private:
	// Constant definitions
	const char *c_defaultParityFileName = "default_H.alist";

	// Minimum key index difference
	const double c_minKeyIndexDiff = 0.0001;


	// Private attributes


	//Map of joints and its corresponding identifiers
	// Used to speed up decoding
	std::map<short, FbxNode*> p_jointMap;

	// True, if we are interleaving packets
	bool  p_isInterleavingMode;

	// Latency window, used for interleaving packets
	int p_latencyWindow;

	// True, if we are enabling LDPC
	bool p_enableLDPC;
	
	// LDPC offset
	int p_LDPC_offset;

	// Parity Matrix
	itpp::LDPC_Parity_Regular H;

	// Generator Matrix
	itpp::LDPC_Generator_Systematic G;

	// Address used to send packets
	struct sockaddr_in p_sock_addr;

	// Parity map, used to restore missing keyframes
	// Relates key timestamps to their corresponding parities
	std::map<std::pair<short,FbxLongLong>, std::bitset<N_PARITY_BIT>> p_ldpc_parity_map;


	// Framerate
	double p_fps;

	// Private methods


	/// <summary>
	/// Decode LDPC packet fragment
	/// </summary>
	/// <param name="animLayer">FBX Animation layer</param>
	/// <param name="frag">Fragment to be decoded</param>
	void decodeLDPCFragment(FbxAnimLayer *animLayer,  PACKET_LDPC &frag);

	/// <summary>
	/// Decode packet fragment
	/// </summary>
	/// <param name="animLayer">FBX Animation layer</param>
	/// <param name="frag">Fragment to be decoded</param>
	void decodeFragment(FbxAnimLayer *animLayer,  PACKET &frag);

	/// <summary>
	/// Encodes keys for curves from a given node
	/// </summary>
	/// <param name="keyTotal">Total number of existing keys</param>
	/// <param name="animLayer">FBX Anim layer</param>
	/// <param name="tgtNode">Node to have curves extracted</param>
	/// <param name="p">Outgoing packet buffer</param>
	/// <param name="pIndex">Index of current keyframe</param>
	/// <param name="s">Socket used to send packages</param>
	/// <param name="isTranslation">Are these translation curves?</param>
	/// <return>Updated pIndex</return>
	int encodeKeyFrame(int keyTotal, FbxAnimLayer *animLayer, FbxNode *tgtNode, int keyIndex, char *p, int pIndex, SOCKET s, bool isTranslation = false);

	/// <summary>
	/// Encodes Common key attributes
	/// </summary>
	/// <param name="outP">Reference to packet that will be overwritten</param>
	/// <param name="keyIndex">Index of current key</param>
	/// <param name="tgtNode">FBX node to have information extracted</param>
	/// <param name="xCurve">X curve</param>
	/// <param name="yCurve">Y curve</param>
	/// <param name="zCurve">Z curve</param>
	/// <param name="isTranslation">Are these translation curves?</param>
	void encodeCommonKeyAttributes(PACKET &outP, int keyIndex, FbxNode *tgtNode, FbxAnimCurve *xCurve, FbxAnimCurve *yCurve, FbxAnimCurve *zCurve, bool isTranslation = false);

	/// <summary>
	/// Encodes LDPC parity, storing it in a PACKET fragment
	/// </summary>
	/// <param name="keyTotal">Maximum number of keys</param>
	/// <param name="outP">Output packet</param>
	/// <param name="xCurve">X curve</param>
	/// <param name="yCurve">Y curve</param>
	/// <param name="zCurve">Z curve</param>
	/// <param name="keyIndex">Index of current key</param>
	void encodeLDPCAttributes(int keyTotal, PACKET_LDPC &outP, FbxAnimCurve *xCurve, FbxAnimCurve *yCurve, FbxAnimCurve *zCurve, int keyIndex);

	void bvec2Bitset(itpp::bvec bin_list, PACKET_LDPC &p);

	itpp::bvec tobvec(float f);

	/// <summary>
	/// Converts parity bitset to bvec
	/// </summary>
	itpp::bvec tobvec(std::bitset<N_PARITY_BIT> bset);

	/// <summary>
	/// Converts bvec to float
	/// </summary>
	float tofloat(itpp::bvec &input);


	/// <summary>
	/// Encode curve data and parity, so we can use LDPC to decode it and fix missing values
	/// </summary>
	itpp::bvec encodeCurveLDPC(float xIntVal, float yIntVal, float zIntVal, std::bitset<N_PARITY_BIT> &parityVal);

	/// <summary>
	/// Recomputes FPS estimate, based on actual timestamps
	/// </summary>
	double decodeFPSReestimate(double curFPS, FbxLongLong t1, FbxLongLong t2);
};