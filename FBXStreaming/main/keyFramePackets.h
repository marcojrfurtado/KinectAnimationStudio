#pragma once

#include "../common/stdafx.h"

#define PACKET_SIZE 512

// Define number of parity bits used to protect data
#define N_PARITY_BIT 16

// Number of bits of data to be encoded and protected
// Currently we are protecting 3 floats (x,y,z), which correspond to 4*3bytes = 4*3*8 bits = 96 bits
#define N_DATA_BIT 96

// Since we only have one translation curve, it can be treated in a special way
#define TRANSLATION_CUSTOM_ID -1

#define SIGN_WIDTH 1

#define EXPONENT_WIDTH 8

#define MANTISSA_WIDTH 23

// struct for what a packet will transmit

struct PACKET {
	short joint_id;
	float x, y, z;
	FbxLongLong time;
};

struct PACKET_LDPC : PACKET {
	std::bitset<N_PARITY_BIT> bits;
};
