#pragma once
#include "DX12_Config.h"
#include "DX12_Component.h"
#include "TextureSystem.h"
#include "ECSRepository.h"

class DX12_RTVHeapRepository : public ECS::IRepository<DX12_HeapComponent>
{
public:
	static DX12_RTVHeapRepository& GetInstance() {
		static DX12_RTVHeapRepository instance;
		return instance;
	}
	inline bool Initialize(ID3D12Device* device, std::uint32_t size = 64)
	{
		mDevice = device;

		mNumDescriptors = size + (DEFAULT_SIZE - size) % DEFAULT_SIZE;	// 64 배수의 RTV 저장공간을 유지하도록 구현

		InitParameters();
		CreateHeap(mNumDescriptors);
		return true;
	}
	inline D3D12_CPU_DESCRIPTOR_HANDLE AllocateHandle()
	{
		return static_cast<D3D12_CPU_DESCRIPTOR_HANDLE>(mHeap->GetCPUDescriptorHandleForHeapStart().ptr + (Load() - 1) * mDescriptorSize);
	}
	inline D3D12_CPU_DESCRIPTOR_HANDLE GetHandleByIndex(std::uint32_t index)
	{
		return static_cast<D3D12_CPU_DESCRIPTOR_HANDLE>(mHeap->GetCPUDescriptorHandleForHeapStart().ptr + index * mDescriptorSize);
	}
	inline D3D12_CPU_DESCRIPTOR_HANDLE GetHandle(ECS::RepoHandle handle)
	{
		return static_cast<D3D12_CPU_DESCRIPTOR_HANDLE>(mHeap->GetCPUDescriptorHandleForHeapStart().ptr + (SIZE_T) Get(handle)->handle * (SIZE_T) mDescriptorSize);
	}

	inline D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(ECS::RepoHandle handle)
	{
		return static_cast<D3D12_GPU_DESCRIPTOR_HANDLE>(mHeap->GetGPUDescriptorHandleForHeapStart().ptr + Get(handle)->handle * mDescriptorSize);
	}

	ECS::RepoHandle Load() {
		auto resource = std::make_unique<DX12_HeapComponent>();
		if (!LoadResourceInternal(resource.get()))
		{
			LOG_ERROR("Failed to load resource without name");
			return 0; // Return an invalid handle if loading fails
		}
		ECS::RepoHandle handle;
		{
			std::lock_guard<std::mutex> lock(mtx);
			handle = mNextHandle++;
			resource->handle = handle - 1; // Handle is 0-based index in the heap
			mResourceStorage[handle] = { std::move(resource), 1 };
		}

		return handle;
	}

	ECS::RepoHandle LoadTexture(Texture* texture) {
		ECS::RepoHandle handle;
		{
			std::lock_guard<std::mutex> lock(mtx);
			auto it = mNameToHandle.find(texture->Name);
			if (it != mNameToHandle.end()) {
				mResourceStorage[it->second].refCount++;
				return it->second;
			}
			auto resource = std::make_unique<DX12_HeapComponent>();

			handle = mNextHandle++;
			texture->Handle = handle;
			resource->handle = handle - 1; // Handle is 0-based index in the heap
			mResourceStorage[handle] = { std::move(resource), 1 };
			mNameToHandle[texture->Name] = handle;
		}

		BuildTexture2DSrv(texture);
		return handle;
	}
protected:
	constexpr static std::uint32_t DEFAULT_SIZE = 64;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mHeap;
	D3D12_DESCRIPTOR_HEAP_TYPE mType = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	std::uint32_t mDescriptorSize = 0;
	std::uint32_t mNumDescriptors = 64;
	ID3D12Device* mDevice = nullptr;

protected:
	inline void InitParameters()
	{
		mDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(mType);
		LOG_INFO("RTV Descriptor Size: {}", mDescriptorSize);
	}
	inline void CreateHeap(uint32_t num)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc
		{
			/* D3D12_DESCRIPTOR_HEAP_TYPE Type	*/mType,
			/* UINT NumDescriptors				*/num,
			/* D3D12_DESCRIPTOR_HEAP_FLAGS Flags*/D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
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
		mDevice->CreateShaderResourceView(texture->Resource.Get(), &srvDesc, GetHandle(texture->Handle));
	}

	virtual bool LoadResourceInternal(DX12_HeapComponent* ptr)
	{
		ptr->handle = mNextHandle - 1;

		return true;
	}
	virtual bool UnloadResource(ECS::RepoHandle handle)
	{
		if (IRepository<DX12_HeapComponent>::UnloadResource(handle))
		{
			// TODO: GPU 리소스 해제 (예: DestroyBuffer(mesh->vertexBuffer))

			return true;
		}

		return false;
	}
};
