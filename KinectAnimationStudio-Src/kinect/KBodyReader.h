#pragma once

#include "..\common\stdafx.h"


// Main class that processes Skeleton data
class KBodyReader {

public:
	/// <summary>
	/// Constructor
	/// </summary>
	KBodyReader::KBodyReader(IKinectSensor *kSensor = NULL);


	/// <summary>
	/// Destructor
	/// </summary>
	KBodyReader::~KBodyReader();



	/// <summary>
	/// Notify class about a frame that arrived
	/// </summary>
	/// <param name="bFrame">Incoming frame</param>
	/// <param name="frameTime">Incoming frame time</param>
	virtual void  notify(IBodyFrame *bFrame, INT64 frameTime = 0);

	/// <summary>
	/// Sets coordinate mapper
	/// </summary>
	/// <param name="cMapper">Coordinater to be set</param>
	void setCoordinateMapper(ICoordinateMapper* cMapper) { m_pCoordinateMapper = cMapper;  };

protected:

	// Coordinate Mapper
	ICoordinateMapper*      m_pCoordinateMapper;

	// Body Array
	IBody* m_ppBodies[BODY_COUNT];

	// Body update mutex
	std::timed_mutex *_m_pbodyUpdateMutex;

	// Latest frame time
	INT64 m_tlatestFrameTime;

	// Frame time of the previously processed frame
	INT64 m_nPreviousFrameTime;

	// Body Read Status
	bool m_pBodyReadStatus;



};