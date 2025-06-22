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
		static DX12_FrameResourceSystem instance; return instance;
	}

	void Initialize(UINT frameCount);
	void BeginFrame();
	void EndFrame(ID3D12CommandQueue* queue);

private:
	std::vector<DX12_FrameResource> mResources;
	UINT64 mFence = 0;
	DX12_FrameResourceSystem()
	{
		mResources.resize(APP_NUM_BACK_BUFFERS);
		for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; ++i)
		{
			mResources[i].fenceValue = 0;
			mResources[i].rtvHandle.ptr = 0;
			mResources[i].commandAllocator.Reset();
		}
		LOG_INFO("Frame Resource System Initialized");
	};
	~DX12_FrameResourceSystem() = default;
	DX12_FrameResourceSystem(const DX12_FrameResourceSystem&) = delete;
	DX12_FrameResourceSystem& operator=(const DX12_FrameResourceSystem&) = delete;
	DX12_FrameResourceSystem(DX12_FrameResourceSystem&&) = delete;
	DX12_FrameResourceSystem& operator=(DX12_FrameResourceSystem&&) = delete;
};