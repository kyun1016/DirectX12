#pragma once
#include "TextureComponent.h"
#include "DX12_DeviceSystem.h"
#include "../EngineCore/DDSTextureLoader.h"
class TextureSystem
{
	DEFAULT_CLASS(TextureSystem)
public:
    // 엔진 클래스에서 의존성 주입을 통해 DX12 시스템 포인터를 받아옵니다.
    TextureSystem(ID3D12Device* pDevice, ID3D12GraphicsCommandList6* pCommandList)
		: mDevice(pDevice)
		, mCommandList(pCommandList)
    { }

	void Initialize()
	{
		// 텍스처 레지스트리 초기화
		mTextureRegistry.clear();
		mTextures.clear();

        LoadTexture("../Data/Textures/bricks.dds");
	}

    // 텍스처를 로드(또는 캐시에서 가져옴)하고 핸들을 반환합니다.
    TextureHandle LoadTexture(const std::string& filePath)
    {
        if (mTextureRegistry.find(filePath) == std::end(mTextureRegistry))
        {
            mTextureRegistry[filePath] = static_cast<TextureHandle>(mTextures.size());
            mTextures.emplace_back();
            mTextures.back().Name = filePath;
            ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(mDevice, mCommandList, StringToWString(filePath).c_str(), mTextures.back().Resource, mTextures.back().UploadHeap));
        }

        return mTextureRegistry[filePath];
    }

    size_t Size() const
	{
		return mTextures.size();
	}
    std::vector<Texture>& GetTextures()
    {
        return mTextures;
    }

	D3D12_DESCRIPTOR_HEAP_DESC GetDescriptorHeapDesc()
	{
		return D3D12_DESCRIPTOR_HEAP_DESC
		{
			/* D3D12_DESCRIPTOR_HEAP_TYPE Type			*/.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			/* UINT NumDescriptors						*/.NumDescriptors = (UINT)mTextures.size(),
			/* D3D12_DESCRIPTOR_HEAP_FLAGS Flags		*/.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			/* UINT NodeMask							*/.NodeMask = 0
		};
	}

private:
    ID3D12Device* mDevice = nullptr;
    ID3D12GraphicsCommandList6* mCommandList = nullptr;
    // 파일 경로 -> 핸들
    std::unordered_map<std::string, TextureHandle> mTextureRegistry;

    // 핸들 -> 실제 텍스처 데이터
    std::vector<Texture> mTextures;
};