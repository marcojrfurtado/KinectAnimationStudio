
// UI.CPP : Defines the entry point for the application.
#include "UI.h"


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
int  gWriteFileFormat = -1;             // Write file format

// Global FBX SDK manager
FbxManager *gSdkManager; 

// Global Kinect Sensor
IKinectSensor *gKinectSensor;

// Reponsible for processing incoming frames
KinectFrameProcessor kFrameProcessor;

// Use Smart pointers for our Body readers
// Responsible for displayer skeletons
KVisualizer_ptr kVisualizer = std::make_shared<KBodyVisualizer>();
// Responsible for converting skeletons to FBX
KExporter_ptr kExporter = std::make_shared<KBodyExporter>();


/********************************
End of Global Variables:
*****************************/



// Forward declarations of functions included in this code module:
ATOM                UIRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void CreateUIControls(HWND hWndParent);
void GetInputFileName(HWND hWndParent);
void GetOutputFileName(HWND hWndParent);

// used to show messages from the ImportExport.cxx file
// void UI_Printf(const char* msg, ...);

// used to check if in the filepath the file extention exist
bool ExtExist(const char * filepath, const char * ext);

static bool gAutoQuit = false;

// entry point for the application
int APIENTRY _tWinMain(
                       HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow
                       )
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    if( FbxString(lpCmdLine) == "-test" ) gAutoQuit = true;

    MSG msg;
    HACCEL hAccelTable;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_UI, szWindowClass, MAX_LOADSTRING);
    UIRegisterClass(hInstance);

    // empty our global file name buffer
    ZeroMemory(gszInputFile,  sizeof(gszInputFile)  ); 
    ZeroMemory(gszOutputFile, sizeof(gszOutputFile) ); 

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_UI));



    // Main message loop:
    while( GetMessage(&msg, NULL, 0, 0) && !gAutoQuit )
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
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

    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_UI));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = CreateSolidBrush( GetSysColor(COLOR_3DFACE) );
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_UI);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_UI));

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
        1250,                                  // int nWidth
        500,                                  // int nHeight
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

			// Initialize Kinect Sensor
			InitializeDefaultSensor(&gKinectSensor);

			// Associate the Kinect Exporter with the FBX SDK mmanager
			kExporter->initFBXSDKManager(gSdkManager);

			// Attach visualizer to window VIEW_STATUS
			// get the HWND of the editbox
			HWND hWndView = GetDlgItem(hWnd, VIEW_STATUS);
			HRESULT hr = kVisualizer->attach(hWndView); // attach it to a screen
			if (FAILED(hr)) {
				UI_Printf("Failed to attach to Kinect Viewing Window.");
			}

			// Associate frame reader Kinect frame processor
			kFrameProcessor.init(gKinectSensor);

			// kExporter and kVisualizer need to subscribe to the frame processor
			// this way they will receive frames
			kFrameProcessor.subscribe(std::dynamic_pointer_cast<KBodyReader>(kExporter));
			kFrameProcessor.subscribe(std::dynamic_pointer_cast<KBodyReader>(kVisualizer));

        }
        break;
	case WM_PAINT:
		kVisualizer->update();
		break;

    case WM_COMMAND:

        wmId    = LOWORD(wParam);
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

        case EXPORT_TO_BUTTON:
			GetOutputFileName(hWnd);
			kExporter->setExportFile(gszOutputFile);
            break;

		case RECORD_BUTTON:
            kExporter->startRecording();
			UI_Printf("Recording has been enabled");
            break;

		case STOP_RECORD_BUTTON:
			UI_Printf("Recording has been disabled");
			kExporter->stopRecording();
			break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;

    case WM_DESTROY:

        // dont forget to delete the SdkManager 
        // and all objects created by the SDK manager
        DestroySdkObjects(gSdkManager, true);

		// Close Kinect Sensor
		CloseDefaultSensor(gKinectSensor);

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

            HICON hIconFBX = (HICON) LoadImage( hinst, MAKEINTRESOURCE(IDI_UI), IMAGE_ICON, 16, 16, LR_DEFAULTSIZE); 
            ::SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM) hIconFBX); 
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
    DWORD dwStyle = WS_CHILD | WS_VISIBLE;

    // create the <Import from> button
    CreateWindowEx( 
        0,                          // DWORD dwExStyle,
        "BUTTON",                   // LPCTSTR lpClassName
        "Export to ...",          // LPCTSTR lpWindowName / control caption
        dwStyle,                    // DWORD dwStyle
        10,                         // int x
        10,                         // int y
        130,                        // int nWidth
        30,                         // int nHeight    
        hWndParent,                 // HWND hWndParent
        (HMENU) EXPORT_TO_BUTTON, // HMENU hMenu or control's ID for WM_COMMAND 
        hInst,                      // HINSTANCE hInstance
        NULL                        // LPVOID lpParam
        );

	// create the <Record> button
	CreateWindowEx(
		0,                          // DWORD dwExStyle,
		"BUTTON",                   // LPCTSTR lpClassName
		"Start Recording",            // LPCTSTR lpWindowName / control caption
		dwStyle,                    // DWORD dwStyle
		10,                         // int x
		50,                         // int y
		130,                        // int nWidth
		30,                         // int nHeight    
		hWndParent,                 // HWND hWndParent
		(HMENU) RECORD_BUTTON,   // HMENU hMenu or control's ID for WM_COMMAND 
		hInst,                      // HINSTANCE hInstance
		NULL                        // LPVOID lpParam
		);

    // create the <Stop Recording> button
    CreateWindowEx( 
        0,                          // DWORD dwExStyle,
        "BUTTON",                   // LPCTSTR lpClassName
        "Stop Recording",            // LPCTSTR lpWindowName / control caption
        dwStyle,                    // DWORD dwStyle
        10,                         // int x
        90,                         // int y
        130,                        // int nWidth
        30,                         // int nHeight    
        hWndParent,                 // HWND hWndParent
        (HMENU) STOP_RECORD_BUTTON,   // HMENU hMenu or control's ID for WM_COMMAND 
        hInst,                      // HINSTANCE hInstance
        NULL                        // LPVOID lpParam
        );

    // create the <Import from> edit box
    CreateWindowEx( 
        WS_EX_STATICEDGE,               // DWORD dwExStyle,
        "EDIT",                         // LPCTSTR lpClassName
        " <- select a file to export",  // LPCTSTR lpWindowName / control caption
        dwStyle|ES_AUTOHSCROLL,         // DWORD dwStyle
        150,                            // int x
        15,                             // int y
        530,                            // int nWidth
        20,                             // int nHeight    
        hWndParent,                     // HWND hWndParent
        (HMENU) IMPORT_FROM_EDITBOX,    // HMENU hMenu or control's ID for WM_COMMAND 
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
        50,                                 // int y
        530,                                // int nWidth
        350,                                // int nHeight    
        hWndParent,                         // HWND hWndParent
        (HMENU) EXECUTE_STATUS,             // HMENU hMenu or control's ID for WM_COMMAND 
        hInst,                              // HINSTANCE hInstance
        NULL                                // LPVOID lpParam
        );


	// create the <View Status> edit box
	CreateWindowEx(
		WS_EX_STATICEDGE,                   // DWORD dwExStyle,
		"STATIC",                             // LPCTSTR lpClassName
		"",                                 // LPCTSTR lpWindowName / control caption
		dwStyle | SS_OWNERDRAW | SS_ICON,             // DWORD dwStyle
		700,                                // int x
		15,                                 // int y
		500,                                // int nWidth
		400,                                // int nHeight    
		hWndParent,                         // HWND hWndParent
		(HMENU) VIEW_STATUS,             // HMENU hMenu or control's ID for WM_COMMAND 
		hInst,                              // HINSTANCE hInstance
		NULL                                // LPVOID lpParam
		);
}

