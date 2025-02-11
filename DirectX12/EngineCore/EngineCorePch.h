#pragma once

#include <array>
#include <memory>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <limits>
#include <sstream>

#include <Windows.h>
#include <WinUser.h>
#include <wrl/client.h> // ComPtr

#include "SimpleMath.h"
#include <d3d12.h>
#include <DirectXCollision.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include "d3dx12.h"

#include "MathHelper.h"
#include "GeometryGenerator.h"
#include "GameTimer.h"
#include "DDSTextureLoader.h"
#include "D3DUtil.h"
#include "Camera.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "dxguid.lib")