#pragma once
#include "DX12_Config.h"
#include "DX12_MeshComponent.h"

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

	inline void FlushCommandQueue(uint64_t fenceValue) {
		if (mFence->GetCompletedValue() < fenceValue) {
			ThrowIfFailed(mFence->SetEventOnCompletion(fenceValue, mFenceEvent));
			WaitForSingleObject(mFenceEvent, INFINITE);
		}
	}

	inline std::uint64_t SetSignalFence() {
		mFenceValue++;
		ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mFenceValue));
		LOG_VERBOSE("Fence Signaled: {}", mFenceValue);

		return mFenceValue;
	}
	inline std::uint64_t GetFenceValue() const {
		return mFenceValue;
	}

	inline void FlushCommandQueue() {
		SetSignalFence();
		if (mFence->GetCompletedValue() < mFenceValue) {
			ThrowIfFailed(mFence->SetEventOnCompletion(mFenceValue, mFenceEvent));
			WaitForSingleObject(mFenceEvent, INFINITE);
		}
		LOG_INFO("Command Queue Flushed Successfully");
	}

	inline void BeginCommandList() {
		// ThrowIfFailed(mCommandAllocator->Reset());
		ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));
	}

	inline void ResetCommandList(ID3D12CommandAllocator* commandAllocator) {
		ThrowIfFailed(mCommandList->Reset(commandAllocator, nullptr));
	}

	inline void SetViewportAndScissor(const D3D12_VIEWPORT& viewport, const D3D12_RECT& scissorRect) {
		mCommandList->RSSetViewports(1, &viewport);
		mCommandList->RSSetScissorRects(1, &scissorRect);
	}

	inline void SetRootSignature(ID3D12RootSignature* rootSignature) {
		mCommandList->SetGraphicsRootSignature(rootSignature);
	}

	inline void SetMesh(const DX12_MeshGeometry* mesh) {
		mLastVertexBufferView = mesh->VertexBufferView();
		mLastIndexBufferView = mesh->IndexBufferView();
		mLastPrimitiveType = mesh->PrimitiveType;

		mCommandList->IASetVertexBuffers(0, 1, &mLastVertexBufferView);
		mCommandList->IASetIndexBuffer(&mLastIndexBufferView);
		mCommandList->IASetPrimitiveTopology(mLastPrimitiveType);
	}

	inline void ExecuteCommandList() {
		// Close the command list to prepare for execution
		ThrowIfFailed(mCommandList->Close());
		// Execute the command list
		ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
		mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
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
	std::uint64_t mFenceValue = 0;

	D3D12_VERTEX_BUFFER_VIEW mLastVertexBufferView = { 0, 0, 0 };
	D3D12_INDEX_BUFFER_VIEW mLastIndexBufferView = D3D12_INDEX_BUFFER_VIEW{ 0, 0, DXGI_FORMAT_R16_UINT };
	D3D12_PRIMITIVE_TOPOLOGY mLastPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

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