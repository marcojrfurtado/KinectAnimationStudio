#pragma once

#include "..\stdafx.h"


// Node property
#define FBX_CUSTOM_ID_PROPERTY_LABEL "CustomId"

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
/// Applies unroll filter to the skeleton tree, recursively
/// </summary>
/// <param name="filter">FBX  Unroll filter instance</param>
/// <param name="fNode">Current FBX  node</param>
void applyUnrollFilterHierarchically(FbxAnimCurveFilterUnroll &filter, FbxNode *fNode);


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