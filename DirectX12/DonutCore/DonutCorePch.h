#pragma once
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#define WIN32

#include "../DonutCore/Math.h"

#include "../DonutCore/log.h"
#include "../DonutCore/json.h"
#include "../DonutCore/string_utils.h"

#include "../DonutCore/chunk.h"
#include "../DonutCore/chunkFile.h"
#include "../DonutCore/chunkDescs.h"

#include "../DonutCore/circular_buffer.h"

#include "../DonutCore/VFS.h"
#include "../DonutCore/Compression.h"
#include "../DonutCore/TarFile.h"
#include "../DonutCore/WinResFS.h"
#include "../DonutCore/ZipFile.h"

#include "../Json/reader.h"

#include "../miniz/miniz.h" // declares mz_alloc_func etc. used in miniz_zip.h
#include "../miniz/miniz_zip.h"