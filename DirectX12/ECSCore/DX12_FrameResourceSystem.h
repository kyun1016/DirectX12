#pragma once

#include "DX12_Config.h"

struct DX12_FrameResource {
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	UINT64 fenceValue = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
};

struct DX12_FrameResourceSystem
{
public:
	inline static DX12_FrameResourceSystem& GetInstance() {
		static DX12_FrameResourceSystem instance;
		return instance;
	}

	void Initialize(ID3D12Device* device)
	{
		// CreateFence(device);
	}

	inline void FlushCommandQueue() {
		//ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mFenceValue));
		//if (mFence->GetCompletedValue() < mFenceValue) {
		//	ThrowIfFailed(mFence->SetEventOnCompletion(mFenceValue, mFenceEvent));
		//	WaitForSingleObject(mFenceEvent, INFINITE);
		//}
	}
	void BeginFrame()
	{
		mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % APP_NUM_BACK_BUFFERS;
		DX12_CommandSystem::GetInstance().FlushCommandQueue(mFrameResources[mCurrFrameResourceIndex].fenceValue);
	}
	void EndFrame(std::uint64_t fenceValue)
	{
		mFrameResources[mCurrFrameResourceIndex].fenceValue = fenceValue;
	}

	// 내부에 Frame value 관리 자체를 제거함 (SwapChainSystem에서 통합 관리)
	inline DX12_FrameResource& GetFrameResource(std::uint64_t frameIndex) {
		return mFrameResources[frameIndex];
	}

private:
	ID3D12CommandQueue* mCommandQueue = nullptr;
	std::vector<DX12_FrameResource> mFrameResources;
	DX12_FrameResource* mCurrFrameResource = nullptr;
	std::uint64_t mCurrFrameResourceIndex = 0;

	DX12_FrameResourceSystem()
	{
		mFrameResources.resize(APP_NUM_BACK_BUFFERS);
		for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; ++i)
		{
			mFrameResources[i].fenceValue = 0;
			mFrameResources[i].rtvHandle.ptr = 0;
			mFrameResources[i].commandAllocator.Reset();
		}
		LOG_INFO("Frame Resource System Initialized");
	};
	~DX12_FrameResourceSystem() = default;
	DX12_FrameResourceSystem(const DX12_FrameResourceSystem&) = delete;
	DX12_FrameResourceSystem& operator=(const DX12_FrameResourceSystem&) = delete;
	DX12_FrameResourceSystem(DX12_FrameResourceSystem&&) = delete;
	DX12_FrameResourceSystem& operator=(DX12_FrameResourceSystem&&) = delete;
};