// show the <Open file> dialog
void GetInputFileName(
                      HWND hWndParent
                      )
{
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    char szFile[_MAX_PATH];  // buffer for file name
    ZeroMemory(szFile, sizeof(szFile));

    // Initialize OPENFILENAME
    ofn.lStructSize     = sizeof(ofn);
    ofn.hwndOwner       = hWndParent;
    ofn.lpstrFile       = szFile;
    ofn.nMaxFile        = sizeof(szFile);
    ofn.nFilterIndex    = 1;
    ofn.lpstrFileTitle  = NULL;
    ofn.nMaxFileTitle   = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle      = "Select the file to import from ... (use the file type filter)";
    ofn.Flags           = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // get a description of all readers registered in the FBX SDK
    const char *filter  = GetReaderOFNFilters(); 
    ofn.lpstrFilter     = filter;

    // Display the Open dialog box. 
    if(GetOpenFileName(&ofn) == false)
    {
        // user cancel
        delete filter;
        return;
    }

    delete filter;

    // show the file name selected
    SetWindowText( GetDlgItem(hWndParent, IMPORT_FROM_EDITBOX),  szFile );

    // Keep a copy of the file name
    FBXSDK_strcpy(gszInputFile, _MAX_PATH, szFile);
}

// show the <Save file> dialog
void GetOutputFileName(
                       HWND hWndParent
                       )
{
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    char szFile[_MAX_PATH];  // buffer for file name
    ZeroMemory(szFile, sizeof(szFile));

    // Initialize OPENFILENAME
    ofn.lStructSize     = sizeof(ofn);
    ofn.hwndOwner       = hWndParent;
    ofn.lpstrFile       = szFile;
    ofn.nMaxFile        = sizeof(szFile)/ sizeof(*szFile); 
    ofn.nFilterIndex    = 1;      // *.fbx binairy by default
    ofn.lpstrFileTitle  = NULL;
    ofn.nMaxFileTitle   = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle      = "Select the file to export to ... (use the file type filter)";
    ofn.Flags           = OFN_EXPLORER | OFN_OVERWRITEPROMPT;

    // get a description of all writers registered in the FBX SDK
    const char *filter  = GetWriterSFNFilters();
    ofn.lpstrFilter     = filter;

    // Display the save as dialog box. 
    if(GetSaveFileName(&ofn) == false)
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
    const char * ext = GetFileFormatExt( gWriteFileFormat );

    // check for file extention
    if(ExtExist(szFile, ext) == false)
    {
        // add the selected file extention
        FBXSDK_strcat(szFile, _MAX_PATH, ext);
    }

    delete ext;

    // show the file name selected with the extention
    SetWindowText( GetDlgItem(hWndParent, DECOMPRESS_TO_EDITBOX), szFile);

    // Keep a copy of the file name
    FBXSDK_strcpy(gszOutputFile, _MAX_PATH, szFile);
}
