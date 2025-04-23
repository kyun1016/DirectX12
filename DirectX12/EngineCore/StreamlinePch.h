#pragma once

#ifdef _ST
#define NOMINMAX
#define SL_WINDOWS 1
#else
#endif

#ifdef _ST
#include "sl.h"
#include "sl_consts.h"
#include "sl_dlss.h"
#include "sl_security.h"
#include "sl_device_wrappers.h"
#include "sl_core_api.h"
#include "sl_core_types.h"
#else
#endif