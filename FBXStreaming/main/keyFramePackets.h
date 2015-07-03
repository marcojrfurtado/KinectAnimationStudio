#pragma once

#include "../common/stdafx.h"

#define PACKET_SIZE 512

// struct for what a packet will transmit

struct PACKET {
	int joint_id;
	float x, y, z, tx, ty, tz;
	FbxLongLong time;
};

