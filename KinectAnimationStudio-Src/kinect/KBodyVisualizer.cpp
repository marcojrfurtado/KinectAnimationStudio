#include "KBodyVisualizer.h"
#include "..\common\stdafx.h"
#include <strsafe.h>

/// <summary>
/// Constructor
/// </summary>
KBodyVisualizer::KBodyVisualizer(IKinectSensor *kSensor) :
m_hWnd(NULL),
m_nStartTime(0),
m_nLastCounter(0),
m_nFramesSinceUpdate(0),
m_fFreq(0),
m_nNextStatusTime(0LL),
m_pD2DFactory(NULL),
m_pD2DWriteFactory(NULL),
m_pRT(NULL),
m_pBrushBoneInferred(NULL),
m_pBrushBoneTracked(NULL),
m_pBrushJointInferred(NULL),
m_pBrushJointTracked(NULL),
m_redBrush(NULL),
m_pTextFormat(NULL),
KBodyReader(kSensor)
{

	HRESULT hr = D2D1CreateFactory(
		D2D1_FACTORY_TYPE_SINGLE_THREADED,
		&m_pD2DFactory
		);

	if (SUCCEEDED(hr)) {
		hr = DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(m_pD2DWriteFactory),
			reinterpret_cast<IUnknown **>(&m_pD2DWriteFactory)
			);
	}

	if (SUCCEEDED(hr)) {
		// Query performance results
		LARGE_INTEGER qpf = { 0 };
		if (QueryPerformanceFrequency(&qpf))
		{
			m_fFreq = double(qpf.QuadPart);
		}
	}

}



/// <summary>
/// Destructor
/// </summary>
KBodyVisualizer::~KBodyVisualizer()
{
	// Release D2D structure
	SafeRelease(m_pBrushBoneInferred);
	SafeRelease(m_pBrushBoneTracked);
	SafeRelease(m_pBrushJointInferred);
	SafeRelease(m_pBrushJointTracked);
	SafeRelease(m_redBrush);
	SafeRelease(m_pTextFormat);
	SafeRelease(m_pRT);
	SafeRelease(m_pD2DWriteFactory);
	SafeRelease(m_pD2DFactory);
}



/// <summary>
/// Attaches body visualizer to the desired window and initializes D2D structures
/// </summary>
/// <param name="hInstance">handle to the application instance</param>
HRESULT KBodyVisualizer::attach(HWND hWnd)
{
	HRESULT hr = S_OK;


	// Save Window handle
	m_hWnd = hWnd;

	// Initalize D2D factory, if necessary
	if (m_pD2DFactory == NULL) {
		hr = D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED,
			&m_pD2DFactory
			);
		if (FAILED(hr)) {
			return hr;
		}
	}

	// Obtain the size of the drawing area.
	GetClientRect(m_hWnd, &m_rc);

	// Create a Direct2D render target			
	if ( SUCCEEDED(hr) && m_pRT == NULL) {
		hr = m_pD2DFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(
			m_hWnd,
			D2D1::SizeU(
			m_rc.right - m_rc.left,
			m_rc.bottom - m_rc.top)
			),
			&m_pRT
			);
	}

	// Create Brushes
	// light green
	m_pRT->CreateSolidColorBrush(D2D1::ColorF(0.27f, 0.75f, 0.27f), &m_pBrushJointTracked);
	m_pRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Yellow, 1.0f), &m_pBrushJointInferred);
	m_pRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Green, 1.0f), &m_pBrushBoneTracked);
	m_pRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Gray, 1.0f), &m_pBrushBoneInferred);
	m_pRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red, 1.0f), &m_redBrush);

	// Create Text Format
	if (SUCCEEDED(hr)) {

		hr = m_pD2DWriteFactory->CreateTextFormat(
			msc_fontName,
			NULL,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			msc_fontSize,
			L"", //locale
			&m_pTextFormat
			);
	}


	if (SUCCEEDED(hr)) {
		// Finally, connect to kinect sensor
	//	hr = InitializeDefaultSensor();
	}

	return hr;

}


/// <summary>
/// Redraw window
/// </summary>
bool KBodyVisualizer::update() {
	PAINTSTRUCT ps;
	BeginPaint(m_hWnd, &ps);
	m_pRT->BeginDraw();

	// Read in Kinect Body information, and process all of them
	ReceiveBodiesInfo();

	m_pRT->EndDraw();
	EndPaint(m_hWnd, &ps);
	return true;
}

/// <summary>
/// Notify class about a frame that arrived
/// </summary>
/// <param name="bFrame">Incoming frame</param>
/// <param name="frameTime">Frame timestamp</param>
void KBodyVisualizer::notify(IBodyFrame *bFrame, INT64 frameTime) {

	// If visualizer is not attached to a window, just return
	if (!m_hWnd)
		return;

	// Use the general notifier first, it will save the bodies of the current frame
	KBodyReader::notify(bFrame, frameTime);

	// Draw skeleton
	update();
}



