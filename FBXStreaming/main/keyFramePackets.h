#pragma once

#include "common/stdafx.h"

#define PACKET_SIZE 512

// struct for what a packet will transmit

struct PACKET {
	FbxUInt64 joint_id;
	float x, y, z;
	FbxLongLong time;
};

