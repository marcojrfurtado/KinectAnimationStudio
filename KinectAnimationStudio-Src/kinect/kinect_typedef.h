#pragma once

#include "..\common\stdafx.h"
#include "KBodyReader.h"
#include "KBodyExporter.h"
#include "KBodyVisualizer.h"

/*
Type definitinons
*/
typedef std::shared_ptr<KBodyReader> KReader_ptr;
typedef std::shared_ptr<KBodyExporter> KExporter_ptr;
typedef std::shared_ptr<KBodyVisualizer> KVisualizer_ptr;