#pragma once


#include "..\common\stdafx.h"
#include "KBodyReader.h"

class KBodyExporter : public KBodyReader {
public:

	/// <summary>
	/// Constructor
	/// </summary>
	KBodyExporter(IKinectSensor *kSensor = NULL, FbxManager *fManager = NULL);

	/// <summary>
	/// Destructor
	/// </summary>
	~KBodyExporter();


	/// <summary>
	/// Sets SDK Manager
	/// </summary>
	/// <param name="fManager">SDK manager to be set</param>
	void initFBXSDKManager(FbxManager *fManager) {
		m_pFBXManager = fManager;
	}

	/// <summary>
	/// Starts recording Skeleton Data to FBX
	/// </summary>
	void startRecording(); 

	/// <summary>
	/// Stops recording Skeleton Data to FBX
	/// </summary>
	void stopRecording();


	/// <summary>
	/// Returns whether we are currently recording the skeletons
	/// </summary>
	bool recordingStatus()  { return m_pIsRecording; };

	/// <summary>
	/// Notify class about a frame that arrived
	/// </summary>
	/// <param name="bFrame">Incoming frame</param>
	/// <param name="frameTime">Frame timestamp</param>
	virtual void notify(IBodyFrame *bFrame, INT64 frameTime);

	/// <summary>
	/// Sets export file, which will be overwritten
	/// </summary>
	/// <param name="fileName">Name of the file to be written</param>
	void setExportFile(char *fieName);


	/// <summary>
	/// Save bodies of the current frame to the scene
	/// </summary>
	void addBodiesToScene();

private:

	// Constants
	const char *c_defaultExportFileName = "output.fbx";
	const char *c_FBXBinaryFileDesc = "FBX binary(*.fbx)";
	// Kinect FPS ( Kinect V2 records at 30fps )
	const int c_KinectFPS = 30;


	// Variables

	bool m_pIsRecording;

	// FBX SDK Manager
	FbxManager *m_pFBXManager;

	// FBX Scene - We only export one scene at a time
	FbxScene* m_lScene;

	// Frame Event Listener
	WAITABLE_HANDLE m_hFrameEvent;


	// Number of frames processed for current scene
	unsigned int m_nRecordCount;

	// Initial timestamp
	INT64 m_initTime;


	// Export file
	char *m_exportFileName;

	/// <summary>
	/// Keep reading new frames to be added to the scene
	/// </summary>
	void InternalFrameProcessingThread();

	/// <summary>
	/// Save anything that has been recorded so far to output file
	/// </summary>
	void flushScene();



};