/// <summary>
/// Main kinect processing function
/// </summary>
void KBodyVisualizer::ReceiveBodiesInfo() {

	// Lock mutex when running this method
	std::unique_lock<std::timed_mutex> lockB(*_m_pbodyUpdateMutex, std::defer_lock);
	// Failed to lock, just return
	if (!lockB.try_lock_for(std::chrono::milliseconds(300)))
		return;
	

	// ___ Entering sensitive area - lock was granted

	// If we still the same frame, don't even bother redisplaying it
	if (m_pBodyReadStatus && (m_tlatestFrameTime > m_nPreviousFrameTime))
	{
		// Update frame time
		m_nPreviousFrameTime = m_tlatestFrameTime;
		
		// Clear background color
		m_pRT->Clear(D2D1::ColorF(D2D1::ColorF::White));

		ProcessBody(m_tlatestFrameTime, BODY_COUNT, m_ppBodies);

		// Release body information after using
		for (int i = 0; i < _countof(m_ppBodies); ++i)
		{
			SafeRelease(m_ppBodies[i]);
		}
	}

	// ___ Leaving sensitive area


}

/// <summary>
/// Handle new body data
/// <param name="nTime">timestamp of frame</param>
/// <param name="nBodyCount">body data count</param>
/// <param name="ppBodies">body data in frame</param>
/// </summary>
void KBodyVisualizer::ProcessBody(INT64 nTime, int nBodyCount, IBody** ppBodies)
{
	if (m_pRT && m_pCoordinateMapper)
	{
		HRESULT hr;

		int width = m_rc.right;
		int height = m_rc.bottom;

		for (int i = 0; i < nBodyCount; ++i)
		{
			IBody* pBody = ppBodies[i];
			if (pBody)
			{
				BOOLEAN bTracked = false;
				hr = pBody->get_IsTracked(&bTracked);

				if (SUCCEEDED(hr) && bTracked)
				{
					Joint joints[JointType_Count];
					D2D1_POINT_2F jointPoints[JointType_Count];
					HandState leftHandState = HandState_Unknown;
					HandState rightHandState = HandState_Unknown;

					pBody->get_HandLeftState(&leftHandState);
					pBody->get_HandRightState(&rightHandState);

					hr = pBody->GetJoints(_countof(joints), joints);
					if (SUCCEEDED(hr))
					{
						for (int j = 0; j < _countof(joints); ++j)
						{
							jointPoints[j] = BodyToScreen(joints[j].Position, width, height);
						}

						DrawBody(joints, jointPoints);
					}
				}
			}


		}

		if (!m_nStartTime)
		{
			m_nStartTime = nTime;
		}

		double fps = 0.0;

		LARGE_INTEGER qpcNow = { 0 };
		if (m_fFreq)
		{
			if (QueryPerformanceCounter(&qpcNow))
			{
				if (m_nLastCounter)
				{
					m_nFramesSinceUpdate++;
					fps = m_fFreq * m_nFramesSinceUpdate / double(qpcNow.QuadPart - m_nLastCounter);
				}

			}

			WCHAR sMessage[100];
			swprintf_s(sMessage, L" FPS = %0.2f  Time = %I64d", fps, nTime - m_nStartTime);
			if (SetStatusMessage(sMessage, 1000, true)) {
				m_nLastCounter = qpcNow.QuadPart;
				m_nFramesSinceUpdate = 0;
			}
		}
	}
}

/// <summary>
/// Converts a body point to screen space
/// </summary>
/// <param name="bodyPoint">body point to tranform</param>
/// <param name="width">width (in pixels) of output buffer</param>
/// <param name="height">height (in pixels) of output buffer</param>
/// <returns>point in screen-space</returns>
D2D1_POINT_2F KBodyVisualizer::BodyToScreen(const CameraSpacePoint& bodyPoint, int width, int height)
{
	// Calculate the body's position on the screen
	DepthSpacePoint depthPoint = { 0 };
	m_pCoordinateMapper->MapCameraPointToDepthSpace(bodyPoint, &depthPoint);

	float screenPointX = static_cast<float>(depthPoint.X * width) / cDepthWidth;
	float screenPointY = static_cast<float>(depthPoint.Y * height) / cDepthHeight;

	return D2D1::Point2F(screenPointX, screenPointY);
}



