
// UI.cxx : Defines the entry point for the application.
#include "UI.h"
#include "../main/FBXTransmitter.h"

#define MAX_LOADSTRING 100

/********************************
Begin of Global Variables:
*****************************/
HINSTANCE hInst;                        // current instance
HWND      ghWnd = NULL;                 // main window

TCHAR szTitle[MAX_LOADSTRING];          // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];    // the main window class name

char gszInputFile[_MAX_PATH];            // File name to import
char gszOutputFile[_MAX_PATH];           // File name to decompress

// Global FBX SDK manager
FbxManager *gSdkManager;
FBXTransmitter transmitter;


/********************************
End of Global Variables:
*****************************/



// Forward declarations of functions included in this code module:
ATOM                UIRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void CreateUIControls(HWND hWndParent);


// used to show messages from the ImportExport.cxx file
// void UI_Printf(const char* msg, ...);

// used to check if in the filepath the file extention exist
bool ExtExist(const char * filepath, const char * ext);

static bool gAutoQuit = false;

// entry point for the application
int APIENTRY WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow
	)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	if (FbxString(lpCmdLine) == "-test") gAutoQuit = true;

	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_UI, szWindowClass, MAX_LOADSTRING);
	UIRegisterClass(hInstance);

	// empty our global file name buffer
	ZeroMemory(gszInputFile, sizeof(gszInputFile));
	ZeroMemory(gszOutputFile, sizeof(gszOutputFile));

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_UI));



	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0) && !gAutoQuit)
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

//
//  FUNCTION: UIRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM UIRegisterClass(
	HINSTANCE hInstance
	)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_UI));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_UI);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_UI));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//   In this function, we save the instance handle in a global variable,
