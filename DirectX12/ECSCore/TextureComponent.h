#pragma once
#include "ECSConfig.h"

struct TextureHandle
{
	std::int32_t id = 0; // 텍스처 핸들을 식별하기 위한 ID
};

struct Texture
{
    std::string Name;
    Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
    size_t Handle;
};