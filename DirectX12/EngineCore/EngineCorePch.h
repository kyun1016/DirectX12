#pragma once

#ifdef _ST
#define NOMINMAX
#define SL_WINDOWS 1
#else
#endif


// #include "donut/core/math/math.h"

#include <array>
#include <memory>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <limits>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <map>

// #include <libloaderapi.h>
#include <Windows.h>
#include <WinUser.h>
#include <wrl/client.h> // ComPtr

#include "SimpleMath.h"
#include "DirectXMesh.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXCollision.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <dxgidebug.h>
#include "d3dx12.h"

#include "MathHelper.h"
#include "GeometryGenerator.h"
#include "GameTimer.h"
#include "DDSTextureLoader.h"
#include "D3DUtil.h"
#include "Camera.h"
#include "LoadM3d.h"

#pragma comment(lib, "d3dcompiler.lib")

#ifdef _ST
#pragma comment(lib, "..\\StreamlineCore\\streamline\\lib\\x64\\sl.interposer.lib")
#else
#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")
#endif

// #pragma comment(lib, "dxguid.lib")