/// <summary>
/// Draws a body 
/// </summary>
/// <param name="pJoints">joint data</param>
/// <param name="pJointPoints">joint positions converted to screen space</param>
void KBodyVisualizer::DrawBody(const Joint* pJoints, const D2D1_POINT_2F* pJointPoints) 
{
	// Draw the bones

	// Torso
	DrawBone(pJoints, pJointPoints, JointType_Head, JointType_Neck);
	DrawBone(pJoints, pJointPoints, JointType_Neck, JointType_SpineShoulder);
	DrawBone(pJoints, pJointPoints, JointType_SpineShoulder, JointType_SpineMid);
	DrawBone(pJoints, pJointPoints, JointType_SpineMid, JointType_SpineBase);
	DrawBone(pJoints, pJointPoints, JointType_SpineShoulder, JointType_ShoulderRight);
	DrawBone(pJoints, pJointPoints, JointType_SpineShoulder, JointType_ShoulderLeft);
	DrawBone(pJoints, pJointPoints, JointType_SpineBase, JointType_HipRight);
	DrawBone(pJoints, pJointPoints, JointType_SpineBase, JointType_HipLeft);

	// Right Arm    
	DrawBone(pJoints, pJointPoints, JointType_ShoulderRight, JointType_ElbowRight);
	DrawBone(pJoints, pJointPoints, JointType_ElbowRight, JointType_WristRight);
	DrawBone(pJoints, pJointPoints, JointType_WristRight, JointType_HandRight);
	DrawBone(pJoints, pJointPoints, JointType_HandRight, JointType_HandTipRight);
	DrawBone(pJoints, pJointPoints, JointType_WristRight, JointType_ThumbRight);

	// Left Arm
	DrawBone(pJoints, pJointPoints, JointType_ShoulderLeft, JointType_ElbowLeft);
	DrawBone(pJoints, pJointPoints, JointType_ElbowLeft, JointType_WristLeft);
	DrawBone(pJoints, pJointPoints, JointType_WristLeft, JointType_HandLeft);
	DrawBone(pJoints, pJointPoints, JointType_HandLeft, JointType_HandTipLeft);
	DrawBone(pJoints, pJointPoints, JointType_WristLeft, JointType_ThumbLeft);

	// Right Leg
	DrawBone(pJoints, pJointPoints, JointType_HipRight, JointType_KneeRight);
	DrawBone(pJoints, pJointPoints, JointType_KneeRight, JointType_AnkleRight);
	DrawBone(pJoints, pJointPoints, JointType_AnkleRight, JointType_FootRight);

	// Left Leg
	DrawBone(pJoints, pJointPoints, JointType_HipLeft, JointType_KneeLeft);
	DrawBone(pJoints, pJointPoints, JointType_KneeLeft, JointType_AnkleLeft);
	DrawBone(pJoints, pJointPoints, JointType_AnkleLeft, JointType_FootLeft);

	// Draw the joints
	for (int i = 0; i < JointType_Count; ++i)
	{
		D2D1_ELLIPSE ellipse = D2D1::Ellipse(pJointPoints[i], c_JointThickness, c_JointThickness);

		if (pJoints[i].TrackingState == TrackingState_Inferred)
		{
			m_pRT->FillEllipse(ellipse, m_pBrushJointInferred);
		}
		else if (pJoints[i].TrackingState == TrackingState_Tracked)
		{
			m_pRT->FillEllipse(ellipse, m_pBrushJointTracked);
		}
	}
}

/// <summary>
/// Draws one bone of a body (joint to joint)
/// </summary>
/// <param name="pJoints">joint data</param>
/// <param name="pJointPoints">joint positions converted to screen space</param>
/// <param name="pJointPoints">joint positions converted to screen space</param>
/// <param name="joint0">one joint of the bone to draw</param>
/// <param name="joint1">other joint of the bone to draw</param>
void KBodyVisualizer::DrawBone(const Joint* pJoints, const D2D1_POINT_2F* pJointPoints, JointType joint0, JointType joint1)
{
	TrackingState joint0State = pJoints[joint0].TrackingState;
	TrackingState joint1State = pJoints[joint1].TrackingState;

	// If we can't find either of these joints, exit
	if ((joint0State == TrackingState_NotTracked) || (joint1State == TrackingState_NotTracked))
	{
		return;
	}

	// Don't draw if both points are inferred
	if ((joint0State == TrackingState_Inferred) && (joint1State == TrackingState_Inferred))
	{
		return;
	}

	// We assume all drawn bones are inferred unless BOTH joints are tracked
	if ((joint0State == TrackingState_Tracked) && (joint1State == TrackingState_Tracked))
	{
		m_pRT->DrawLine(pJointPoints[joint0], pJointPoints[joint1], m_pBrushBoneTracked, c_TrackedBoneThickness);
	}
	else
	{
		m_pRT->DrawLine(pJointPoints[joint0], pJointPoints[joint1], m_pBrushBoneInferred, c_InferredBoneThickness);
	}
}



bool KBodyVisualizer::SetStatusMessage(_In_z_ WCHAR* pMsg, DWORD64 nShowTimeMsec, bool bForce)
{
	if (!m_pRT)
		return false;

	// Size of canvas
	int width = m_rc.right;
	int height = m_rc.bottom;

	static DWORD64 s_NextStatusTime = 0;

	UINT64 now = GetTickCount();

	if (ghWnd && (bForce || (s_NextStatusTime <= now)))
	{
		m_pRT->DrawTextA(
			pMsg,
			UINT32(wcslen(pMsg)-1),
			m_pTextFormat,
			D2D1::RectF(0.0, 0.0, FLOAT(width), FLOAT(height)),
			m_redBrush);
		s_NextStatusTime = now + nShowTimeMsec;

		return true;
	}

	return false;
}