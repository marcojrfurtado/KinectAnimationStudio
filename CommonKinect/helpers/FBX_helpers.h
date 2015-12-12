#pragma once

#include "..\stdafx.h"


// Node property
#define FBX_CUSTOM_ID_PROPERTY_LABEL "CustomId"
#define FBX_TRANS_SCALING_PROPERTY_LABEL "TranslationScale"

// Global FBX manager ( extern variable )
extern FbxManager*   gSdkManager;

void InitializeSdkManager();

void DestroySdkObjects(
	FbxManager* pSdkManager,
	bool pExitStatus
	);

const char *GetReaderOFNFilters();

const char *GetWriterSFNFilters();

const char *GetFileFormatExt(
	const int pWriteFileFormat
	);

bool LoadScene(
	FbxManager* pSdkManager,
	FbxScene* pScene,
	const char* pFilename
	);

bool SaveScene(
	FbxManager* pSdkManager,
	FbxScene* pScene,
	const char* pFilename,
	int pFileFormat,
	bool pEmbedMedia
	);


// Filtering methods
/// <summary>
/// Applies filter to the skeleton tree, recursively
/// </summary>
/// <param name="filter">FBX  filter instance</param>
/// <param name="fNode">Current FBX  node</param>
void applyFilterHierarchically(FbxAnimCurveFilter &filter, FbxNode *fNode);


/// <summary>
/// Get number of keys from rotation curve of a given joints
/// </summary>
/// <param name="tgtNode">Node to have info extracted</param>
/// <param name="lScene">FBX Scene</param>
int getKeyCount(FbxNode *tgtNode, FbxScene *lScene);

/// <summary>
/// Checks if joint is animatable
/// </summary>
/// <param name="tgtNode">Node to be checked</param>
bool isAnimatable(FbxNode *tgtNode);


/// <summary>
/// Gets custom ID from a FBX NODE
/// </summary>
int getCustomIdProperty(FbxNode *fNode);

/// <summary>
/// Sets custom ID for a FBX NODE
/// </summary>
void setCustomIdProperty(FbxNode *fNode, int newId);

/// <summary>
/// Gets translation scale from a FBX NODE
/// </summary>
float getTranslationScaleProperty(FbxNode *fNode);

/// <summary>
/// Sets translation scale for a FBX NODE
/// </summary>
void setTranslationScaleProperty(FbxNode *fNode, float val);


/// <summary>
/// Computes keyframe rate
/// </summary>
double computeFPS(FbxAnimCurve *tgtCurve);

/// <summary>
/// Computes time with offset
/// </summary>
double computeOffsetTime(FbxLongLong currentTime, int offset, double fps);


/// <summary>
/// Inserts new key into animation curve
/// </summary>
void insertKeyCurve(FbxAnimCurve* tgtCurve, FbxTime keyTime, float keyVal, bool isTranslation);




/// <summary>
///Returns true if node has any curves for which there are keys past kTime
/// </summary>
bool hasMoreKeys(FbxTime kTime, FbxNode *refNode, FbxAnimLayer *layer);

/// <summary>
///Returns true if curve has keys past kTime
/// </summary>
bool hasMoreKeys(FbxTime kTime, FbxAnimCurve *curve);

/// <summary>
///Returns true if keys exist on curve at kTime
/// </summary>
bool hasKeysAt(FbxAnimCurve *curve, FbxTime kTime);

bool hasKeysAt(FbxAnimLayer *pLayer, FbxNode *tgtNode, FbxTime kTime, bool isTranslation=false);

FbxDouble3 getKeyValueFromMarker(FbxNode *vMarker, FbxAnimLayer *pLayer, int keyIndex);
FbxTime getKeyTimeFromMarker(FbxNode *vMarker, FbxAnimLayer *pLayer, int keyIndex);

FbxTime getKeyTimeFromCurve(FbxAnimCurve*curve, int keyIndex);

/// <summary>
/// Creates key on Curve based on transformation vector
/// </summary>
/// <param name="tgtCurve">Fbx Curve to have key added to</param>
/// <param name="kTime">Time for transformation</param>
/// <param name="kVal">Key value</param>
/// <param name="interpolationType">Interpolation to be used by key ( defaults to cubic)</param>
void applyTransformationVectorToCurve(FbxAnimCurve *tgtCurve, FbxTime kTime, float kVal, FbxAnimCurveDef::EInterpolationType interpolationType = FbxAnimCurveDef::eInterpolationCubic);

/// <summary>
/// Applies transformation to node
/// </summary>
/// <param name="pLayer">Fbx Anim Layer</param>
/// <param name="node">Fbx Node</param>
/// <param name="transf">Transformation</param>
/// <param name="time">Transformation time</param>
/// <param name="isTransl">Is transformation about translation?</param>
/// <param name="interpolationType">Interpolation to be used by key ( defaults to cubic)</param>
void applyTransformation(FbxAnimLayer *pLayer, FbxNode *node, FbxDouble3 transf, FbxTime time, bool isTransl = false, FbxAnimCurveDef::EInterpolationType interpolationType = FbxAnimCurveDef::eInterpolationCubic);

FbxMatrix relocateTranslation(FbxMatrix inputM);

FbxAMatrix toAffine(FbxMatrix m);

/// <summary>
/// Converts axis angle to quaternion
/// </summary>
FbxQuaternion axisAngleToQuat(FbxVector4 axisAngle);