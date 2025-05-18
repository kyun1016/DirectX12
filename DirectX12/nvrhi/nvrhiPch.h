#pragma once
#define NOMINMAX

#include <stdint.h>
#include <atomic>
#include <cstdint>
#include <type_traits>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>
#include <cstddef>
#include <sstream>

#include "../nvrhi/nvrhi.h"
// #include "../nvrhi/common/resource.h"
// #include "../nvrhi/vulkan.h"
// #include "../nvrhi/d3d11.h"
// #include "../nvrhi/d3d12.h"
#include "../nvrhi/utils.h"
#include "../nvrhi/validation.h"

#include "../nvrhi/common/aftermath.h"
#include "../nvrhi/common/containers.h"
#include "../nvrhi/common/dxgi-format.h"
#include "../nvrhi/common/misc.h"
#include "../nvrhi/common/resourcebindingmap.h"
#include "../nvrhi/common/sparse-bitset.h"
#include "../nvrhi/common/state-tracking.h"
#include "../nvrhi/common/versioning.h"

#include "../nvrhi/validation/validation-backend.h"