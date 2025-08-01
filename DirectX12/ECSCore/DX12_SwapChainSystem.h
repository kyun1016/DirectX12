#pragma once
#include "ECSConfig.h"
#include "DX12_Config.h"
#include "DX12_HeapRepository.h"
#include "DX12_CommandSystem.h"

class DX12_SwapChainSystem {
DEFAULT_SINGLETON(DX12_SwapChainSystem)
public:
    void Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue, IDXGIFactory4* factory, const HWND& hwnd, UINT width, UINT height, DX12_HeapRepository* heap)
    {
		mDevice = device;
		mWidth = width;
		mHeight = height;
		mScreenViewport = { 0.0f, 0.0f, static_cast<float>(mWidth), static_cast<float>(mHeight), 0.0f, 1.0f };
		mScissorRect = { 0, 0, static_cast<LONG>(mWidth), static_cast<LONG>(mHeight) };
		mBackBuffers.resize(APP_NUM_BACK_BUFFERS);
		mDescritorHandles.resize(APP_NUM_BACK_BUFFERS);

		for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; i++)
			mDescritorHandles[i] = heap->AllocateHandle();

		CreateSwapChain(factory, commandQueue, hwnd);
    }

    void Resize(UINT newWidth, UINT newHeight)
    {
		if (!mDevice)
			return;
		if (!mSwapChain)
			return;
		if ((mWidth == newWidth) && (mHeight == newHeight))
			return;
		DX12_CommandSystem::GetInstance().FlushCommandQueue();
		DX12_CommandSystem::GetInstance().BeginCommandList();

		// 초기값 설정
		mWidth = newWidth;
		mHeight = newHeight;
		mScreenViewport = { 0.0f, 0.0f, static_cast<float>(mWidth), static_cast<float>(mHeight), 0.0f, 1.0f };
		mScissorRect = { 0, 0, static_cast<LONG>(mWidth)/2, static_cast<LONG>(mHeight) };
		
		// 1. 백버퍼 핸들 초기화
		ReleaseBackBufferReferences();

		// 1. 백버퍼 리사이즈
		ThrowIfFailed(mSwapChain->ResizeBuffers(APP_NUM_BACK_BUFFERS, mWidth, mHeight, mSwapChainFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
		SyncBackBufferReferences();

		// 2. RTV Texture 업데이트

		// 3. DSV 업데이트
		DX12_CommandSystem::GetInstance().ExecuteCommandList();
		DX12_CommandSystem::GetInstance().FlushCommandQueue();
    }
    void Present(bool vsync)
    {
		//D3D12_CPU_DESCRIPTOR_HANDLE rtvs[1];
		//rtvs[0] = mDescritorHandles[mCurrBackBuffer];
		//mCommandList->OMSetRenderTargets(1, rtvs, false, &mDescritorHandles[0]);

		// DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH 가 있으면 Alt+Enter 지원
		// Present: 화면에 백버퍼를 출력
		UINT syncInterval = vsync ? 1 : 0; // 1: VSync On (모니터 리프레시 동기화), 0: VSync Off (최대한 빠르게)
		UINT presentFlags = DXGI_PRESENT_DO_NOT_WAIT;// 0;             // 특별한 Present 플래그 (예: DXGI_PRESENT_DO_NOT_WAIT 등)

		HRESULT hr = mSwapChain->Present(syncInterval, presentFlags);
		if (hr == DXGI_ERROR_WAS_STILL_DRAWING)
		{
			// GPU가 아직 이전 프레임을 그리고 있는 정상적인 상황입니다.
			// 이 경우에는 아무것도 하지 않고 다음 프레임으로 넘어갑니다.
			LOG_VERBOSE("[{} Frame] GPU was still drawing...", mBackBufferIndex); // 필요 시 로그 추가
		}
		else
		{
			// 그 외 다른 오류가 발생한 경우 예외 처리를 합니다.
			ThrowIfFailed(hr);
			// 다음 프레임을 위한 백버퍼 인덱스 갱신
			mBackBufferIndex = (mBackBufferIndex + 1) % APP_NUM_BACK_BUFFERS;
		}
    }

    ID3D12Resource* GetBackBuffer() const
    {
		return mBackBuffers[mBackBufferIndex].Get();
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
	D3D12_VIEWPORT mScreenViewport = D3D12_VIEWPORT{};
	D3D12_RECT mScissorRect = D3D12_RECT{};
	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> mBackBuffers;
	ID3D12Device* mDevice;

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> mDescritorHandles;
	DXGI_FORMAT mSwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	std::uint32_t mWidth = 0;
	std::uint32_t mHeight = 0;
	std::uint32_t mBackBufferIndex = 0;

	std::uint32_t m4xMsaaQuality = 0;
	bool mEnable4xMsaa = false;
	

	void CreateSwapChain(IDXGIFactory4* factory, ID3D12CommandQueue* commandQueue, const HWND& hwnd)
	{
		// Release the previous swapchain we will be recreating.
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
			/* HWND OutputWindow							*/hwnd,
			/* BOOL Windowed								*/true,				// TBD
			/* DXGI_SWAP_EFFECT SwapEffect					*/DXGI_SWAP_EFFECT_FLIP_DISCARD,
			/* UINT Flags									*/DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
		};

		// Note: Swap chain uses queue to perform flush.
#ifdef _ST
		if (mPref.renderAPI == sl::RenderAPI::eD3D12)
			slSetFeatureLoaded(sl::kFeatureDLSS_G, true);
#endif
		mSwapChain.Reset();
		ThrowIfFailed(factory->CreateSwapChain(commandQueue, &sd, mSwapChain.GetAddressOf()));
#ifdef _ST
		if (mPref.renderAPI == sl::RenderAPI::eD3D12)
			slSetFeatureLoaded(sl::kFeatureDLSS_G, false);
#endif

		SyncBackBufferReferences();
	}

	void ReleaseBackBufferReferences()
	{
		for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; i++)
			if (mBackBuffers[i])
				mBackBuffers[i].Reset();
	}

	void SyncBackBufferReferences()
	{
		mBackBufferIndex = 0;
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