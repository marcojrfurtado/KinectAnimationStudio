#include "KBodyReader.h"

/// <summary>
/// Constructor
/// </summary>
KBodyReader::KBodyReader(IKinectSensor *kSensor) :
m_pCoordinateMapper(NULL),
m_pBodyReadStatus(false),
m_nPreviousFrameTime(0),
_m_pbodyUpdateMutex(NULL)
{
	// Allocate new mutex, necessary for synchronizing multiple readers
	_m_pbodyUpdateMutex = new std::timed_mutex;

	// Clear body data
	for (int i = 0; i < _countof(m_ppBodies); ++i)
	{
		m_ppBodies[i] = nullptr;
	}
}

/// <summary>
/// Destructor
/// </summary>
KBodyReader::~KBodyReader() {

	for (int i = 0; i < _countof(m_ppBodies); ++i)
	{
		SafeRelease(m_ppBodies[i]);
	}

	// Finalize lock
	if ( _m_pbodyUpdateMutex ) 
		delete _m_pbodyUpdateMutex;
}


/// <summary>
/// Notify class about a frame that arrived
/// </summary>
/// <param name="bFrame">Incoming frame</param>
void  KBodyReader::notify(IBodyFrame *bFrame, INT64 frameTime) {
	HRESULT hr;

	// Lock mutex when running this method
	std::unique_lock<std::timed_mutex> lockB(*_m_pbodyUpdateMutex, std::defer_lock);
	if (!lockB.try_lock_for(std::chrono::seconds(1)))
		return;

	
	// ___ Entering sensitive area
	m_tlatestFrameTime = frameTime;
	hr = bFrame->GetAndRefreshBodyData(_countof(m_ppBodies), m_ppBodies);
	if (SUCCEEDED(hr))
		m_pBodyReadStatus = true;
	else
		m_pBodyReadStatus = false;
	// ___ Leaving sensitive area

}