#pragma once

#include "..\stdafx.h"


// Global kinect sensor ( extern variable )
extern IKinectSensor* gKinectSensor;


/// <summary>
/// Initializes the default Kinect sensor
/// </summary>
/// <returns>S_OK on success, otherwise failure code</returns>
HRESULT                 InitializeDefaultSensor(IKinectSensor **kSensor = &gKinectSensor);


/// <summary>
/// Close the default Kinect sensor
/// </summary>
/// <returns>indicates success or failure</returns>
HRESULT CloseDefaultSensor(IKinectSensor *kSensor = gKinectSensor);



/// <summary>
/// Get structures from default kinect sensor
/// </summary>
/// <param name="kSensor">Sensor from which structure will be extracted from</param>
/// <param name="coordinateMapper">Output coordinate mapper for this sensor</param>
/// <param name="bodyFrameReader">Output body frame reader for this sensor</param>
/// <returns>S_OK on success, otherwise failure code</returns>
HRESULT		RetrieveKinectSensorStructures(IKinectSensor *kSensor, ICoordinateMapper **coordinateMapper, IBodyFrameReader **bodyFrameReader);
