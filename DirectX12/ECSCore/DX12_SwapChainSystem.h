#pragma once
#include "DX12_Config.h"
#include "DX12_RTVHeapRepository.h"
#include "DX12_CommandSystem.h"
class DX12_SwapChainSystem {
DEFAULT_SINGLETON(DX12_SwapChainSystem)
public:
    void Initialize(ID3D12Device* device, IDXGIFactory4* factory, ID3D12CommandQueue* command, HWND hwnd, UINT width, UINT height)
    {
		mDevice = device;
		mDxgiFactory = factory;
		mCommandQueue = command;
		mHwnd = hwnd;
		mWidth = width;
		mHeight = height;
		mScreenViewport = { 0.0f, 0.0f, static_cast<float>(mWidth), static_cast<float>(mHeight), 0.0f, 1.0f };
		mScissorRect = { 0, 0, static_cast<LONG>(mWidth), static_cast<LONG>(mHeight) };
		mBackBuffers.resize(APP_NUM_BACK_BUFFERS);
		mDescritorHandles.resize(APP_NUM_BACK_BUFFERS);

		for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; i++)
			mDescritorHandles[i] = DX12_RTVHeapRepository::GetInstance().AllocateHandle();

		CreateSwapChain();
    }
    void Resize(UINT newWidth, UINT newHeight)
    {
		if ((mWidth == newWidth) && (mHeight == newHeight))
			return;

		mWidth = newWidth;
		mHeight = newHeight;
		mScreenViewport = { 0.0f, 0.0f, static_cast<float>(mWidth), static_cast<float>(mHeight), 0.0f, 1.0f };
		mScissorRect = { 0, 0, static_cast<LONG>(mWidth), static_cast<LONG>(mHeight) };
		mBackBufferIndex = 0;
		DX12_CommandSystem::GetInstance().FlushCommandQueue();

		ThrowIfFailed(mSwapChain->ResizeBuffers(APP_NUM_BACK_BUFFERS, mWidth, mHeight, mSwapChainFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
		
		CreateRenderTargerView();
    }
    void Present(bool vsync)
    {

    }

    ID3D12Resource* GetBackBuffer(UINT index) const
    {
		return mBackBuffers[index].Get();
    }
	IDXGISwapChain* GetSwapChain() const
    {
		return mSwapChain.Get();
    }
	std::uint32_t GetCurrentBackBufferIndex() const
    {
		return mBackBufferIndex;
    }
	bool GetMsaaState() const
	{
		return mEnable4xMsaa;
	}
	std::uint32_t GetMsaaQuality() const
	{
		return m4xMsaaQuality;
	}
	inline const D3D12_VIEWPORT& GetViewport() const
	{
		return mScreenViewport;
	}
	inline const D3D12_RECT& GetScissorRect() const
	{
		return mScissorRect;
	}
	inline const D3D12_CPU_DESCRIPTOR_HANDLE& GetBackBufferDescriptorHandle() const
	{
		return mDescritorHandles[mBackBufferIndex];
	}


private:
	static constexpr std::uint32_t APP_NUM_BACK_BUFFERS = 3;

	ID3D12Device* mDevice = nullptr;
	IDXGIFactory4* mDxgiFactory = nullptr;
	ID3D12CommandQueue* mCommandQueue = nullptr;
	HWND mHwnd;
	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;
	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> mBackBuffers;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> mDescritorHandles;
	DXGI_FORMAT mSwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	std::uint32_t mWidth = 0;
	std::uint32_t mHeight = 0;
	std::uint32_t mBackBufferIndex = 0;

	std::uint32_t m4xMsaaQuality = 0;
	bool mEnable4xMsaa = false;
	

	void CreateSwapChain()
	{
		// Release the previous swapchain we will be recreating.
		mSwapChain.Reset();

		DXGI_SWAP_CHAIN_DESC sd
		{
			/* DXGI_MODE_DESC BufferDesc					*/
			/* 	UINT Width									*/mWidth,
			/* 	UINT Height									*/mHeight,
			/* 	DXGI_RATIONAL RefreshRate					*/
			/*		UINT Numerator							*/60,
			/*		UINT Denominator						*/1,
			/* 	DXGI_FORMAT Format							*/mSwapChainFormat,
			/* 	DXGI_MODE_SCANLINE_ORDER ScanlineOrdering	*/DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
			/* 	DXGI_MODE_SCALING Scaling					*/DXGI_MODE_SCALING_UNSPECIFIED,
			/* DXGI_SAMPLE_DESC SampleDesc					*/
			/*	UINT Count									*/mEnable4xMsaa ? 4u : 1u,
			/*	UINT Quality								*/mEnable4xMsaa ? (m4xMsaaQuality - 1) : 0u,
			/* DXGI_USAGE BufferUsage						*/DXGI_USAGE_RENDER_TARGET_OUTPUT,
			/* UINT BufferCount								*/APP_NUM_BACK_BUFFERS,
			/* HWND OutputWindow							*/mHwnd,
			/* BOOL Windowed								*/true,				// TBD
			/* DXGI_SWAP_EFFECT SwapEffect					*/DXGI_SWAP_EFFECT_FLIP_DISCARD,
			/* UINT Flags									*/DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
		};

		// Note: Swap chain uses queue to perform flush.
#ifdef _ST
		if (mPref.renderAPI == sl::RenderAPI::eD3D12)
			slSetFeatureLoaded(sl::kFeatureDLSS_G, true);
#endif
		ThrowIfFailed(mDxgiFactory->CreateSwapChain(mCommandQueue, &sd, mSwapChain.GetAddressOf()));
#ifdef _ST
		if (mPref.renderAPI == sl::RenderAPI::eD3D12)
			slSetFeatureLoaded(sl::kFeatureDLSS_G, false);
#endif

		CreateRenderTargerView();
	}

	void CreateRenderTargerView()
	{
		for (int i = 0; i < APP_NUM_BACK_BUFFERS; ++i)
			if(mBackBuffers[i])
				mBackBuffers[i].Reset();

		for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; i++)
		{
			ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mBackBuffers[i])));
			mDevice->CreateRenderTargetView(mBackBuffers[i].Get(), nullptr, mDescritorHandles[i]);
		}
	}

	inline void CheckFeatureSupport()
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
};