#pragma once

#include "../common/stdafx.h"

#define PACKET_SIZE 512
#define N_PARITY_BIT 8

// Since we only have one translation curve, it can be treated in a special way
#define TRANSLATION_CUSTOM_ID -1

// struct for what a packet will transmit

struct PACKET {
	short joint_id;
	float x, y, z;
	FbxLongLong time;
};

struct PACKET_LDPC : PACKET {
	std::bitset<N_PARITY_BIT> bits;
};
