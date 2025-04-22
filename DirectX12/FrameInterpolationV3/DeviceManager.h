#pragma once

#include <wrl/client.h> // ComPtr
#include <dxgi1_4.h>
#include <string>
#include <memory>

struct DeviceParameter
{
    uint32_t backBufferWidth = 1280;
    uint32_t backBufferHeight = 720;
	float aspectRatio;

    DXGI_FORMAT swapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    UINT rtvDescriptorSize = 0;
    UINT dsvDescriptorSize = 0;
    UINT cbvSrvUavDescriptorSize = 0;
};

