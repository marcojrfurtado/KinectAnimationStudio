#pragma once

#include "..\stdafx.h"

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