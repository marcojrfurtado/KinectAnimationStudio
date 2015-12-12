#include "KBodyExporter.h"


/// <summary>
/// Constructor
/// </summary>
KBodyExporter::KBodyExporter(IKinectSensor *kSensor, FbxManager *fManager) :
m_pIsRecording(false),
m_pFBXManager(NULL),
m_lScene(NULL),
m_nRecordCount(0),
m_initTime(0),
m_exportFileName(NULL),
KBodyReader(kSensor)
{
	if (fManager) {
		initFBXSDKManager(fManager);
	}
}

/// <summary>
/// Destructor
/// </summary>
KBodyExporter::~KBodyExporter() {

	// Clear export file name
	if (m_exportFileName) {
		free(m_exportFileName);
		m_exportFileName = NULL;
	}

	// output any frame to file
	flushScene();
}

/// <summary>
/// Starts recording Skeleton Data to FBX
/// </summary>
void KBodyExporter::startRecording() {
	m_pIsRecording = true;


	// Create a scene
	m_lScene = FbxScene::Create(m_pFBXManager, "");

	// Create Animation Stack
	FbxAnimStack* lAnimStack = FbxAnimStack::Create(m_lScene, "Base animation");

	// The animation nodes can only exist on AnimLayers therefore it is mandatory to
	// add at least one AnimLayer to the AnimStack. And for the purpose of this example,
	// one layer is all we need.
	FbxAnimLayer* lAnimLayer = FbxAnimLayer::Create(m_lScene, "Base Layer");
	lAnimStack->AddMember(lAnimLayer);
};

/// <summary>
/// Stops recording Skeleton Data to FBX
/// </summary>
void KBodyExporter::stopRecording() {
	// Stop Recording
	m_pIsRecording = false;

	// Save scene
	flushScene();
};


/// <summary>
/// Sets export file, which will be overwritten
/// </summary>
/// <param name="fileName">Name of the file to be written</param>
void KBodyExporter::setExportFile(char *fieName) {
	// If replacing old filename, make sure it is freed
	if (m_exportFileName)
		free(m_exportFileName);

	m_exportFileName = _strdup(fieName);
}

/// <summary>
/// Notify class about a frame that arrived
/// </summary>
/// <param name="bFrame">Incoming frame</param>
/// <param name="frameTime">Frame timestamp</param>
void KBodyExporter::notify(IBodyFrame *bFrame, INT64 frameTime) {
	// Not recording, ignore frame
	if (!m_pIsRecording)
		return;

	// Use the general notifier first, it will save the bodies of the current frame
	KBodyReader::notify(bFrame, frameTime);


	// Take each one of the bodies that has been read, and add them to the scene
	addBodiesToScene();
}

/// <summary>
/// Save anything that has been recorded so far to output file
/// </summary>
void KBodyExporter::flushScene() {

	// If there is no scene, nothing to be saved
	if (!m_lScene)
		return;


	// Only save if at least one frame has been read, otherwise the FBX would be an empty scene
	if (m_nRecordCount > 0) {

		// Apply post processing filters
		KinectSkeletonMapper::applyPostProcessingFilters(m_lScene);

		// Get File Format
		int lFileFormat = m_pFBXManager->GetIOPluginRegistry()->FindReaderIDByDescription(c_FBXBinaryFileDesc);

		// Define export file name ( If user did not define it, use a default file name )
		const char *outputFile;
		if (m_exportFileName)
			outputFile = m_exportFileName;
		else
			outputFile = c_defaultExportFileName;

		SaveScene(m_pFBXManager, m_lScene, outputFile, lFileFormat, false);

		// Warn the user about the file
		UI_Printf("Scene has been saved to file %s", outputFile);
	}

	// Scene has been saved (or not), now clean it up
	m_lScene->Destroy();
	m_lScene = NULL;
};

/// <summary>
/// Save bodies of the current frame to the scene
/// </summary>
void KBodyExporter::addBodiesToScene() {


	// If failed to read bodies for the last frame, just skip everything
	if (!m_pBodyReadStatus)
		return;


	// Process each body individually
	for (int i = 0; i < BODY_COUNT; ++i)
	{
		IBody* pBody = m_ppBodies[i];
		if (pBody)
		{
			BOOLEAN isTracked;
			HRESULT hr =  pBody->get_IsTracked(&isTracked);


			if (SUCCEEDED(hr) && isTracked) {
				// Kinect clock works in increments of 100ns. Who the hell needs documentation, let people figure it out.
				INT64 timeMS = m_tlatestFrameTime / 10000;

				if (m_initTime == 0)
					m_initTime = timeMS;

				KinectSkeletonMapper::map(m_lScene, timeMS - m_initTime + 1, pBody);
			}
		}
	}

	// Update frame count
	m_nRecordCount++;
};