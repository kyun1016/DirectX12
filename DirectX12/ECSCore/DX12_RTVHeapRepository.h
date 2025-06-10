#pragma once
#include "DX12_Config.h"
#include "DX12_Component.h"
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

		mNumDescriptors = size + size % DEFAULT_SIZE;	// 64 배수의 RTV 저장공간을 유지하도록 구현

		InitParameters();
		CreateHeap(mNumDescriptors);
		return true;
	}
	inline D3D12_CPU_DESCRIPTOR_HANDLE AllocateHandle()
	{
		return static_cast<D3D12_CPU_DESCRIPTOR_HANDLE>(mHeap->GetCPUDescriptorHandleForHeapStart().ptr + (Load() - 1) * mDescriptorSize);
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
		mNumDescriptors = num;
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc
		{
			/* D3D12_DESCRIPTOR_HEAP_TYPE Type	*/mType,
			/* UINT NumDescriptors				*/mNumDescriptors,
			/* D3D12_DESCRIPTOR_HEAP_FLAGS Flags*/D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			/* UINT NodeMask					*/0
		};
		ThrowIfFailed(mDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(mHeap.GetAddressOf())));
		LOG_INFO("DX12 Render Tagert View Descriptor Heap Size: {}", mNumDescriptors);
	}

	virtual bool LoadResourceInternal(DX12_HeapComponent* ptr)
	{
		ptr->handle = mNextHandle;

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
