// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// Do not define WIN32_LEAN_AND_MEAN to use OPENFILENAME structure
// #define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>



// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// C++ STD header Files
#include <list>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <memory>
#include <exception>
#include <future>
#include <vector>


//Additional program headers
// FBX SDK
#include <fbxsdk.h>

// Kinect SDK
#include <Kinect.h>

// DirectD2D -- for visualization
#include <d2d1.h>

// Library containing helpers
#include "CommonKinect\CommmonKinect.h"


// Safe release for interfaces
template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease)
{
	if (pInterfaceToRelease != NULL)
	{
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}