//   create and display the main program window.
//
BOOL InitInstance(
	HINSTANCE hInstance,
	int nCmdShow
	)
{
	hInst = hInstance; // Store instance handle in our global variable

	ghWnd = CreateWindow(
		szWindowClass,                        // LPCTSTR lpClassName
		"Kinect Animation Studio",   // LPCTSTR caption 
		WS_OVERLAPPED | WS_SYSMENU,           // DWORD dwStyle
		400,                                  // int x
		200,                                  // int Y
		700,                                  // int nWidth
		520,                                  // int nHeight
		NULL,                                 // HWND hWndParent
		NULL,                                 // HMENU hMenu
		hInstance,                            // HINSTANCE hInstance
		NULL                                  // LPVOID lpParam
		);

	if (!ghWnd)
	{
		return FALSE;
	}

	ShowWindow(ghWnd, nCmdShow);
	UpdateWindow(ghWnd);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
LRESULT CALLBACK WndProc(
	HWND hWnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam
	)
{
	int wmId, wmEvent;

	switch (message)
	{

	case WM_CREATE:
	{
		CreateUIControls(hWnd);

		// Initalize FBX SDK Manager
		InitializeSdkManager();

		// Pass the SDK manager to the transmitter
		transmitter.initFBXSDKManager(gSdkManager);


	}
	break;
	case WM_COMMAND:

		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;

		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;

		case IMPORT_FROM_BUTTON:
			GetInputFileName(hWnd,gszInputFile);
			transmitter.setImporttName(gszInputFile);
			break;

		case EXPORT_TO_BUTTON:
			GetOutputFileName(hWnd, gszOutputFile);
			transmitter.setImporttName(gszOutputFile);
			break;

		case START_SERVER_BUTTON:
			UI_Printf("Server Mode has been enabled");
			transmitter.startServer();
			break;

		case SEND_FILE_BUTTON:
			if (*gszInputFile == NULL) {
				UI_Printf("Input file has not been set.");
			}
			else {
				UI_Printf("Beginning of transmission");
				transmitter.transmit(gszInputFile);
				UI_Printf("End of transmission");
			}
			break;
		case UPDATE_SERVER_CLIENT_PORT_BUTTON:
			// Update port
			char portSetStr[10];
			int portSet;
			GetWindowText(GetDlgItem(hWnd, SET_SERVER_CLIENT_PORT_EDIT), portSetStr, 10);

			portSet = atoi(portSetStr);
			if (portSet <= 0) {
				UI_Printf("Invalid port number.");
			}
			else {
				transmitter.setPort(portSet);
				UI_Printf("Port has been updated to %d", portSet);
			}
			break;
		case UPDATE_CLIENT_CONNECT_ADDRESS_BUTTON:
			// Update address
			char addressStr[50];
			GetWindowText(GetDlgItem(hWnd, SET_CLIENT_CONNECT_ADDRESS_EDIT), addressStr, 50);
			transmitter.setConnectAddress(addressStr);
			UI_Printf("Client connection address has been updated to %s.",addressStr);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_DESTROY:

		// dont forget to delete the SdkManager 
		// and all objects created by the SDK manager
		DestroySdkObjects(gSdkManager, true);

		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(
	HWND hDlg,
	UINT message,
	WPARAM wParam,
	LPARAM lParam
	)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		// set the FBX app. icon
		HINSTANCE hinst = (HINSTANCE)(LONG_PTR)GetWindowLongPtr(hDlg, GWLP_HINSTANCE);

		HICON hIconFBX = (HICON)LoadImage(hinst, MAKEINTRESOURCE(IDI_UI), IMAGE_ICON, 16, 16, LR_DEFAULTSIZE);
		::SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIconFBX);
	}

	return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


// Create the UI childs controls
void CreateUIControls(
	HWND hWndParent
	)
{
	HWND hwnd_sub_handler;
	DWORD dwStyle = WS_CHILD | WS_VISIBLE;

	// create the <Import from> button
	CreateWindowEx(
		0,                          // DWORD dwExStyle,
		"BUTTON",                   // LPCTSTR lpClassName
		"Import from ...",          // LPCTSTR lpWindowName / control caption
		dwStyle,                    // DWORD dwStyle
		10,                         // int x
		10,                         // int y
		130,                        // int nWidth
		30,                         // int nHeight    
		hWndParent,                 // HWND hWndParent
		(HMENU)IMPORT_FROM_BUTTON, // HMENU hMenu or control's ID for WM_COMMAND 
		hInst,                      // HINSTANCE hInstance
		NULL                        // LPVOID lpParam
		);

	// create the <Export to> button
	CreateWindowEx(
		0,                          // DWORD dwExStyle,
		"BUTTON",                   // LPCTSTR lpClassName
		"Export to ...",          // LPCTSTR lpWindowName / control caption
		dwStyle,                    // DWORD dwStyle
		10,                         // int x
		50,                         // int y
		130,                        // int nWidth
		30,                         // int nHeight    
		hWndParent,                 // HWND hWndParent
		(HMENU)EXPORT_TO_BUTTON, // HMENU hMenu or control's ID for WM_COMMAND 
		hInst,                      // HINSTANCE hInstance
		NULL                        // LPVOID lpParam
		);

	// create the <Import from> button
	CreateWindowEx(
		0,                          // DWORD dwExStyle,
		"BUTTON",                   // LPCTSTR lpClassName
		"Send File (Client)",          // LPCTSTR lpWindowName / control caption
		dwStyle,                    // DWORD dwStyle
		10,                         // int x
		90,                         // int y
		130,                        // int nWidth
		30,                         // int nHeight    
		hWndParent,                 // HWND hWndParent
		(HMENU)SEND_FILE_BUTTON, // HMENU hMenu or control's ID for WM_COMMAND 
		hInst,                      // HINSTANCE hInstance
		NULL                        // LPVOID lpParam
		);


	
	// create the <Start Server> button
	CreateWindowEx(
		0,                          // DWORD dwExStyle,
		"BUTTON",                   // LPCTSTR lpClassName
		"Start Server",            // LPCTSTR lpWindowName / control caption
		dwStyle,                    // DWORD dwStyle
		10,                         // int x
		130,                         // int y
		130,                        // int nWidth
		30,                         // int nHeight    
		hWndParent,                 // HWND hWndParent
		(HMENU)START_SERVER_BUTTON,   // HMENU hMenu or control's ID for WM_COMMAND 
		hInst,                      // HINSTANCE hInstance
		NULL                        // LPVOID lpParam
		);

	// create the <Import from> edit box
	CreateWindowEx(
		WS_EX_STATICEDGE,               // DWORD dwExStyle,
		"EDIT",                         // LPCTSTR lpClassName
		" <- select a file to import",  // LPCTSTR lpWindowName / control caption
		dwStyle | ES_AUTOHSCROLL,         // DWORD dwStyle
		150,                            // int x
		15,                             // int y
		530,                            // int nWidth
		20,                             // int nHeight    
		hWndParent,                     // HWND hWndParent
		(HMENU)IMPORT_FROM_EDITBOX,    // HMENU hMenu or control's ID for WM_COMMAND 
		hInst,                          // HINSTANCE hInstance
		NULL                            // LPVOID lpParam
		);

	// create the <Export to> edit box
	CreateWindowEx(
		WS_EX_STATICEDGE,               // DWORD dwExStyle,
		"EDIT",                         // LPCTSTR lpClassName
		" <- select a file to export",  // LPCTSTR lpWindowName / control caption
		dwStyle | ES_AUTOHSCROLL,         // DWORD dwStyle
		150,                            // int x
		50,                             // int y
		530,                            // int nWidth
		20,                             // int nHeight    
		hWndParent,                     // HWND hWndParent
		(HMENU)EXPORT_TO_EDITBOX,    // HMENU hMenu or control's ID for WM_COMMAND 
		hInst,                          // HINSTANCE hInstance
		NULL                            // LPVOID lpParam
		);

	// create the <Execute Status> edit box
	DWORD dwStyle_Edit = ES_AUTOHSCROLL | ES_MULTILINE | WS_HSCROLL | WS_VSCROLL;
	CreateWindowEx(
		WS_EX_STATICEDGE,                   // DWORD dwExStyle,
		"EDIT",                             // LPCTSTR lpClassName
		"",                                 // LPCTSTR lpWindowName / control caption
		dwStyle | dwStyle_Edit,             // DWORD dwStyle
		150,                                // int x
		85,                                 // int y
		530,                                // int nWidth
		365,                                // int nHeight    
		hWndParent,                         // HWND hWndParent
		(HMENU)EXECUTE_STATUS,             // HMENU hMenu or control's ID for WM_COMMAND 
		hInst,                              // HINSTANCE hInstance
		NULL                                // LPVOID lpParam
		);

	// Label: Set client or server port
	hwnd_sub_handler = CreateWindow("static", "ST_U",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP,
		10,									// int x
		250,									// int y
		130,									// int nWidth
		30,									// int nHeight 
		hWndParent,
		(HMENU)SET_SERVER_CLIENT_PORT_LABEL,
		hInst, NULL);
	SetWindowText(hwnd_sub_handler, "Default port:");

	char defaultPort[10];
	sprintf_s(defaultPort, "%d", transmitter.getPort());
	// Editbox: Set client or server port
	CreateWindowEx(
		WS_EX_STATICEDGE,               // DWORD dwExStyle,
		"EDIT",                         // LPCTSTR lpClassName
		defaultPort,  // LPCTSTR lpWindowName / control caption
		dwStyle | ES_AUTOHSCROLL,         // DWORD dwStyle
		10,                            // int x
		280,                             // int y
		130,                            // int nWidth
		20,                             // int nHeight    
		hWndParent,                     // HWND hWndParent
		(HMENU)SET_SERVER_CLIENT_PORT_EDIT,    // HMENU hMenu or control's ID for WM_COMMAND 
		hInst,                          // HINSTANCE hInstance
		NULL                            // LPVOID lpParam
		);

	// create the <Update port> button
	CreateWindowEx(
		0,               // DWORD dwExStyle,
		"BUTTON",                         // LPCTSTR lpClassName
		"Update Port",  // LPCTSTR lpWindowName / control caption
		dwStyle,         // DWORD dwStyle
		10,                            // int x
		310,                             // int y
		130,                            // int nWidth
		20,                             // int nHeight    
		hWndParent,                     // HWND hWndParent
		(HMENU)UPDATE_SERVER_CLIENT_PORT_BUTTON,    // HMENU hMenu or control's ID for WM_COMMAND 
		hInst,                          // HINSTANCE hInstance
		NULL                            // LPVOID lpParam
		);

	// Label: Set client or server address
	hwnd_sub_handler = CreateWindow("static", "ST_U",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP,
		10,									// int x
		370,									// int y
		130,									// int nWidth
		30,									// int nHeight 
		hWndParent,
		(HMENU)SET_CLIENT_CONNECT_ADDRESS_LABEL,
		hInst, NULL);
	SetWindowText(hwnd_sub_handler, "Default address:");

	char defaultAddress[50];
	sprintf_s(defaultAddress, "%s", transmitter.getConnectAddress());
	// Editbox: Set client or server address
	CreateWindowEx(
		WS_EX_STATICEDGE,               // DWORD dwExStyle,
		"EDIT",                         // LPCTSTR lpClassName
		defaultAddress,  // LPCTSTR lpWindowName / control caption
		dwStyle | ES_AUTOHSCROLL,         // DWORD dwStyle
		10,                            // int x
		400,                             // int y
		130,                            // int nWidth
		20,                             // int nHeight    
		hWndParent,                     // HWND hWndParent
		(HMENU)SET_CLIENT_CONNECT_ADDRESS_EDIT,    // HMENU hMenu or control's ID for WM_COMMAND 
		hInst,                          // HINSTANCE hInstance
		NULL                            // LPVOID lpParam
		);

	// Create the <Update Address> button
	CreateWindowEx(
		0,               // DWORD dwExStyle,
		"BUTTON",                         // LPCTSTR lpClassName
		"Update Address",  // LPCTSTR lpWindowName / control caption
		dwStyle,         // DWORD dwStyle
		10,                            // int x
		430,                             // int y
		130,                            // int nWidth
		20,                             // int nHeight    
		hWndParent,                     // HWND hWndParent
		(HMENU)UPDATE_CLIENT_CONNECT_ADDRESS_BUTTON,    // HMENU hMenu or control's ID for WM_COMMAND 
		hInst,                          // HINSTANCE hInstance
		NULL                            // LPVOID lpParam
		);
}
