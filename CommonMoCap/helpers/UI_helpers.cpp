#include "UI_helpers.h"
#include "FBX_helpers.h"



// call this to add a message to the EXECUTE_STATUS edit box
// same variable arguments list as function printf()
void UI_Printf(
	const char* pMsg,
	...
	)
{
	if (ghWnd == NULL) return;

	// build the pMsg with variable arguments 
	char msg[2048];
	va_list Arguments;
	va_start(Arguments, pMsg);     // Initialize variable arguments.
	FBXSDK_vsprintf(msg, 2048, pMsg, Arguments);
	va_end(Arguments);            // Reset variable arguments.

	// get the HWND of the editbox
	HWND hWndStatus = GetDlgItem(ghWnd, EXECUTE_STATUS);

	// append \r\n to the msg to print on a new line
	int msglen = int(strlen(msg));
	if (msglen < 1) return;
	msglen = msglen + int(strlen("\r\n")) + 1;

	char* msgrn = (char*)GlobalAlloc(GPTR, msglen);
	memset(msgrn, 0, msglen);
	FBXSDK_strcat(msgrn, msglen, msg);
	FBXSDK_strcat(msgrn, msglen, "\r\n");
	//----------

	// append the new message to the end
	int len = GetWindowTextLength(hWndStatus);
	if (len > 0)
	{
		// get the Editbox text content
		char* buf = (char*)GlobalAlloc(GPTR, len + 1);
		GetDlgItemText(ghWnd, EXECUTE_STATUS, buf, len + 1);

		// append the new text
		msglen = int(strlen(msgrn)) + len + 1;
		char* s = (char*)GlobalAlloc(GPTR, msglen);
		memset(s, 0, msglen);
		FBXSDK_strcat(s, msglen, buf);
		FBXSDK_strcat(s, msglen, msgrn);

		// set the new text
		SetWindowText(hWndStatus, s);

		GlobalFree((HANDLE)buf);
		GlobalFree((HANDLE)s);
	}
	else
	{
		SetWindowText(hWndStatus, msgrn);
	}

	GlobalFree((HANDLE)msgrn);

	// scroll to bottom
	SendMessage(hWndStatus, (UINT)EM_SCROLL, SB_BOTTOM, (LPARAM)0);
}


// show the <Open file> dialog
void GetInputFileName(
	HWND hWndParent, char *gszInputFile
	)
{
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));

	char szFile[_MAX_PATH];  // buffer for file name
	ZeroMemory(szFile, sizeof(szFile));

	// Initialize OPENFILENAME
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWndParent;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = "Select the file to import from ... (use the file type filter)";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	// get a description of all readers registered in the FBX SDK
	const char *filter = GetReaderOFNFilters();
	ofn.lpstrFilter = filter;

	// Display the Open dialog box. 
	if (GetOpenFileName(&ofn) == false)
	{
		// user cancel
		delete filter;
		return;
	}

	delete filter;

	// show the file name selected
	SetWindowText(GetDlgItem(hWndParent, IMPORT_FROM_EDITBOX), szFile);

	// Keep a copy of the file name
	FBXSDK_strcpy(gszInputFile, _MAX_PATH, szFile);
}

// show the <Save file> dialog
void GetOutputFileName(
	HWND hWndParent, char *gszOutputFile
	)
{
	int  gWriteFileFormat = -1;             // Write file format

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));

	char szFile[_MAX_PATH];  // buffer for file name
	ZeroMemory(szFile, sizeof(szFile));

	// Initialize OPENFILENAME
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWndParent;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile) / sizeof(*szFile);
	ofn.nFilterIndex = 1;      // *.fbx binairy by default
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = "Select the file to export to ... (use the file type filter)";
	ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT;

	// get a description of all writers registered in the FBX SDK
	const char *filter = GetWriterSFNFilters();
	ofn.lpstrFilter = filter;

	// Display the save as dialog box. 
	if (GetSaveFileName(&ofn) == false)
	{
		// User cancel ...
		delete filter;
		return;
	}

	delete filter;

	// keep the selected file format writer
	// ofn.nFilterIndex is not 0 based but start at 1, the FBX SDK file format enum start at 0
	gWriteFileFormat = ofn.nFilterIndex - 1;

	// get the extention string from the file format selected by the user
	const char * ext = GetFileFormatExt(gWriteFileFormat);

	// check for file extention
	if (ExtExist(szFile, ext) == false)
	{
		// add the selected file extention
		FBXSDK_strcat(szFile, _MAX_PATH, ext);
	}

	delete ext;

	// show the file name selected with the extention
	SetWindowText(GetDlgItem(hWndParent, DECOMPRESS_TO_EDITBOX), szFile);

	// Keep a copy of the file name
	FBXSDK_strcpy(gszOutputFile, _MAX_PATH, szFile);
}

// check if in the filepath the file extention exist
bool ExtExist(
	const char * filepath,
	const char * ext
	)
{
	int iExtLen = (int)strlen(ext);
	int ifpLen = (int)strlen(filepath);

	if (ifpLen < iExtLen) return false;

	int x = ifpLen - iExtLen;

	for (int i = 0; i < iExtLen; i++)
	{
		if (filepath[x] != ext[i]) return false;
		x++;
	}

	return true;
}