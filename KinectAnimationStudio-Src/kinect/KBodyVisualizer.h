#pragma once


#include "..\common\stdafx.h"
#include "..\UI\resource.h"
#include "KBodyReader.h"

#include <dwrite.h>


// Global consts
static const WCHAR msc_fontName[] = L"Verdana";
static const FLOAT msc_fontSize = 12;

class KBodyVisualizer : public KBodyReader {

public:
	/// <summary>
	/// Constructor
	/// </summary>
	/// <param name="kSensor">Default Kinect Sensor</param>
	KBodyVisualizer(IKinectSensor *kSensor = NULL);

	/// <summary>
	/// Destructor
	/// </summary>
	~KBodyVisualizer();

	/// <summary>
	/// Initializes the desired window and begins processing
	/// </summary>
	/// <param name="hWnd"></param>
	HRESULT   attach(HWND hWnd);

	/// <summary>
	/// Main update function, will redraw anything
	/// </summary>
	HRESULT update();




private:
	// Constants
	const float c_JointThickness = 3.0f;
	const float c_TrackedBoneThickness = 6.0f;
	const float c_InferredBoneThickness = 1.0f;

	const int        cDepthWidth = 512;
	const int        cDepthHeight = 424;


	// Variables
	HWND                    m_hWnd;
	INT64                   m_nStartTime;
	INT64                   m_nLastCounter;
	double                  m_fFreq;
	INT64                   m_nNextStatusTime;
	DWORD                   m_nFramesSinceUpdate;



	// D2D Factory
	ID2D1Factory* m_pD2DFactory;
	// D2D Write Factory
	IDWriteFactory* m_pD2DWriteFactory;
	// D2D render target
	ID2D1HwndRenderTarget* m_pRT;
	// Size of the drawing area
	RECT m_rc;

	// D2D Brushes
	ID2D1SolidColorBrush* m_redBrush;
	ID2D1SolidColorBrush* m_pBrushJointTracked;
	ID2D1SolidColorBrush* m_pBrushJointInferred;
	ID2D1SolidColorBrush* m_pBrushBoneTracked;
	ID2D1SolidColorBrush* m_pBrushBoneInferred;

	// D2D text format
	IDWriteTextFormat* m_pTextFormat;

	/// <summary>
	/// Main kinect processing function
	/// </summary>
	void ReceiveBodiesInfo();

	/// <summary>
	/// Handle new body data
	/// <param name="nTime">timestamp of frame</param>
	/// <param name="nBodyCount">body data count</param>
	/// <param name="ppBodies">body data in frame</param>
	/// </summary>
	void ProcessBody(INT64 nTime, int nBodyCount, IBody** ppBodies);

	/// <summary>
	/// Converts a body point to screen space
	/// </summary>
	/// <param name="bodyPoint">body point to tranform</param>
	/// <param name="width">width (in pixels) of output buffer</param>
	/// <param name="height">height (in pixels) of output buffer</param>
	/// <returns>point in screen-space</returns>
	D2D1_POINT_2F BodyToScreen(const CameraSpacePoint& bodyPoint, int width, int height);

	/// <summary>
	/// Draws a body 
	/// </summary>
	/// <param name="pJoints">joint data</param>
	/// <param name="pJointPoints">joint positions converted to screen space</param>
	void DrawBody(const Joint* pJoints, const D2D1_POINT_2F* pJointPoints);

	/// <summary>
	/// Draws one bone of a body (joint to joint)
	/// </summary>
	/// <param name="pJoints">joint data</param>
	/// <param name="pJointPoints">joint positions converted to screen space</param>
	/// <param name="pJointPoints">joint positions converted to screen space</param>
	/// <param name="joint0">one joint of the bone to draw</param>
	/// <param name="joint1">other joint of the bone to draw</param>
	void DrawBone(const Joint* pJoints, const D2D1_POINT_2F* pJointPoints, JointType joint0, JointType joint1);



	/// <summary>
	/// Set the status bar message
	/// </summary>
	/// <param name="pMsg">message to display</param>
	/// <param name="showTimeMsec">time in milliseconds to ignore future status messages</param>
	/// <param name="bForce">force status update</param>
	bool SetStatusMessage(_In_z_ WCHAR* pMsg, DWORD64 nShowTimeMsec, bool bForce);


};