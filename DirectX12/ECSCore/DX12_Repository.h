#pragma once
#include "DX12_Config.h"
#include "DX12_Component.h"
#include "ECSRepository.h"

class DX12_DescriptorHeapRepository : public ECS::IRepository<RenderTargetComponent>
{
public:
	static DX12_DescriptorHeapRepository& GetInstance() {
		static DX12_DescriptorHeapRepository instance;
		return instance;
	}
	inline bool Initialize()
	{
		mDevice = DX12_Core::GetInstance().GetDevice();

		InitParameters();
		CreateRTVHeap(mNumRtvDescriptors);
		CreateDSVHeap(mNumRtvDescriptors);
		return true;
	}
	inline D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle(ECS::RepoHandle handle)
	{
		return static_cast<D3D12_CPU_DESCRIPTOR_HANDLE>(mRtvHeap->GetCPUDescriptorHandleForHeapStart().ptr + (Get(handle)->rtvHandle - 1) * mRtvDescriptorSize);
	}
	inline D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle(ECS::RepoHandle handle)
	{
		return static_cast<D3D12_CPU_DESCRIPTOR_HANDLE>(mDsvHeap->GetCPUDescriptorHandleForHeapStart().ptr + (Get(handle)->dsvHandle - 1) * mRtvDescriptorSize);
	}
protected:
	uint32_t mRtvDescriptorSize = 0;
	uint32_t mDsvDescriptorSize = 0;
	uint32_t mCbvSrvUavDescriptorSize = 0;

	uint32_t mNumRtvDescriptors = 64;
	uint32_t mNumDsvDescriptors = 64;
	uint32_t mCurrentRtvIndex = 0;
	uint32_t mCurrentDsvIndex = 0;
	ID3D12Device* mDevice = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;
protected:
	inline void InitParameters()
	{
		mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		mDsvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		mCbvSrvUavDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		LOG_INFO("RTV Descriptor Size: {}", mRtvDescriptorSize);
		LOG_INFO("DSV Descriptor Size: {}", mDsvDescriptorSize);
		LOG_INFO("CBV/SRV/UAV Descriptor Size: {}", mCbvSrvUavDescriptorSize);
	}
	inline void CreateRTVHeap(uint32_t num)
	{
		mNumRtvDescriptors = num;
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc
		{
			/* D3D12_DESCRIPTOR_HEAP_TYPE Type	*/D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			/* UINT NumDescriptors				*/mNumRtvDescriptors,
			/* D3D12_DESCRIPTOR_HEAP_FLAGS Flags*/D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			/* UINT NodeMask					*/0
		};
		ThrowIfFailed(mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));
		LOG_INFO("DX12 Render Tagert View Descriptor Heap Size: {}", mNumRtvDescriptors);
	}
	inline void CreateDSVHeap(uint32_t num)
	{
		mNumDsvDescriptors = num;
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc
		{
			/* D3D12_DESCRIPTOR_HEAP_TYPE Type	*/D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
			/* UINT NumDescriptors				*/mNumDsvDescriptors,
			/* D3D12_DESCRIPTOR_HEAP_FLAGS Flags*/D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			/* UINT NodeMask					*/0
		};
		ThrowIfFailed(mDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
		LOG_INFO("DX12 Depth Stencil View Descriptor Heap Size: {}", mNumDsvDescriptors);
	}

	virtual bool LoadResourceInternal(RenderTargetComponent* ptr)
	{
		ptr->rtvHandle = mNextHandle;
		ptr->dsvHandle = mNextHandle;

		return true;
	}
	virtual bool UnloadResource(ECS::RepoHandle handle)
	{
		if (IRepository<RenderTargetComponent>::UnloadResource(handle))
		{
			// TODO: GPU 리소스 해제 (예: DestroyBuffer(mesh->vertexBuffer))

			return true;
		}

		return false;
	}
};
