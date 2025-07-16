#pragma once

#include "DX12_Config.h"
#include "InstanceData.h"
#include "PassData.h"
#include "CameraData.h"
#include "DX12_CommandSystem.h"

struct DX12_FrameResource {
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	UINT64 fenceValue = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	std::unique_ptr<UploadBuffer<InstanceIDData>> InstanceIDCB;
	std::unique_ptr<UploadBuffer<InstanceData>> InstanceDataBuffer;
	std::unique_ptr<UploadBuffer<CameraData>> CameraDataBuffer;
};

class DX12_FrameResourceSystem
{
public:
	inline static DX12_FrameResourceSystem& GetInstance() {
		static DX12_FrameResourceSystem instance;
		return instance;
	}

	void Initialize(ID3D12Device* device, const std::uint32_t instanceCount = 1000)
	{
		for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; ++i)
		{
			ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mFrameResources[i].commandAllocator.GetAddressOf())));
			mFrameResources[i].InstanceDataBuffer = std::make_unique<UploadBuffer<InstanceData>>(device, instanceCount, false);
			mFrameResources[i].InstanceIDCB = std::make_unique<UploadBuffer<InstanceIDData>>(device, instanceCount, true);
			mFrameResources[i].CameraDataBuffer = std::make_unique<UploadBuffer<CameraData>>(device, 1, false);
		}
	}
	void BeginFrame()
	{
		DX12_CommandSystem::GetInstance().FlushCommandQueue(mFrameResources[mCurrFrameResourceIndex].fenceValue);
		mFrameResources[mCurrFrameResourceIndex].commandAllocator->Reset();
		DX12_CommandSystem::GetInstance().ResetCommandList(mFrameResources[mCurrFrameResourceIndex].commandAllocator.Get());
	}
	void EndFrame()
	{
		mFrameResources[mCurrFrameResourceIndex].fenceValue = DX12_CommandSystem::GetInstance().SetSignalFence();
		mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % APP_NUM_BACK_BUFFERS;
	}

	void SetGPUMemory()
	{
		// auto* commandList = DX12_CommandSystem::GetInstance().GetCommandList();
		// commandList->SetGraphicsRootSignature();
	}

	// 내부에 Frame value 관리 자체를 제거함 (SwapChainSystem에서 통합 관리)
	inline DX12_FrameResource& GetFrameResource(std::uint64_t frameIndex) {
		return mFrameResources[frameIndex];
	}
	inline DX12_FrameResource& GetCurrentFrameResource() {
		return mFrameResources[mCurrFrameResourceIndex];
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetInstanceDataGPUVirtualAddress() const {
		return mFrameResources[mCurrFrameResourceIndex].InstanceDataBuffer->Resource()->GetGPUVirtualAddress();
	}
	D3D12_GPU_VIRTUAL_ADDRESS GetInstanceIDDataGPUVirtualAddress() const {
		return mFrameResources[mCurrFrameResourceIndex].InstanceIDCB->Resource()->GetGPUVirtualAddress();
	}
	D3D12_GPU_VIRTUAL_ADDRESS GetCameraDataGPUVirtualAddress() const {
		return mFrameResources[mCurrFrameResourceIndex].CameraDataBuffer->Resource()->GetGPUVirtualAddress();
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
		}
		LOG_INFO("DX12 Frame Resource System Initialized");
	};
	~DX12_FrameResourceSystem() = default;
	DX12_FrameResourceSystem(const DX12_FrameResourceSystem&) = delete;
	DX12_FrameResourceSystem& operator=(const DX12_FrameResourceSystem&) = delete;
	DX12_FrameResourceSystem(DX12_FrameResourceSystem&&) = delete;
	DX12_FrameResourceSystem& operator=(DX12_FrameResourceSystem&&) = delete;
};