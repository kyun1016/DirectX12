#pragma once
#include "DX12_Config.h"
class DX12_CommandSystem {
public:
	static DX12_CommandSystem& GetInstance() {
		static DX12_CommandSystem instance;
		return instance;
	}

	// Initialize DirectX 12 resources
	inline void Initialize(ID3D12Device* device) {
		mDevice = device;
		CreateCommandContext();
		CreateFence();
	}

	inline void ExecuteAndSync() {
		ThrowIfFailed(mCommandList->Close());
		ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
		mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
		FlushCommandQueue();
	}

	inline ID3D12CommandQueue* GetCommandQueue() const
	{
		return mCommandQueue.Get();
	}
	inline ID3D12CommandAllocator* GetCommandAllocator() const
	{
		return mCommandAllocator.Get();
	}
		inline ID3D12GraphicsCommandList6* GetCommandList() const
	{
		return mCommandList.Get();
	}

	inline void FlushCommandQueue() {
		mFenceValue++;
		ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mFenceValue));
		if (mFence->GetCompletedValue() < mFenceValue) {
			ThrowIfFailed(mFence->SetEventOnCompletion(mFenceValue, mFenceEvent));
			WaitForSingleObject(mFenceEvent, INFINITE);
		}
		ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));
		LOG_INFO("Command Queue Flushed Successfully");
	}

private:
	DX12_CommandSystem() = default;
	~DX12_CommandSystem() {
		if (mFenceEvent) {
			CloseHandle(mFenceEvent);
			mFenceEvent = nullptr;
		}
	}
	DX12_CommandSystem(const DX12_CommandSystem&) = delete;
	DX12_CommandSystem& operator=(const DX12_CommandSystem&) = delete;
	DX12_CommandSystem(DX12_CommandSystem&&) = delete;
	DX12_CommandSystem& operator=(DX12_CommandSystem&&) = delete;

	ID3D12Device* mDevice = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCommandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> mCommandList;
	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
	HANDLE mFenceEvent = nullptr;
	UINT64 mFenceValue = 0;

	void CreateCommandContext() {
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		ThrowIfFailed(mDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&mCommandQueue)));
		ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator)));
		ThrowIfFailed(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&mCommandList)));
		mCommandList->Close(); // 초기에는 Close 상태여야 재사용 가능
		LOG_INFO("Command context created");
	}

	void CreateFence() {
		ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
		mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (!mFenceEvent) {
			LOG_ERROR("Failed to create fence event.");
			throw std::runtime_error("Fence event creation failed");
		}
		LOG_INFO("Fence created");
	}
};