#pragma once
#include "DX12_Config.h"
#include "DX12_DeviceSystem.h"
#include "DX12_CommandSystem.h"
#include "DX12_SwapChainSystem.h"
#include "WindowSystem.h"

class DX12_Core {
public:
	inline static DX12_Core& GetInstance() {
		static DX12_Core instance;
		return instance;
	}

	// Initialize DirectX 12 resources
	inline bool Initialize() {
		// auto& coordinator = ECS::Coordinator::GetInstance();
		// auto deviceSystem = coordinator.RegisterSystem<DX12_DeviceSystem>();
		// mDevice = deviceSystem->GetDevice();
		mDeviceSystem.Initialize();
		mCommandSystem.Initialize(mDeviceSystem.GetDevice()); // 시스템 간 공유가 필요 없는 로직은 DX12_Core에서 처리하는 방식으로 구현
		auto wc = ECS::Coordinator::GetInstance().GetSingletonComponent<WindowComponent>();
		mSwapChainSystem.Initialize(mDeviceSystem.GetDevice(), mDeviceSystem.GetFactory(), mCommandSystem.GetCommandQueue(), wc.hwnd, wc.width, wc.height);

		// DX12_DescriptorHeapRepository::GetInstance().Initialize();

		return true; // Return true if successful
	}

	ID3D12Device* GetDevice() const {
		return mDeviceSystem.GetDevice();
	}

private:
	DX12_DeviceSystem mDeviceSystem;
	DX12_CommandSystem mCommandSystem;
	DX12_SwapChainSystem mSwapChainSystem;
	

	inline void CreateSwapChain()
	{
		// mSwapChain.Reset();
		DXGI_SWAP_CHAIN_DESC sd
		{
			///* DXGI_MODE_DESC BufferDesc					*/
			///* 	UINT Width									*/mParam.backBufferWidth,
			///* 	UINT Height									*/mParam.backBufferHeight,
			///* 	DXGI_RATIONAL RefreshRate					*/
			///*		UINT Numerator							*/60,
			///*		UINT Denominator						*/1,
			///* 	DXGI_FORMAT Format							*/mParam.swapChainFormat,
			///* 	DXGI_MODE_SCANLINE_ORDER ScanlineOrdering	*/DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
			///* 	DXGI_MODE_SCALING Scaling					*/DXGI_MODE_SCALING_UNSPECIFIED,
			///* DXGI_SAMPLE_DESC SampleDesc					*/
			///*	UINT Count									*/m4xMsaaState ? 4u : 1u,
			///*	UINT Quality								*/m4xMsaaState ? (m4xMsaaQuality - 1) : 0u,
			///* DXGI_USAGE BufferUsage						*/DXGI_USAGE_RENDER_TARGET_OUTPUT,
			///* UINT BufferCount								*/APP_NUM_BACK_BUFFERS,
			///* HWND OutputWindow							*/mHwndWindow,
			///* BOOL Windowed								*/true,				// TBD
			///* DXGI_SWAP_EFFECT SwapEffect					*/DXGI_SWAP_EFFECT_FLIP_DISCARD,
			///* UINT Flags									*/DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
		};
	}
	inline void InitRtvAndDsvDescriptorHeaps(UINT numRTV, UINT numDSV, UINT numRTVST)
	{
		//D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc
		//{
		//	/* D3D12_DESCRIPTOR_HEAP_TYPE Type	*/D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		//	/* UINT NumDescriptors				*/numRTV + numRTVST,
		//	/* D3D12_DESCRIPTOR_HEAP_FLAGS Flags*/D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		//	/* UINT NodeMask					*/0
		//};
		//ThrowIfFailed(mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

		//{
		//	mhCPUSwapChainBuffer.resize(numRTV);
		//	mhCPUSwapChainBuffer[0] = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
		//	for (UINT i = 1; i < numRTV; i++)
		//	{
		//		mhCPUSwapChainBuffer[i].ptr = mhCPUSwapChainBuffer[i - 1].ptr + mParam.rtvDescriptorSize;
		//	}

		//	mhCPUDescHandleST.resize(numRTVST);
		//	mhCPUDescHandleST[0].ptr = mhCPUSwapChainBuffer.back().ptr + mParam.rtvDescriptorSize;
		//	for (UINT i = 1; i < numRTVST; i++)
		//	{
		//		mhCPUDescHandleST[i].ptr = mhCPUDescHandleST[i - 1].ptr + mParam.rtvDescriptorSize;
		//	}
		//}

		//D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc
		//{
		//	/* D3D12_DESCRIPTOR_HEAP_TYPE Type	*/D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
		//	/* UINT NumDescriptors				*/numDSV,
		//	/* D3D12_DESCRIPTOR_HEAP_FLAGS Flags*/D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		//	/* UINT NodeMask					*/0
		//};
		//ThrowIfFailed(mDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
		//{
		//	mhCPUDSVBuffer.resize(dsvHeapDesc.NumDescriptors);
		//	mhCPUDSVBuffer[0] = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
		//	for (UINT i = 1; i < dsvHeapDesc.NumDescriptors; i++)
		//	{
		//		mhCPUDSVBuffer[i].ptr = mhCPUDSVBuffer[i - 1].ptr + mParam.dsvDescriptorSize;
		//	}
		//}
	}

	DX12_Core() = default;
	virtual ~DX12_Core() = default;
	DX12_Core(const DX12_Core&) = delete;
	DX12_Core& operator=(const DX12_Core&) = delete;
	DX12_Core(DX12_Core&&) = delete;
	DX12_Core& operator=(DX12_Core&&) = delete; 

	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> mReadbackBuffer;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mGraphicsQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mComputeQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCopyQueue;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPipelineState;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mImGuiHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTexture;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontUploadBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontReadbackBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureUploadHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureReadbackHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureSRV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureUAV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureRTV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDSV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencil;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilUploadHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilReadbackHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilSRV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilUAV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilRTV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilDSV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilResourceSRV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilResourceUAV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilResourceRTV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilResourceDSV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilResourceReadbackHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilResourceUploadHeap;
	// Root signature
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mImGuiRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mImGuiPipelineState;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mGraphicsPipelineState;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mComputePipelineState;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mCopyPipelineState;
};
	