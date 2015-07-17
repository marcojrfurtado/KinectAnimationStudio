#pragma once
#include "..\stdafx.h"

#include "WindowIDS.h" // So we can write to specific windows

// extern variables
extern HWND      ghWnd;                 // main window

// Used to define helper functions

// call this to add a message to the EXECUTE_STATUS edit box
// same variable arguments list as function printf()
void UI_Printf(
	const char* pMsg,
	...
	);

// show the <Open file> dialog
void GetInputFileName(HWND hWndParent, char *gszInputFile);
// show the <Open file> dialog
void GetOutputFileName(HWND hWndParent, char *gszOutputFile);

// check if in the filepath the file extention exist
bool ExtExist(
	const char * filepath,
	const char * ext
	);

/// <summary>
/// Returns path to a file located at the executable folder
/// </summary>
/// <param name="in_fileName">File name to be located in the local directory</param>
/// <param name="out_Path">Output path</param>
/// <param name="out_Size">Output path size</param>
void GetLocalFile(const char *in_fileName, char *out_Path, size_t out_Size);

