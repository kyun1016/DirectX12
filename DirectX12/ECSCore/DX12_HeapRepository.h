#pragma once
#include "DX12_Config.h"
#include "DX12_Component.h"
#include "TextureSystem.h"
#include "ECSRepository.h"

class DX12_HeapRepository
{
public:
	DX12_HeapRepository() = delete;
	DX12_HeapRepository(ID3D12Device* device
		, std::uint32_t size = 64
		, const D3D12_DESCRIPTOR_HEAP_TYPE& descType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		, const D3D12_DESCRIPTOR_HEAP_FLAGS& flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
	)
		: mDevice(device)
		, mType(descType)
		, mNumDescriptors(size + (DEFAULT_SIZE - size) % DEFAULT_SIZE)
		, mDescriptorSize(device->GetDescriptorHandleIncrementSize(mType))
		, mSize(0)
	{
		LOG_INFO("DX12 Heap Type: {}", static_cast<int>(mType));
		LOG_INFO("Descriptor Size: {}", mDescriptorSize);
		CreateHeap(mNumDescriptors, flags);
	}

	~DX12_HeapRepository() = default;
	DX12_HeapRepository(const DX12_HeapRepository&) = delete;
	DX12_HeapRepository& operator=(const DX12_HeapRepository&) = delete;

	size_t GetSize() const { return mSize; }
	size_t LoadTexture(Texture* texture) {
		texture->Handle = mSize;
		BuildTexture2DSrv(texture);
		return mSize++;
	}

	inline ID3D12DescriptorHeap* GetHeap() const
	{
		return mHeap.Get();
	}
	size_t GetIndex(D3D12_CPU_DESCRIPTOR_HANDLE handle)
	{
		return (handle.ptr - mHeap->GetCPUDescriptorHandleForHeapStart().ptr) / mDescriptorSize;
	}
	inline D3D12_CPU_DESCRIPTOR_HANDLE AllocateHandle()
	{
		return static_cast<D3D12_CPU_DESCRIPTOR_HANDLE>(mHeap->GetCPUDescriptorHandleForHeapStart().ptr + mSize++ * mDescriptorSize);
	}
	inline D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(std::uint32_t index)
	{
		return static_cast<D3D12_CPU_DESCRIPTOR_HANDLE>(mHeap->GetCPUDescriptorHandleForHeapStart().ptr + index * mDescriptorSize);
	}
	inline D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(std::uint32_t index)
	{
		return static_cast<D3D12_GPU_DESCRIPTOR_HANDLE>(mHeap->GetGPUDescriptorHandleForHeapStart().ptr + index * mDescriptorSize);
	}
protected:
	constexpr static std::uint32_t DEFAULT_SIZE = 64;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mHeap;
	const D3D12_DESCRIPTOR_HEAP_TYPE mType;
	const std::uint32_t mDescriptorSize;
	const std::uint32_t mNumDescriptors;
	ID3D12Device* mDevice;

	mutable std::mutex mtx;
	size_t mSize;

protected:
	inline void CreateHeap(uint32_t num, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc
		{
			/* D3D12_DESCRIPTOR_HEAP_TYPE Type	*/mType,
			/* UINT NumDescriptors				*/num,
			/* D3D12_DESCRIPTOR_HEAP_FLAGS Flags*/flags,
			/* UINT NodeMask					*/0
		};
		ThrowIfFailed(mDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(mHeap.GetAddressOf())));
		LOG_INFO("DX12 Render Tagert View Descriptor Heap Size: {}", num);
	}

	void BuildTexture2DSrv(Texture* texture)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc
		{
			/* DXGI_FORMAT Format															*/.Format = texture->Resource->GetDesc().Format,
			/* D3D12_SRV_DIMENSION ViewDimension											*/.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
			/* UINT Shader4ComponentMapping													*/.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			/* union {																		*/
			/* 	D3D12_BUFFER_SRV Buffer														*/
			/* 	D3D12_TEX1D_SRV Texture1D													*/
			/* 	D3D12_TEX1D_ARRAY_SRV Texture1DArray										*/
			/* 	D3D12_TEX2D_SRV Texture2D{													*/.Texture2D{
			/*		UINT MostDetailedMip													*/	.MostDetailedMip = 0,
			/*		UINT MipLevels															*/	.MipLevels = texture->Resource->GetDesc().MipLevels,
			/*		UINT PlaneSlice															*/	.PlaneSlice = 0,
			/*		FLOAT ResourceMinLODClamp												*/	.ResourceMinLODClamp = 0.0f,
			/*	}																			*/}
			/* 	D3D12_TEX2D_ARRAY_SRV Texture2DArray										*/
			/* 	D3D12_TEX2DMS_SRV Texture2DMS												*/
			/* 	D3D12_TEX2DMS_ARRAY_SRV Texture2DMSArray									*/
			/* 	D3D12_TEX3D_SRV Texture3D													*/
			/* 	D3D12_TEXCUBE_SRV TextureCube												*/
			/* 	D3D12_TEXCUBE_ARRAY_SRV TextureCubeArray									*/
			/* 	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_SRV RaytracingAccelerationStructure	*/
			/* }																			*/
		};
		mDevice->CreateShaderResourceView(texture->Resource.Get(), &srvDesc, GetCPUHandle(texture->Handle));
	}
};
