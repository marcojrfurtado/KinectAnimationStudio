#include "Kinect_helpers.h"



/// <summary>
/// Initializes the default Kinect sensor
/// </summary>
/// <returns>indicates success or failure</returns>
HRESULT InitializeDefaultSensor(IKinectSensor **kSensor) {

	HRESULT hr;

	hr = GetDefaultKinectSensor(kSensor);
	if (FAILED(hr))
	{
		return hr;
	}
	
	if (!(*kSensor))
	{
		UI_Printf("No ready Kinect found!");
		return E_FAIL;
	} else {
		hr = (*kSensor)->Open(); // Open Sensor
	}

	return hr;

}


/// <summary>
/// Close the default Kinect sensor
/// </summary>
/// <returns>indicates success or failure</returns>
HRESULT CloseDefaultSensor(IKinectSensor *kSensor) {
	
	HRESULT hr = S_OK;
	// close the Kinect Sensor
	if (kSensor)
	{
		hr = kSensor->Close();
	}
	return hr;
}


HRESULT RetrieveKinectSensorStructures(IKinectSensor *kSensor,ICoordinateMapper **coordinateMapper, IBodyFrameReader **bodyFrameReader) {
	HRESULT hr;
	BOOLEAN isOpen;
	hr = kSensor->get_IsOpen(&isOpen);
	if (gKinectSensor && SUCCEEDED(hr) && isOpen )
	{
		// Initialize the Kinect and get coordinate mapper and the body reader
		IBodyFrameSource* pBodyFrameSource = NULL;

		hr = kSensor->Open();

		if (SUCCEEDED(hr) && coordinateMapper)
		{
			hr = kSensor->get_CoordinateMapper(coordinateMapper);
		}

		if (SUCCEEDED(hr))
		{
			hr = kSensor->get_BodyFrameSource(&pBodyFrameSource);
		}

		if (SUCCEEDED(hr) && bodyFrameReader)
		{
			hr = pBodyFrameSource->OpenReader(bodyFrameReader);
		}

		pBodyFrameSource->Release();
	}
	return hr;
}