#pragma once
#include "DX12_Config.h"
#include "DX12_DeviceSystem.h"

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
		mDevice = mDeviceSystem.GetDevice(); // 시스템 간 공유가 필요 없는 로직은 DX12_Core에서 처리하는 방식으로 구현

		if (!InitFence()) return false;
		InitMSAA();
		InitCommandObjects();

		// DX12_DescriptorHeapRepository::GetInstance().Initialize();

		return true; // Return true if successful
	}

	ID3D12Device* GetDevice() const {
		return mDevice.Get();
	}

private:
	DX12_DeviceSystem mDeviceSystem;
	inline void InitDebugLayer()
	{
#if defined(DEBUG) || defined(_DEBUG) 
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(mDebugController.GetAddressOf())));
		ThrowIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(mDxgiDebug.GetAddressOf())));
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(mDredSettings.GetAddressOf()))))
		{
			mDredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
			mDredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		}
		LOG_INFO("DirectX 12 Debug Layer Enabled");
#endif
	}
	inline void InitFactory()
	{
		UINT dxgiFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
		dxgiFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
		ThrowIfFailed(CreateDXGIFactory2(_DEBUG, IID_PPV_ARGS(&mFactory)));
		LOG_INFO("DXGI Factory Created Successfully");
	}
	inline void InitDevice()
	{
		if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice)))) {
			LOG_WARN("Failed to create D3D12 device. Falling back to WARP adapter.");
			Microsoft::WRL::ComPtr<IDXGIAdapter> pWarpAdapter;
			ThrowIfFailed(mFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));
			ThrowIfFailed(D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice)));
		}
		LOG_INFO("DirectX 12 Device Created Successfully");
	}
	inline bool InitFence()
	{
		ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
		LOG_INFO("DirectX 12 Fence Created Successfully");

		mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (mFenceEvent == nullptr)
		{
			LOG_ERROR("Failed to create fence event.");
			return false;
		}
		LOG_INFO("Fence Event Created Successfully");
		return true;
	}
	inline void InitMSAA()
	{
		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels
		{
			/*_In_  DXGI_FORMAT Format							*/.Format = mSwapChainFormat,
			/*_In_  UINT SampleCount							*/.SampleCount = 4,
			/*_In_  D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS Flags	*/.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE,
			/*_Out_  UINT NumQualityLevels						*/.NumQualityLevels = 0
		};
		ThrowIfFailed(mDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels)));
		m4xMsaaQuality = msQualityLevels.NumQualityLevels;
		if (m4xMsaaQuality > 0) {
			mEnable4xMsaa = true; // Enable 4X MSAA if supported
			LOG_INFO("4X MSAA Quality Level: {}", m4xMsaaQuality);
		}
		else {
			mEnable4xMsaa = false; // Disable 4X MSAA if not supported
			LOG_WARN("4X MSAA is not supported.");
		}
	}

	inline void FlushCommandQueue()
	{
		//// Increment the fence value.
		//mCurrentFence++;
		//ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));
		//// Wait until the GPU has completed commands up to this fence point.
		//if (mFence->GetCompletedValue() < mCurrentFence)
		//{
		//	ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, mFenceEvent));
		//	WaitForSingleObject(mFenceEvent, INFINITE);
		//}
		//LOG_INFO("Command Queue Flushed Successfully");
	}
	inline void InitCommandObjects()
	{
		// Create command queue
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		ThrowIfFailed(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));
		// Create command allocator
		ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator)));
		// Create command list
		ThrowIfFailed(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&mCommandList)));
		mCommandList->Close();
		LOG_INFO("DirectX 12 Command Objects Created Successfully");
	}
	inline void CreateSwapChain()
	{
		mSwapChain.Reset();
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

	inline void LogAdapters()
	{
		UINT i = 0;
		IDXGIAdapter* adapter = nullptr;
		std::vector<IDXGIAdapter*> adapterList;
		while (mFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
		{
			DXGI_ADAPTER_DESC desc;
			adapter->GetDesc(&desc);

			std::wstring text = L"***Adapter: ";
			text += desc.Description;
			LOG_INFO("{}", WStringToString(text));

			adapterList.push_back(adapter);

			++i;
		}

		for (i = 0; i < adapterList.size(); ++i)
		{
			LogAdapterOutputs(adapterList[i]);
			ReleaseCom(adapterList[i]);
		}
	}
	inline void LogAdapterOutputs(IDXGIAdapter* adapter)
	{
		UINT i = 0;
		IDXGIOutput* output = nullptr;
		while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
		{
			DXGI_OUTPUT_DESC desc;
			output->GetDesc(&desc);

			std::wstring text = L"***Output: ";
			text += desc.DeviceName;
			::OutputDebugString(text.c_str());
			LOG_INFO("{}", WStringToString(text));

			LogOutputDisplayModes(output, mSwapChainFormat);

			ReleaseCom(output);

			++i;
		}
	}
	inline void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
	{
		UINT count = 0;
		UINT flags = 0;

		// Call with nullptr to get list count.
		output->GetDisplayModeList(format, flags, &count, nullptr);

		std::vector<DXGI_MODE_DESC> modeList(count);
		output->GetDisplayModeList(format, flags, &count, &modeList[0]);

		for (auto& x : modeList)
		{
			UINT n = x.RefreshRate.Numerator;
			UINT d = x.RefreshRate.Denominator;
			std::wstring text =
				L"Width = " + std::to_wstring(x.Width) + L" " +
				L"Height = " + std::to_wstring(x.Height) + L" " +
				L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d);

			::OutputDebugString(text.c_str());

			LOG_INFO("{}", WStringToString(text));
		}
	}

	DX12_Core() = default;
	virtual ~DX12_Core() = default;
	DX12_Core(const DX12_Core&) = delete;
	DX12_Core& operator=(const DX12_Core&) = delete;
	DX12_Core(DX12_Core&&) = delete;
	DX12_Core& operator=(DX12_Core&&) = delete; 

	DXGI_FORMAT mSwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	UINT m4xMsaaQuality = 0;		// quality level of 4X MSAA
	bool mEnable4xMsaa = false;	// 4X MSAA enabled
	HANDLE mFenceEvent = nullptr;

#if defined(DEBUG) || defined(_DEBUG) 
	Microsoft::WRL::ComPtr<ID3D12Debug> mDebugController;
	Microsoft::WRL::ComPtr<IDXGIDebug1> mDxgiDebug;
	Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedDataSettings> mDredSettings;
#endif
	Microsoft::WRL::ComPtr<IDXGIFactory4> mFactory;
	Microsoft::WRL::ComPtr<IDXGIAdapter1> mAdapter;
	Microsoft::WRL::ComPtr<ID3D12Device> mDevice;
	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> mSwapChain;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCommandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> mBackBuffers;
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
	