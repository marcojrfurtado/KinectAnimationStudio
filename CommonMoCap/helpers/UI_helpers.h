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

