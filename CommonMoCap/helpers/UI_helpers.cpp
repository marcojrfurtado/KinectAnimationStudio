#include "UI_helpers.h"




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
