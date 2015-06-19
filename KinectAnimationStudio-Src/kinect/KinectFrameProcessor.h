#pragma once

#include "..\common\stdafx.h"
#include "kinect_typedef.h"

#include "KBodyReader.h"

class KinectFrameProcessor {
public:
	/// <summary>
	/// Constructor
	/// </summary>
	KinectFrameProcessor(IKinectSensor *kSensor = NULL);

	/// <summary>
	/// Destructor
	/// </summary>
	~KinectFrameProcessor();

	/// <summary>
	/// Initializes this object, by attaching a sensor to it
	/// </summary>
	/// <param name="kSensor" >Sensor that frame processor will be attached to</param>
	HRESULT init(IKinectSensor *kSensor);


	/// <summary>
	/// Returns kinect coordinate mapper
	/// </summary>
	ICoordinateMapper* getCoordinateMapper() { return m_pCoordinateMapper; };

	/// <summary>
	/// Returns kinect frame reader
	/// </summary>
	IBodyFrameReader* getBodyFrameReader() { return m_pBodyFrameReader; };

	/// <summary>
	/// Stops frame reader
	/// </summary>
	void stop() { quitMain = false; };

	/// <summary>
	/// Allow other classes to subscribe to our method,
	/// so anytime a frame arrives, they are signaled
	/// </summary>
	void subscribe(const KReader_ptr &subscriber); 

	/// <summary>
	/// All subscribers are removed from the list
	/// </summary>
	void unsubscribeAll();


private:
	// Current Kinect
	IKinectSensor*          m_pKinectSensor;
	ICoordinateMapper*      m_pCoordinateMapper;

	// Body reader
	IBodyFrameReader*       m_pBodyFrameReader;

	// Frame Event Listener
	WAITABLE_HANDLE m_hFrameEvent;

	// Main processing thread
	std::future<bool> m_pmainThread;

	// Quit processing thread
	std::atomic_bool quitMain;

	// List of subscribers
	std::list<std::shared_ptr<KBodyReader>> subscriberList;

	// Mutex for the list
	std::timed_mutex *m_pSubscriberListMutex;


	/// <summary>
	/// Frame processing method
	/// </summary>
	void Process();

};