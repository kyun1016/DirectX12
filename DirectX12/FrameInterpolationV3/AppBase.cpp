#include "pch.h"
#include "AppBase.h"
#include <sl_security.h>
#include <sl_core_api.h>

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Singleton object so that worker threads can share members.

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	return AppBase::g_appBase->MsgProc(hWnd, msg, wParam, lParam);
}
//===================================
// Constructor
//===================================
AppBase::AppBase() : AppBase(1080, 720, L"AppBase") {}

AppBase::AppBase(uint32_t width, uint32_t height, std::wstring name)
{
	mWindowClass = {
		/*UINT        cbSize		*/	sizeof(mWindowClass),
		/*UINT        style			*/	CS_CLASSDC,
		/*WNDPROC     lpfnWndProc	*/	WndProc,
		/*int         cbClsExtra	*/	0L,
		/*int         cbWndExtra	*/	0L,
		/*HINSTANCE   hInstance		*/	GetModuleHandle(nullptr),
		/*HICON       hIcon			*/	nullptr,
		/*HCURSOR     hCursor		*/	nullptr,
		/*HBRUSH      hbrBackground	*/	nullptr,
		/*LPCWSTR     lpszMenuName	*/	nullptr,
		/*LPCWSTR     lpszClassName	*/	name.c_str(), // lpszClassName, L-string
		/*HICON       hIconSm		*/	nullptr
	};
	g_appBase = this;

	UpdateForSizeChange(width, height);
	// mCamera = make_unique<Camera>();

	m_callbacks.beforeFrame = [](AppBase& m, uint32_t f) { g_appBase->ReflexCallback_Sleep(m, f); };
	m_callbacks.beforeAnimate = [](AppBase& m, uint32_t f) { g_appBase->ReflexCallback_SimStart(m, f); };
	m_callbacks.afterAnimate = [](AppBase& m, uint32_t f) { g_appBase->ReflexCallback_SimEnd(m, f); };
	m_callbacks.beforeRender = [](AppBase& m, uint32_t f) { g_appBase->ReflexCallback_RenderStart(m, f); };
	m_callbacks.afterRender = [](AppBase& m, uint32_t f) { g_appBase->ReflexCallback_RenderEnd(m, f); };
	m_callbacks.beforePresent = [](AppBase& m, uint32_t f) { g_appBase->ReflexCallback_PresentStart(m, f); };
	m_callbacks.afterPresent = [](AppBase& m, uint32_t f) { g_appBase->ReflexCallback_PresentEnd(m, f); };
}

AppBase::~AppBase()
{
#if defined(DEBUG) || defined(_DEBUG) 
	ShutdownDebugLayer();
#endif
	OutputDebugStringW(L"*Info, Release Singleton\n");
	SL_LOG_VERBOSE("Release Singleton!");
	sl::log::destroyInterface();
	slShutdown();
	g_appBase = nullptr;
}

void AppBase::Release()
{
	if (g_appBase)
	{
		delete g_appBase;
		g_appBase = nullptr;
	}
}

float AppBase::AspectRatio() const
{
	return static_cast<float>(mParam.backBufferWidth) / mParam.backBufferHeight;
}

void AppBase::Set4xMsaaState(bool value)
{
	if (m4xMsaaState != value)
	{
		m4xMsaaState = value;

		// Recreate the swapchain and buffers with new multisample settings.
		CreateSwapChain();
		OnResize();
	}
}

std::wstring AppBase::GetAssetFullPath(LPCWSTR assetName)
{
	return mAssetsPath + assetName;
}

bool AppBase::Initialize()
{
	mSwapChainBuffer.resize(APP_NUM_BACK_BUFFERS, Microsoft::WRL::ComPtr<ID3D12Resource>());
	mSRVUserBuffer.resize(SRV_USER_SIZE, Microsoft::WRL::ComPtr<ID3D12Resource>());

	if (!InitSLLog())
		return false;

	if (!InitMainWindow())
		return false;

	if (!LoadStreamline())
		return false;

	// Load Streamline 단계와 통합
	// if (!InitDirect3D())
	// 	return false;

	{
		mCBShaderToy.dx = 1.0f / 640;
		mCBShaderToy.dy = 1.0f / 480;
		mCBShaderToy.iResolution.x = (float)640;
		mCBShaderToy.iResolution.y = (float)480;
	}

	// Do the initial resize code.
	OnResize();

	return true;
}

void AppBase::CleanUp()
{
	// Cleanup
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	DestroyWindow(mHwndWindow);
	::UnregisterClassW(mWindowClass.lpszClassName, mWindowClass.hInstance);
}

int AppBase::Run()
{
	Initialize();


	MSG msg{ 0 };

	mTimer.Reset();

	while (msg.message != WM_QUIT)
	{
		// If there are Window messages then process them.
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
 			DispatchMessage(&msg);
		}
		else if (mSyncEn)
		{
			mSyncEn = false;
			FlushCommandQueue();
			Sync();
		}
		else {
			mTimer.Tick();
			if (!mAppPaused)
			{
				//==========================================
				// My Render
				//==========================================
				CalculateFrameStats();

				if (m_callbacks.beforeAnimate) m_callbacks.beforeAnimate(*this, mFrameCount);
				Update();
				if (m_callbacks.afterAnimate) m_callbacks.afterAnimate(*this, mFrameCount);

				if (m_callbacks.beforeRender) m_callbacks.beforeRender(*this, mFrameCount);
				// New Function
				SLFrameInit();
				Render();
				RenderImGui();
				// SLFrameSetting();

				if (m_callbacks.afterRender) m_callbacks.afterRender(*this, mFrameCount);

				// Done recording commands.
				ThrowIfFailed(mCommandList->Close());

				// Add the command list to the queue for execution.
				ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
				mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

				// Update and Render additional Platform Windows
				if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
				{
					ImGui::UpdatePlatformWindows();
					ImGui::RenderPlatformWindowsDefault();
				}

				if (m_callbacks.beforePresent) m_callbacks.beforePresent(*this, mFrameCount);
				// Swap the back and front buffers
				ThrowIfFailed(mSwapChain->Present(mParam.vsyncEnabled ? 1 : 0, 0));
				mCurrBackBuffer = (mCurrBackBuffer + 1) % APP_NUM_BACK_BUFFERS;

				// Add an instruction to the command queue to set a new fence point. 
				// Because we are on the GPU timeline, the new fence point won't be 
				// set until the GPU finishes processing all the commands prior to this Signal().
				mCommandQueue->Signal(mFence.Get(), mFrameCount);

				// Advance the fence value to mark commands up to this fence point.
				mCurrFrameResource->Fence = ++mFrameCount;
				if (m_callbacks.afterPresent) m_callbacks.afterPresent(*this, mFrameCount);
			}
			else
			{
				Sleep(100);
			}
		}
	}
	FlushCommandQueue();

	CleanUp();
	return (int)msg.wParam;
}

void AppBase::UpdateForSizeChange(uint32_t clientWidth, uint32_t clientHeight)
{
	mParam.backBufferWidth = clientWidth;
	mParam.backBufferHeight = clientHeight;
	mParam.aspectRatio = static_cast<float>(mParam.backBufferWidth) / static_cast<float>(mParam.backBufferHeight);
}

void AppBase::SetWindowBounds(RECT& rect, int left, int top, int right, int bottom)
{
	rect.left = static_cast<LONG>(left);
	rect.top = static_cast<LONG>(top);
	rect.right = static_cast<LONG>(right);
	rect.bottom = static_cast<LONG>(bottom);
}

void AppBase::LogAdapters()
{
	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;
	std::vector<IDXGIAdapter*> adapterList;
	while (mDxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		std::wstring text = L"***Adapter: ";
		text += desc.Description;
		SL_LOG_INFO("%ls", text.c_str());

		adapterList.push_back(adapter);

		++i;
	}

	for (i = 0; i < adapterList.size(); ++i)
	{
		LogAdapterOutputs(adapterList[i]);
		ReleaseCom(adapterList[i]);
	}
}

void AppBase::LogAdapterOutputs(IDXGIAdapter* adapter)
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
		SL_LOG_INFO("%ls", text.c_str());
		
		LogOutputDisplayModes(output, mParam.swapChainFormat);

		ReleaseCom(output);

		++i;
	}
}

void AppBase::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
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

		SL_LOG_INFO("%ls", text.c_str());
	}
}

void AppBase::CreateRtvAndDsvDescriptorHeaps(UINT numRTV, UINT numDSV, UINT numRTVST)
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc
	{
		/* D3D12_DESCRIPTOR_HEAP_TYPE Type	*/D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		/* UINT NumDescriptors				*/numRTV + numRTVST,
		/* D3D12_DESCRIPTOR_HEAP_FLAGS Flags*/D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		/* UINT NodeMask					*/0
	};
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

	mParam.rtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	{
		mhCPUSwapChainBuffer.resize(numRTV);
		mhCPUSwapChainBuffer[0] = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
		for (UINT i = 1; i < numRTV; i++)
		{
			mhCPUSwapChainBuffer[i].ptr = mhCPUSwapChainBuffer[i - 1].ptr + mParam.rtvDescriptorSize;
		}

		mhCPUDescHandleST.resize(numRTVST);
		mhCPUDescHandleST[0].ptr = mhCPUSwapChainBuffer.back().ptr + mParam.rtvDescriptorSize;
		for (UINT i = 1; i < numRTVST; i++)
		{
			mhCPUDescHandleST[i].ptr = mhCPUDescHandleST[i - 1].ptr + mParam.rtvDescriptorSize;
		}
	}

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc
	{
		/* D3D12_DESCRIPTOR_HEAP_TYPE Type	*/D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
		/* UINT NumDescriptors				*/numDSV,
		/* D3D12_DESCRIPTOR_HEAP_FLAGS Flags*/D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		/* UINT NodeMask					*/0
	};
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));

	mParam.dsvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	{
		mhCPUDSVBuffer.resize(dsvHeapDesc.NumDescriptors);
		mhCPUDSVBuffer[0] = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
		for (UINT i = 1; i < dsvHeapDesc.NumDescriptors; i++)
		{
			mhCPUDSVBuffer[i].ptr = mhCPUDSVBuffer[i - 1].ptr + mParam.dsvDescriptorSize;
		}
	}
	
}
// DX12 Debug Layer <- GPU에서 에러나는 걸 로그로 출력
void AppBase::OnResize()
{
	if ((mParam.backBufferWidth == mLastClientWidth) && (mParam.backBufferHeight == mLastClientHeight))
		return;
	mLastClientHeight = mParam.backBufferHeight;
	mLastClientWidth = mParam.backBufferWidth;
	assert(mDevice);
	assert(mSwapChain);
	UpdateForSizeChange(mParam.backBufferWidth, mParam.backBufferHeight);

	// Flush before changing any resources.
	FlushCommandQueue();

	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

	// Release the previous resources we will be recreating.
	for (int i = 0; i < APP_NUM_BACK_BUFFERS; ++i)
		mSwapChainBuffer[i].Reset();
	for (int i = 0; i < SRV_USER_SIZE; ++i)
		mSRVUserBuffer[i].Reset();
	mDepthStencilBuffer.Reset();

	// Resize the swap chain.
	ThrowIfFailed(mSwapChain->ResizeBuffers(APP_NUM_BACK_BUFFERS, mParam.backBufferWidth, mParam.backBufferHeight, mParam.swapChainFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurrBackBuffer = 0;
	for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; ++i)
	{
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		mDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, mhCPUSwapChainBuffer[i]);
	}

	//=====================================
	// RTV Render in a Texture
	D3D12_RESOURCE_DESC rtvTexDesc
	{
		/* D3D12_RESOURCE_DIMENSION Dimension	*/	D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		/* UINT64 Alignment						*/	0,
		/* UINT64 Width							*/	mParam.backBufferWidth,
		/* UINT Height							*/	mParam.backBufferHeight,
		/* UINT16 DepthOrArraySize				*/	1,
		/* UINT16 MipLevels						*/	1,
		/* DXGI_FORMAT Format					*/	DXGI_FORMAT_R8G8B8A8_UNORM,
		/* DXGI_SAMPLE_DESC SampleDesc{			*/	{
		/*		UINT Count						*/		1,
		/*		UINT Quality					*/ 		0
		/* }									*/	},
		/* D3D12_TEXTURE_LAYOUT Layout			*/	D3D12_TEXTURE_LAYOUT_UNKNOWN,
		/* D3D12_RESOURCE_FLAGS Flags			*/	D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
	};

	D3D12_CLEAR_VALUE clearValue
	{
		/* DXGI_FORMAT Format;							*/rtvTexDesc.Format,
		/* union {										*/{
		/*		FLOAT Color[4];							*/	{ 0.7f, 0.7f, 0.7f, 1.0f }
		/*		D3D12_DEPTH_STENCIL_VALUE DepthStencil{	*/
		/*			FLOAT Depth;						*/
		/*			UINT8 Stencil;						*/
		/*		}										*/
		/* } 											*/}
	};

	D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	for (int i = 0; i < RTV_USER_SIZE + 1; ++i)
		ThrowIfFailed(mDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &rtvTexDesc, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(mSRVUserBuffer[i].GetAddressOf())));

	rtvTexDesc.Width = (UINT64) mCBShaderToy.iResolution.x;
	rtvTexDesc.Height = (UINT64) mCBShaderToy.iResolution.y;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 0.0f;
	for (int i = 0; i < RTV_TOY_SIZE; ++i)
		ThrowIfFailed(mDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &rtvTexDesc, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(mSRVUserBuffer[i + RTV_USER_SIZE + 1].GetAddressOf())));

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {
		/* DXGI_FORMAT Format						*/.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
		/* D3D12_RTV_DIMENSION ViewDimension		*/.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
		/* union									*/
		/* 	{										*/
		/* 	D3D12_BUFFER_RTV Buffer					*/
		/* 	D3D12_TEX1D_RTV Texture1D				*/
		/* 	D3D12_TEX1D_ARRAY_RTV Texture1DArray	*/
		/* 	D3D12_TEX2D_RTV Texture2D{				*/.Texture2D = {
		/*		UINT MipSlice						*/	0,
		/*		UINT PlaneSlice						*/	0
		/*  }										*/}
		/* 	D3D12_TEX2D_ARRAY_RTV Texture2DArray	*/
		/* 	D3D12_TEX2DMS_RTV Texture2DMS			*/
		/* 	D3D12_TEX2DMS_ARRAY_RTV Texture2DMSArray*/
		/* 	D3D12_TEX3D_RTV Texture3D				*/
		/* 	}										*/
	};

	for (int i = 0; i < RTV_USER_SIZE; ++i)
	{
		mDevice->CreateRenderTargetView(mSRVUserBuffer[i].Get(), &rtvDesc, mhCPUSwapChainBuffer[APP_NUM_BACK_BUFFERS + i]);
	}
	for (int i = 0; i < RTV_TOY_SIZE; ++i)
	{
		mDevice->CreateRenderTargetView(mSRVUserBuffer[i+RTV_USER_SIZE + 1].Get(), &rtvDesc, mhCPUDescHandleST[i]);
	}

	// RTV Render in a Texture
	//=====================================

	// Create the depth/stencil buffer and view.
	D3D12_RESOURCE_DESC depthStencilDesc
	{
		/* D3D12_RESOURCE_DIMENSION Dimension	*/	D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		/* UINT64 Alignment						*/	0,
		/* UINT64 Width							*/	mParam.backBufferWidth,
		/* UINT Height							*/	mParam.backBufferHeight,
		/* UINT16 DepthOrArraySize				*/	1,
		/* UINT16 MipLevels						*/	1,
		/* DXGI_FORMAT Format					*/	DXGI_FORMAT_R24G8_TYPELESS,
		/* DXGI_SAMPLE_DESC SampleDesc{			*/	{
		/*		UINT Count						*/	m4xMsaaState ? (UINT)4 : (UINT)1,
		/*		UINT Quality					*/ 	m4xMsaaState ? (m4xMsaaQuality - 1) : 0
		/* }									*/	},
		/* D3D12_TEXTURE_LAYOUT Layout			*/	D3D12_TEXTURE_LAYOUT_UNKNOWN,
		/* D3D12_RESOURCE_FLAGS Flags			*/	D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	};

	
	D3D12_CLEAR_VALUE optClear
	{
		/* DXGI_FORMAT Format;							*/mParam.depthStencilFormat,
		/* union {										*/{
		/*		FLOAT Color[4];							*/
		/*		D3D12_DEPTH_STENCIL_VALUE DepthStencil{	*/	{
		/*			FLOAT Depth;						*/		1.0f,
		/*			UINT8 Stencil;						*/		0
		/*		}										*/	},
		/* } 											*/}
	};
	ThrowIfFailed(mDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &optClear, IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

	// Create descriptor to mip level 0 of entire resource using the format of the resource.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc
	{
		/* DXGI_FORMAT Format;								*/.Format = mParam.depthStencilFormat,
		/* D3D12_DSV_DIMENSION ViewDimension;				*/.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
		/* D3D12_DSV_FLAGS Flags;							*/.Flags = D3D12_DSV_FLAG_NONE,
		/* union {											*/
		/*		D3D12_TEX1D_DSV Texture1D{					*/
		/*			UINT MipSlice;							*/
		/*      }											*/
		/*		D3D12_TEX1D_ARRAY_DSV Texture1DArray{		*/
		/*			UINT MipSlice;							*/
		/*			UINT FirstArraySlice;					*/
		/*			UINT ArraySize;							*/
		/*		}											*/
		/*		D3D12_TEX2D_DSV Texture2D{					*/.Texture2D = 0
		/*			UINT MipSlice;							*/
		/*		}											*/
		/*		D3D12_TEX2D_ARRAY_DSV Texture2DArray{		*/
		/*			UINT MipSlice;							*/
		/*			UINT FirstArraySlice;					*/
		/*			UINT ArraySize;							*/
		/*		}											*/
		/*		D3D12_TEX2DMS_DSV Texture2DMS{				*/
		/*			UINT UnusedField_NothingToDefine;		*/
		/*		}											*/
		/*		D3D12_TEX2DMS_ARRAY_DSV Texture2DMSArray{	*/
		/*			UINT FirstArraySlice;					*/
		/*			UINT ArraySize;							*/
		/*		}											*/
		/* } D3D12_DEPTH_STENCIL_VIEW_DESC					*/
	};
	mDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, mhCPUDSVBuffer[0]);

	// Transition the resource from its initial state to be used as a depth buffer.
	CD3DX12_RESOURCE_BARRIER ResourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	mCommandList->ResourceBarrier(1, &ResourceBarrier);

	// Execute the resize commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until resize is complete.
	FlushCommandQueue();

	// Update the viewport transform to cover the client area.
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mParam.backBufferWidth);
	mScreenViewport.Height = static_cast<float>(mParam.backBufferHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, static_cast<LONG>(mParam.backBufferWidth), static_cast<LONG>(mParam.backBufferHeight) };
}

#pragma region Window
bool AppBase::RegisterWindowClass()
{
	if (!RegisterClassEx(&mWindowClass)) {
		std::cout << "RegisterClassEx() failed." << std::endl;
		MessageBox(0, L"Register WindowClass Failed.", 0, 0);
		return false;
	}

	return true;
}

bool AppBase::MakeWindowHandle()
{
	RECT rect;
	SetWindowBounds(rect, 0, 0, mParam.backBufferWidth, mParam.backBufferHeight);
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	mHwndWindow = CreateWindow(
		/* _In_opt_ LPCWSTR lpClassName	*/ mWindowClass.lpszClassName,
		/* _In_opt_ LPCWSTR lpWindowName*/ mWndCaption.c_str(),
		/* _In_ DWORD dwStyle			*/ WS_OVERLAPPEDWINDOW,
		/* _In_ int X					*/ 100, // 윈도우 좌측 상단의 x 좌표
		/* _In_ int Y					*/ 100, // 윈도우 좌측 상단의 y 좌표
		/* _In_ int nWidth				*/ mParam.backBufferWidth, // 윈도우 가로 방향 해상도
		/* _In_ int nHeight				*/ mParam.backBufferHeight, // 윈도우 세로 방향 해상도
		/* _In_opt_ HWND hWndParent		*/ NULL,
		/* _In_opt_ HMENU hMenu			*/ (HMENU)0,
		/* _In_opt_ HINSTANCE hInstance	*/ mWindowClass.hInstance,
		/* _In_opt_ LPVOID lpParam		*/ NULL
	);

	if (!mHwndWindow) {
		std::cout << "CreateWindow() failed." << std::endl;
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	return true;
}
// Render Doc + PIX tool
bool AppBase::InitMainWindow()
{
	if (!RegisterWindowClass())
		return false;

	if (!MakeWindowHandle())
		return false;

	ShowWindow(mHwndWindow, SW_SHOWDEFAULT);
	UpdateWindow(mHwndWindow);

	return true;
}
#pragma endregion Window

bool AppBase::InitDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG) 
	InitDebugLayer();
#endif
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mDxgiFactory)));

	// Try to create hardware device.
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // default adapter
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&mDevice));

	// Fallback to WARP device.
	if (FAILED(hardwareResult))
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(mDxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&mDevice)));
	}

	ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)));

	mParam.cbvSrvUavDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Check 4X MSAA quality support for our back buffer format.
	// All Direct3D 11 capable devices support 4X MSAA for all render 
	// target formats, so we only need to check quality support.

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels
	{
		/*_In_  DXGI_FORMAT Format							*/mParam.swapChainFormat,
		/*_In_  UINT SampleCount							*/4,
		/*_In_  D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS Flags	*/D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE,
		/*_Out_  UINT NumQualityLevels						*/0
	};
	ThrowIfFailed(mDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels)));

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	// m4xMsaaState = true;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

#ifdef _DEBUG
	LogAdapters();
#endif

	mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (mFenceEvent == nullptr)
		return false;

	CreateCommandObjects();
	CreateRtvAndDsvDescriptorHeaps(APP_NUM_BACK_BUFFERS + RTV_USER_SIZE, 1, RTV_TOY_SIZE);
	CreateSwapChain();

	return true;
}

bool AppBase::InitImgui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& imGuiIO = ImGui::GetIO();
	imGuiIO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	imGuiIO.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	imGuiIO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	imGuiIO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(mHwndWindow);

	ImGui_ImplDX12_InitInfo init_info = {};
	init_info.Device = mDevice.Get();
	init_info.CommandQueue = mCommandQueue.Get();
	init_info.NumFramesInFlight = APP_NUM_FRAME_RESOURCES;
	init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
	// Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
	// (current version of the backend will only allocate one descriptor, future versions will need to allocate more)
	mSrvDescHeapAlloc.Create(mDevice.Get(), mSrvDescriptorHeap.Get());
	init_info.SrvDescriptorHeap = mSrvDescriptorHeap.Get();
	init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return g_appBase->mSrvDescHeapAlloc.Alloc(out_cpu_handle, out_gpu_handle); };
	init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return g_appBase->mSrvDescHeapAlloc.Free(cpu_handle, gpu_handle); };
	ImGui_ImplDX12_Init(&init_info);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != nullptr);

	return true;
}

void AppBase::CreateImGuiDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc
	{
		/* D3D12_DESCRIPTOR_HEAP_TYPE Type	*/D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		/* UINT NumDescriptors				*/APP_SRV_HEAP_SIZE,
		/* D3D12_DESCRIPTOR_HEAP_FLAGS Flags*/D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		/* UINT NodeMask					*/0
	};
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescHeap)));
}

void AppBase::CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {
		/* D3D12_COMMAND_LIST_TYPE Type    */D3D12_COMMAND_LIST_TYPE_DIRECT,
		/* INT Priority                    */0,
		/* D3D12_COMMAND_QUEUE_FLAGS Flags */D3D12_COMMAND_QUEUE_FLAG_NONE,
		/* UINT NodeMask                   */0
	};
	ThrowIfFailed(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mCommandAllocator.GetAddressOf())));

	ThrowIfFailed(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), nullptr, IID_PPV_ARGS(mCommandList.GetAddressOf())));

	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	mCommandList->Close();
}

void AppBase::CreateSwapChain()
{
	// Release the previous swapchain we will be recreating.
	mSwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC sd
	{
		/* DXGI_MODE_DESC BufferDesc					*/
		/* 	UINT Width									*/mParam.backBufferWidth,
		/* 	UINT Height									*/mParam.backBufferHeight,
		/* 	DXGI_RATIONAL RefreshRate					*/
		/*		UINT Numerator							*/60,
		/*		UINT Denominator						*/1,
		/* 	DXGI_FORMAT Format							*/mParam.swapChainFormat,
		/* 	DXGI_MODE_SCANLINE_ORDER ScanlineOrdering	*/DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
		/* 	DXGI_MODE_SCALING Scaling					*/DXGI_MODE_SCALING_UNSPECIFIED,
		/* DXGI_SAMPLE_DESC SampleDesc					*/
		/*	UINT Count									*/m4xMsaaState ? 4u : 1u,
		/*	UINT Quality								*/m4xMsaaState ? (m4xMsaaQuality - 1) : 0u,
		/* DXGI_USAGE BufferUsage						*/DXGI_USAGE_RENDER_TARGET_OUTPUT,
		/* UINT BufferCount								*/APP_NUM_BACK_BUFFERS,
		/* HWND OutputWindow							*/mHwndWindow,
		/* BOOL Windowed								*/true,				// TBD
		/* DXGI_SWAP_EFFECT SwapEffect					*/DXGI_SWAP_EFFECT_FLIP_DISCARD,
		/* UINT Flags									*/DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
	};

	// Note: Swap chain uses queue to perform flush.
#ifdef _ST
	if (mPref.renderAPI == sl::RenderAPI::eD3D12)
		slSetFeatureLoaded(sl::kFeatureDLSS_G, true);
#endif
	ThrowIfFailed(mDxgiFactory->CreateSwapChain(mCommandQueue.Get(), &sd, mSwapChain.GetAddressOf()));
#ifdef _ST
	if (mPref.renderAPI == sl::RenderAPI::eD3D12)
		slSetFeatureLoaded(sl::kFeatureDLSS_G, false);
#endif

	for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; i++)
	{
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		mDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, mhCPUSwapChainBuffer[i]);
	}
}

void AppBase::FlushCommandQueue()
{
	// Advance the fence value to mark commands up to this fence point.
	mFrameCount++;

	// Add an instruction to the command queue to set a new fence point.  Because we 
	// are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mFrameCount));

	// Wait until the GPU has completed commands up to this fence point.
	if (mFence->GetCompletedValue() < mFrameCount)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, L"", false, EVENT_ALL_ACCESS);

		// Fire event when GPU hits current fence.  
		ThrowIfFailed(mFence->SetEventOnCompletion(mFrameCount, eventHandle));

		// Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void AppBase::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.

	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// Compute averages over one second period.
	if ((mTimer.TotalTime() - timeElapsed) >= 1.0f)
	{
		mFPS = (double)frameCnt; // fps = frameCnt / 1
		mMSpFrame = 1000.0f / mFPS;
		mSpFrame = 1.0f / mFPS;

		std::wstring fpsStr = std::to_wstring(mFPS);
		std::wstring mspfStr = std::to_wstring(mMSpFrame);

		std::wstring windowText = mWndCaption +
			L"    fps: " + fpsStr +
			L"   ms/fps: " + mspfStr;

		SetWindowText(mHwndWindow, windowText.c_str());

		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}


LRESULT AppBase::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	// std::cout << "msg: " << std::hex << msg << std::hex << "  |  LPARAM: " << HIWORD(lParam) << " " << LOWORD(lParam) << "  |  WPARAM: " << HIWORD(wParam) << " " << LOWORD(wParam) << std::endl;

	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_CREATE:
		return 0;
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.  
	case WM_ACTIVATE:
		//if (LOWORD(wParam) == WA_INACTIVE)
		//{
		//	mAppPaused = true;
		//	mTimer.Stop();
		//}
		//else
		//{
		//	mAppPaused = false;
		//	mTimer.Start();
		//}
		return 0;

		// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
		mParam.backBufferWidth = (UINT)LOWORD(lParam);
		mParam.backBufferHeight = (UINT)HIWORD(lParam);

		if (mDevice)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				mAppPaused = true;
				mMinimized = true;
				mMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				mAppPaused = false;
				mMinimized = false;
				mMaximized = true;
				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{

				// Restoring from minimized state?
				if (mMinimized)
				{
					mAppPaused = false;
					mMinimized = false;
					OnResize();
				}

				// Restoring from maximized state?
				else if (mMaximized)
				{
					mAppPaused = false;
					mMaximized = false;
					OnResize();
				}
				else if (mResizing)
				{
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;

		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		mResizing = true;
		mTimer.Stop();
		return 0;

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		mResizing = false;
		mTimer.Start();
		OnResize();
		return 0;

		// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

		// The WM_MENUCHAR message is sent when a menu is active and the user presses 
		// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

		// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, LOWORD(lParam), HIWORD(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, LOWORD(lParam), HIWORD(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, LOWORD(lParam), HIWORD(lParam));
		return 0;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		else if ((int)wParam == VK_F2)
			Set4xMsaaState(!m4xMsaaState);

		return 0;
	}

	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

void AppBase::UpdateImGui()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	// Our state

	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
	{
		// ImGui::SetNextWindowPos(Imfloat2(main_viewport->WorkPos.x, main_viewport->WorkPos.y), ImGuiCond_FirstUseEver);
		// ImGui::SetNextWindowSize(Imfloat2((float)mParam.backBufferWidth, (float)mParam.backBufferHeight), ImGuiCond_FirstUseEver);
		static float f = 0.0f;
		static int counter = 0;
		const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
		// ImGui::PushStyleVar();
		ImGui::Begin("Root");
		// ImGui::Begin("Root", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
		// ImGui::SetWindowPos("Root", Imfloat2(main_viewport->WorkPos.x + 100.0f, main_viewport->WorkPos.y + 100.0f));
		// ImGui::SetWindowSize("Root", Imfloat2((float)mParam.backBufferWidth, (float)mParam.backBufferHeight));

		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &mShowDemoWindow);      // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &mShowAnotherWindow);
		ImGui::Checkbox("Show Viewport", &mShowViewport);

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (mShowDemoWindow)
		ImGui::ShowDemoWindow(&mShowDemoWindow);

	// 3. Show another simple window.
	if (mShowAnotherWindow)
	{
		ImGui::Begin("Another Window", &mShowAnotherWindow);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			mShowAnotherWindow = false;
		ImGui::End();
	}
	// 3. Show another simple window.
	if (mShowViewport)
	{
		ImGui::Begin("Viewport1", &mShowViewport);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)


		// mSrvDescHeapAlloc.Alloc(&CurrentBackBufferView(), my_texture_srv_gpu_handle);
		ImGui::Text("Hello from another window!");
		// ImGui::Image((ImTextureID)my_texture_srv_gpu_handle->ptr, Imfloat2((float)mParam.backBufferWidth, (float)mParam.backBufferHeight));
		ImGui::End();
	}
}

void AppBase::RenderImGui()
{
	UpdateImGui();		// override this function
	ImGui::Render();

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = mSwapChainBuffer[mCurrBackBuffer].Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	mCommandList->ResourceBarrier(1, &barrier);

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	mCommandList->ResourceBarrier(1, &barrier);
}

void AppBase::ShowImguiViewport(bool* p_open)
{
	if (!ImGui::Begin("Viewport", p_open))
	{
		ImGui::End();
		return;
	}

	// ImTextureID my_tex_id = gpu ptr;
	//float my_tex_w = (float)io.Fonts->TexWidth;
	//float my_tex_h = (float)io.Fonts->TexHeight;

	//{
	//	Imfloat2 pos = ImGui::GetCursorScreenPos();
	//	ImGui::Text("%.0fx%.0f", my_tex_w, my_tex_h);
	//	Imfloat2 uv_min = Imfloat2(0.0f, 0.0f);                 // Top-left
	//	Imfloat2 uv_max = Imfloat2(1.0f, 1.0f);                 // Lower-right

	//	ImGui::Image(my_tex_id, Imfloat2(my_tex_w, my_tex_h), uv_min, uv_max, tint_col, border_col);
	//}
}
void AppBase::ShowStreamlineWindow()
{
	ImGui::Begin("streamline", &mShowStreamlineWindow);

	if (ImGui::IsAnyItemHovered() || ImGui::IsMouseHoveringRect(ImGui::GetWindowPos(), ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y))) {
		m_ui.MouseOverUI = true;
	}
	else
		m_ui.MouseOverUI = false;

	ImGui::Text("Engine FPS: %.0f ", mFPS);
	if (m_ui.DLSSG_mode != sl::DLSSGMode::eOff) {
		ImGui::Text("True FPS: %.0f ", m_ui.DLSSG_fps);
	}
	// Vsync
	if (m_ui.DLSSG_mode != sl::DLSSGMode::eOff && !m_dev_view) {
		pushDisabled();
		m_ui.EnableVsync = false;
	}
	ImGui::Checkbox("VSync", &m_ui.EnableVsync);
	if (m_ui.DLSSG_mode != sl::DLSSGMode::eOff && !m_dev_view) {
		popDisabled();
	}

	// Resolution 
	std::vector<std::string> Resolutions_strings = { "1280 x 720", "1920 x 1080", "2560 x 1440", "3840 x 2160" };
	std::vector<donut::math::int2> Resolutions_values = { {1280, 720}, {1920, 1080}, {2560, 1440}, {3840, 2160} };
	int resIndex = -1;
	for (auto i = 0; i < Resolutions_values.size(); ++i) {
		if (Resolutions_values[i].x == m_ui.Resolution.x && Resolutions_values[i].y == m_ui.Resolution.y) {
			resIndex = i;
		}
	}
	if (resIndex == -1) {
		Resolutions_strings.push_back(std::to_string(m_ui.Resolution.x) + " x " + std::to_string(m_ui.Resolution.y) + " (custom)");
		Resolutions_values.push_back(m_ui.Resolution);
		resIndex = (int)Resolutions_strings.size() - 1;
	}
	auto preIndex = resIndex;
	if (ImGui::BeginCombo("Resolution", Resolutions_strings[resIndex].c_str()))
	{
		for (auto i = 0; i < Resolutions_strings.size(); ++i)
		{
			bool is_selected = i == resIndex;
			auto is_selected_pre = i == resIndex;
			if (ImGui::Selectable(Resolutions_strings[i].c_str(), is_selected)) resIndex = i;
			if (is_selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	m_ui.Resolution = Resolutions_values[resIndex];
	if (preIndex != resIndex) {
		m_ui.Resolution_changed = true;
	}

	ImGui::Separator();
	ImGui::Checkbox("Developer Menu", &m_dev_view);

	if (!m_dev_view) {
		//
		//  Reflex & Reflex Frame Warp
		//
		ImGui::Separator();

		ImGui::Text("Nvidia Reflex Low Latency");
		ImGui::SameLine();
		if (!m_ui.REFLEX_Supported) pushDisabled();

		if (m_ui.DLSSG_mode != sl::DLSSGMode::eOff)
		{
			auto i = (int)m_ui.REFLEX_Mode - 1;
			i = i < 0 ? 0 : i;
			ImGui::Combo("##Reflex", &i, "On\0On + Boost\0");
			m_ui.REFLEX_Mode = i + 1;
		}
		else
		{
			ImGui::Combo("##Reflex", (int*)&m_ui.REFLEX_Mode, "Off\0On\0On + Boost\0");
		}

		ImGui::Text("Frame Warp");
		ImGui::SameLine();
		if (!m_ui.Latewarp_Supported || !m_ui.REFLEX_Supported) pushDisabled();
		ImGui::Combo("##Latewarp", &m_ui.Latewarp_active, "Off\0On\0");
		if (!m_ui.Latewarp_Supported || !m_ui.REFLEX_Supported) popDisabled();

		//
		//  Generic DLSS
		//

		ImGui::Separator();

		ImGui::Text("NVIDIA DLSS");
		ImGui::SameLine();
		ImGui::Combo("##DLSSMode", &m_dev_view_TopLevelDLSS, "Off\0On\0");

		ImGui::Indent();
		if (m_dev_view_TopLevelDLSS == 0) {
			pushDisabled();
			m_ui.DLSS_Mode = sl::DLSSMode::eOff;
			m_dev_view_dlss_mode = 0;
			m_ui.DLSSG_mode = sl::DLSSGMode::eOff;
			m_ui.REFLEX_Mode = 0;
			m_ui.NIS_Mode = sl::NISMode::eOff;
		}



		//
		//  DLSS Frame Gen
		//

		ImGui::Text("Frame Generation");
		ImGui::SameLine();
		if (!m_ui.DLSSG_Supported || !m_ui.REFLEX_Supported) pushDisabled();
		if (ImGui::Combo("##FrameGeneration", (int*)&m_ui.DLSSG_mode, "Off\0On\0Auto (Dynamic Frame Generation)\0"))
		{
			if (m_ui.DLSSG_mode == sl::DLSSGMode::eOff)
			{
				m_ui.DLSSG_cleanup_needed = true;
			}
		}
		if (m_ui.DLSSG_mode != sl::DLSSGMode::eOff)
		{
			ImGui::Indent();
			ImGui::Text("Generated Frames");
			ImGui::SameLine();
			ImGui::SliderInt("##MultiframeCount", &m_ui.DLSSG_numFrames, 2, m_ui.DLSSG_numFramesMaxMultiplier, "%dx", ImGuiSliderFlags_AlwaysClamp);
			ImGui::Unindent();
		}
		if (!m_ui.DLSSG_Supported || !m_ui.REFLEX_Supported) popDisabled();
		if (m_ui.DLSSG_status != "") ImGui::Text((std::string("State: ") + m_ui.DLSSG_status).c_str());

		//
		//  DLSS SuperRes
		//

		auto DLSSModeNames = std::vector<std::string>({
			"Off##DLSSModes",
			"Auto##DLSSModes",
			"Quality##DLSSModes",
			"Balanced##DLSSModes",
			"Performance##DLSSModes",
			"UltraPerformance##DLSSModes",
			"DLAA##DLSSModes"
			});

		ImGui::Text("Super Resolution");
		ImGui::SameLine();
		if (!m_ui.DLSS_Supported) pushDisabled();
		if (ImGui::BeginCombo("##SuperRes", DLSSModeNames[m_dev_view_dlss_mode].data()))
		{
			for (int i = 0; i < DLSSModeNames.size(); ++i)
			{
				bool is_selected = i == m_dev_view_dlss_mode;

				if (ImGui::Selectable(DLSSModeNames[i].data(), is_selected)) m_dev_view_dlss_mode = i;
				if (is_selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
			if (ImGui::IsItemHovered()) m_ui.MouseOverUI = true;
		}

		if (!m_ui.DLSS_Supported) popDisabled();

		if (m_dev_view_dlss_mode == 0) m_ui.DLSS_Mode = sl::DLSSMode::eOff;
		else if (m_dev_view_dlss_mode == 2) m_ui.DLSS_Mode = sl::DLSSMode::eMaxQuality;
		else if (m_dev_view_dlss_mode == 3) m_ui.DLSS_Mode = sl::DLSSMode::eBalanced;
		else if (m_dev_view_dlss_mode == 4) m_ui.DLSS_Mode = sl::DLSSMode::eMaxPerformance;
		else if (m_dev_view_dlss_mode == 5) m_ui.DLSS_Mode = sl::DLSSMode::eUltraPerformance;
		//else if (m_dev_view_dlss_mode == 6) m_ui.DLSS_Mode = sl::DLSSMode::eUltraQuality;
		else if (m_dev_view_dlss_mode == 6) m_ui.DLSS_Mode = sl::DLSSMode::eDLAA;
		else if (m_dev_view_dlss_mode == 1) {
			if (m_ui.Resolution.x < 1920) m_ui.DLSS_Mode = sl::DLSSMode::eOff;
			else if (m_ui.Resolution.x < 2560) m_ui.DLSS_Mode = sl::DLSSMode::eMaxQuality;
			else if (m_ui.Resolution.x < 3840) m_ui.DLSS_Mode = sl::DLSSMode::eMaxPerformance;
			else m_ui.DLSS_Mode = sl::DLSSMode::eUltraPerformance;
		}

		if (m_ui.DLSS_Mode != sl::DLSSMode::eOff) {
			m_ui.AAMode = UIData::AntiAliasingMode::DLSS;
		}
		else {
			m_ui.AAMode = UIData::AntiAliasingMode::NONE;
		}

		//
		//  NIS Sharpening
		//

		ImGui::Text("NIS Sharpening");
		ImGui::SameLine();
		if (!m_ui.NIS_Supported) pushDisabled();
		int nis_mode = m_ui.NIS_Mode == sl::NISMode::eScaler ? 1 : 0;
		ImGui::Combo("##NISMode", &nis_mode, "Off\0On\0");
		m_ui.NIS_Mode = nis_mode == 1 ? sl::NISMode::eScaler : sl::NISMode::eOff;
		if (nis_mode == 1)
		{
			ImGui::DragFloat("Sharpness", &m_ui.NIS_Sharpness, 0.05f, 0, 1);
		}
		if (!m_ui.NIS_Supported) popDisabled();


		if (ImGui::IsItemHovered()) m_ui.MouseOverUI = true;

		if (m_ui.REFLEX_Mode != 0) {
			ImGui::Indent();
			bool useFrameCap = m_ui.REFLEX_CapedFPS != 0;
			ImGui::Checkbox("Reflex FPS Capping", &useFrameCap);
			if (useFrameCap) {
				if (m_ui.REFLEX_CapedFPS == 0) { m_ui.REFLEX_CapedFPS = 60; }
				ImGui::SameLine();
				ImGui::DragInt("##FPSReflexCap", &m_ui.REFLEX_CapedFPS, 1.f, 20, 240);
			}
			else {
				m_ui.REFLEX_CapedFPS = 0;
			}
			ImGui::Unindent();
		}

		if (!m_ui.REFLEX_Supported) popDisabled();

		if (m_dev_view_TopLevelDLSS == 0) popDisabled();
		ImGui::Unindent();

		//
		//  DeepDVC
		//

		ImGui::Separator();
		ImGui::Text("NVIDIA DeepDVC");
		ImGui::Indent();
		ImGui::Text("Supported: %s", m_ui.DeepDVC_Supported ? "yes" : "no");
		if (m_ui.DeepDVC_Supported) {
			int deeoDVC_mode = m_ui.DeepDVC_Mode == sl::DeepDVCMode::eOn ? 1 : 0;
			ImGui::Text("DeepDVC Mode");
			ImGui::SameLine();
			ImGui::Combo("##DeepDVC Mode", &deeoDVC_mode, "Off\0On\0");
			m_ui.DeepDVC_Mode = deeoDVC_mode == 1 ? sl::DeepDVCMode::eOn : sl::DeepDVCMode::eOff;
			if (m_ui.DeepDVC_Mode == sl::DeepDVCMode::eOn)
			{
				ImGui::Text("VRAM = %4.2f MB", m_ui.DeepDVC_VRAM / 1024 / 1024.0f);
				ImGui::Text("Intensity");
				ImGui::SameLine();
				ImGui::DragFloat("##Intensity", &m_ui.DeepDVC_Intensity, 0.01f, 0, 1);
				ImGui::Text("Saturation Boost");
				ImGui::SameLine();
				ImGui::DragFloat("##Saturation Boost", &m_ui.DeepDVC_SaturationBoost, 0.01f, 0, 1);
			}
		}


	}

	else { // if m_dev_view

		//
		// DLSS SuperRes and AA
		//

		ImGui::Separator();
		ImGui::PushStyleColor(ImGuiCol_Text, TITLE_COL);
		ImGui::Text("AA and DLSS");
		ImGui::PopStyleColor();

		ImGui::Text("DLSS_Supported: %s", m_ui.DLSS_Supported ? "yes" : "no");

		if (m_ui.DLSS_Supported) {
			ImGui::Combo("AA Mode", (int*)&m_ui.AAMode, "None\0TemporalAA\0DLSS\0");
		}
		else {
			ImGui::Combo("TAA Fallback", (int*)&m_ui.AAMode, "None\0TemporalAA");
		}

		if (m_ui.AAMode == UIData::AntiAliasingMode::TEMPORAL) {
			ImGui::Combo("TAA Camera Jitter", (int*)&m_ui.TemporalAntiAliasingJitter, "MSAA\0Halton\0R2\0White Noise\0");
		}


		if (m_ui.AAMode == UIData::AntiAliasingMode::DLSS)
		{
			if (m_ui.DLSS_Mode == sl::DLSSMode::eOff) m_ui.DLSS_Mode = sl::DLSSMode::eBalanced;

			// We do not show 'eOff' 
			const char* DLSSModeNames[] = {
				"Off",
				"Performance",
				"Balanced",
				"Quality",
				"Ultra-Performance",
				"Ultra-Quality",
				"DLAA"
			};

			if (ImGui::BeginCombo("DLSS Mode", DLSSModeNames[(int)m_ui.DLSS_Mode]))
			{
				for (int i = 0; i < static_cast<int>(sl::DLSSMode::eCount); ++i)
				{
					if ((i == static_cast<int>(sl::DLSSMode::eUltraQuality)) || (i == static_cast<int>(sl::DLSSMode::eOff))) continue;

					bool is_selected = (i == (int)m_ui.DLSS_Mode);

					if (ImGui::Selectable(DLSSModeNames[i], is_selected)) {
						m_ui.DLSS_Mode = (sl::DLSSMode)i;
					}
					if (is_selected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			auto PresetSlotNames = std::vector<std::string>({
				"Off##Presets",
				"MaxPerformance##Presets",
				"Balanced##Presets",
				"MaxQuality##Presets",
				"UltraPerformance##Presets",
				"UltraQuality##Presets",
				"DLAA##Presets"
				});

			const std::map<sl::DLSSPreset, std::string> DLSSPresetToDropdownMap = {
				{sl::DLSSPreset::eDefault, "Default##Presets"},
				{sl::DLSSPreset::ePresetA, "Preset A##Presets"},
				{sl::DLSSPreset::ePresetB, "Preset B##Presets"},
				{sl::DLSSPreset::ePresetC, "Preset C##Presets"},
				{sl::DLSSPreset::ePresetD, "Preset D##Presets"},
				{sl::DLSSPreset::ePresetE, "Preset E##Presets"},
				{sl::DLSSPreset::ePresetF, "Preset F##Presets"},
				{sl::DLSSPreset::ePresetJ, "Preset J##Presets"},
			};

			if (ImGui::CollapsingHeader("Presets")) {
				ImGui::Indent();

				for (int j = 0; j < static_cast<int>(sl::DLSSMode::eCount); j++) {

					if ((j == static_cast<int>(sl::DLSSMode::eUltraQuality)) || (j == static_cast<int>(sl::DLSSMode::eOff))) continue;

					const std::string* currentPresetString = nullptr;

					auto currentPreset = DLSSPresetToDropdownMap.find(m_ui.DLSS_presets[j]);

					if (currentPreset != DLSSPresetToDropdownMap.end()) {
						currentPresetString = &DLSSPresetToDropdownMap.at(m_ui.DLSS_presets[j]);
					}
					else {
						currentPresetString = &DLSSPresetToDropdownMap.at(sl::DLSSPreset::eDefault);
						SL_LOG_INFO("Warning: There is a mismatch in the preset supported by the sample and the preset selected by the snippet");
					}

					if (ImGui::BeginCombo(PresetSlotNames[j].c_str(), currentPresetString->data())) {
						for (const auto& [presetEnum, presetName] : DLSSPresetToDropdownMap) {

							bool is_selected = (presetEnum == m_ui.DLSS_presets[j]);

							if (ImGui::Selectable(presetName.data(), is_selected)) m_ui.DLSS_presets[j] = presetEnum;
							if (is_selected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
						if (ImGui::IsItemHovered()) m_ui.MouseOverUI = true;
					}
				}
				ImGui::Unindent();
			}

			static const char* DLSSResModeNames[] = {
				"Fixed",
				"Dynamic"
			};

			if (ImGui::BeginCombo("DLSS Resolution Mode", DLSSResModeNames[(int)m_ui.DLSS_Resolution_Mode]))
			{
				for (int i = 0; i < (int)UIData::RenderingResolutionMode::COUNT; ++i)
				{
					bool is_selected = (i == (int)m_ui.DLSS_Resolution_Mode);
					if (ImGui::Selectable(DLSSResModeNames[i], is_selected)) m_ui.DLSS_Resolution_Mode = (UIData::RenderingResolutionMode)i;
					if (is_selected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			if (m_ui.DLSS_Resolution_Mode == UIData::RenderingResolutionMode::DYNAMIC)
			{
				if (ImGui::Button("Change Res"))
				{
					m_ui.DLSS_Dynamic_Res_change = true;
				}
			}

			ImGui::Checkbox("Debug: Show full input buffer", &m_ui.DLSS_DebugShowFullRenderingBuffer);
			ImGui::Checkbox("Debug: Force Extent use", &m_ui.DLSS_always_use_extents);

			ImGui::Checkbox("Overide LOD Bias", &m_ui.DLSS_lodbias_useoveride);
			if (m_ui.DLSS_lodbias_useoveride) {
				ImGui::SameLine();
				ImGui::SliderFloat("", &m_ui.DLSS_lodbias_overide, -2, 2);
			}

		}

		//
		// Reflex
		//

		ImGui::Separator();
		ImGui::PushStyleColor(ImGuiCol_Text, TITLE_COL);
		ImGui::Text("Reflex");
		ImGui::PopStyleColor();

		ImGui::Text("Reflex Supported (PCL tracking): %s", m_ui.REFLEX_Supported ? "yes" : "no");
		ImGui::Text("Reflex LowLatency Supported: %s", m_ui.REFLEX_LowLatencyAvailable ? "yes" : "no");
		if (m_ui.REFLEX_Supported && m_ui.REFLEX_LowLatencyAvailable) {
			ImGui::Combo("Reflex Low Latency", (int*)&m_ui.REFLEX_Mode, "Off\0On\0On + Boost\0");

			bool useFrameCap = m_ui.REFLEX_CapedFPS != 0;
			ImGui::Checkbox("Reflex FPS Capping", &useFrameCap);

			if (useFrameCap) {
				if (m_ui.REFLEX_CapedFPS == 0) { m_ui.REFLEX_CapedFPS = 60; }
				ImGui::SameLine();
				ImGui::DragInt("##FPSReflexCap", &m_ui.REFLEX_CapedFPS, 1.f, 20, 240);
			}
			else {
				m_ui.REFLEX_CapedFPS = 0;
			}

			if (ImGui::CollapsingHeader("Stats Report")) {
				ImGui::Indent();
				ImGui::Text(m_ui.REFLEX_Stats.c_str());
				ImGui::Unindent();
			}
		}

		ImGui::Text("Frame Warp");
		ImGui::SameLine();
		if (!m_ui.Latewarp_Supported || !m_ui.REFLEX_Supported) pushDisabled();
		ImGui::Combo("##Latewarp", &m_ui.Latewarp_active, "Off\0On\0");
		if (!m_ui.Latewarp_Supported || !m_ui.REFLEX_Supported) popDisabled();

		//
		// DLSS Frame Generatioon
		//

		ImGui::Separator();
		ImGui::PushStyleColor(ImGuiCol_Text, TITLE_COL);
		ImGui::Text("DLSS-G");
		ImGui::PopStyleColor();

		ImGui::Text("DLSS-G Supported: %s", m_ui.DLSSG_Supported ? "yes" : "no");
		if (m_ui.DLSSG_Supported) {

			if (m_ui.REFLEX_Mode == (int)sl::ReflexMode::eOff) {
				ImGui::Text("Reflex needs to be enabled for DLSSG to be enabled");
				m_ui.DLSSG_mode = sl::DLSSGMode::eOff;
			}
			else {
				if (ImGui::Combo("DLSS-G Mode", (int*)&m_ui.DLSSG_mode, "Off\0On\0Auto (Dynamic Frame Generation)\0"))
				{
					if (m_ui.DLSSG_mode == sl::DLSSGMode::eOff)
					{
						m_ui.DLSSG_cleanup_needed = true;
					}
				}
			}


		}


		//
		//  NIS Sharpening
		//

		ImGui::Separator();
		ImGui::PushStyleColor(ImGuiCol_Text, TITLE_COL);
		ImGui::Text("NIS Sharpening");
		ImGui::PopStyleColor();

		ImGui::Text("NIS Supported: %s", m_ui.NIS_Supported ? "yes" : "no");

		if (m_ui.NIS_Supported) {
			int nis_mode = m_ui.NIS_Mode == sl::NISMode::eScaler ? 1 : 0;
			ImGui::Combo("NIS Mode", &nis_mode, "Off\0On\0");
			m_ui.NIS_Mode = nis_mode == 1 ? sl::NISMode::eScaler : sl::NISMode::eOff;
			ImGui::DragFloat("Sharpness", &m_ui.NIS_Sharpness, 0.05f, 0, 1);
		}

		//
		//  Additional Settings
		//

		ImGui::Separator();

		if (ImGui::CollapsingHeader("Additional settings")) {

			ImGui::Indent();

			// Scene
			ImGui::PushStyleColor(ImGuiCol_Text, TITLE_COL);
			ImGui::Text("Scene");
			ImGui::PopStyleColor();

			// // Scene 
			// const std::string currentScene = m_app->GetCurrentSceneName();
			// if (ImGui::BeginCombo("Scene", currentScene.c_str()))
			// {
			// 	const std::vector<std::string>& scenes = m_app->GetAvailableScenes();
			// 	for (const std::string& scene : scenes)
			// 	{
			// 		bool is_selected = scene == currentScene;
			// 		if (ImGui::Selectable(scene.c_str(), is_selected))
			// 			m_app->SetCurrentSceneName(scene);
			// 		if (is_selected)
			// 			ImGui::SetItemDefaultFocus();
			// 	}
			// 	ImGui::EndCombo();
			// }

			// Animation
			ImGui::Checkbox("Animate", &m_ui.EnableAnimations);
			if (m_ui.EnableAnimations) {
				ImGui::SameLine();
				ImGui::DragFloat("Speed", &m_ui.AnimationSpeed, 0.01f, 0.01f, 2.f);
			}

			ImGui::SliderFloat("Ambient Intensity", &m_ui.AmbientIntensity, 0.f, 1.f);

			ImGui::Checkbox("Enable Procedural Sky", &m_ui.EnableProceduralSky);
			if (m_ui.EnableProceduralSky && ImGui::CollapsingHeader("Sky Parameters"))
			{
				ImGui::Indent();
				ImGui::SliderFloat("Brightness", &m_ui.SkyParams.brightness, 0.f, 1.f);
				ImGui::SliderFloat("Glow Size", &m_ui.SkyParams.glowSize, 0.f, 90.f);
				ImGui::SliderFloat("Glow Sharpness", &m_ui.SkyParams.glowSharpness, 1.f, 10.f);
				ImGui::SliderFloat("Glow Intensity", &m_ui.SkyParams.glowIntensity, 0.f, 1.f);
				ImGui::SliderFloat("Horizon Size", &m_ui.SkyParams.horizonSize, 0.f, 90.f);
				ImGui::Unindent();
			}

			// Additional Load
			ImGui::Separator();
			ImGui::PushStyleColor(ImGuiCol_Text, TITLE_COL);
			ImGui::Text("Additional Load");
			ImGui::PopStyleColor();

			// CPU Load
			bool enableCPULoad = m_ui.CpuLoad != 0;
			ImGui::Checkbox("Additional CPU Load", &enableCPULoad);
			if (enableCPULoad) {
				if (m_ui.CpuLoad == 0) { m_ui.CpuLoad = 0.5; }
				ImGui::SameLine();
				ImGui::DragFloat("##CPULoad", &m_ui.CpuLoad, 1.f, 0.001f, 50);
			}
			else {
				m_ui.CpuLoad = 0;
			}
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Runs a while loop for a given number of ms");


			// GPU Load
			bool enableGPULoad = m_ui.GpuLoad != 0;
			ImGui::Checkbox("Additional GPU Load", &enableGPULoad);
			if (enableGPULoad) {
				if (m_ui.GpuLoad == 0) { m_ui.GpuLoad = 1; }
				ImGui::SameLine();
				ImGui::DragInt("##GPULoad", &m_ui.GpuLoad, 1.f, 1, 300);
			}
			else {
				m_ui.GpuLoad = 0;
			}
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Repeats the Gbuffer pass an additional number of times");

			ImGui::Separator();
			ImGui::PushStyleColor(ImGuiCol_Text, TITLE_COL);
			ImGui::Text("Debug visualisation");
			ImGui::PopStyleColor();

			ImGui::Checkbox("Overlay Buffers", &m_ui.VisualiseBuffers);
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("Shows the depth and motion vector buffers.");

			// Pipeline
			ImGui::Separator();
			ImGui::PushStyleColor(ImGuiCol_Text, TITLE_COL);
			ImGui::Text("Pipeline options");
			ImGui::PopStyleColor();

			ImGui::Checkbox("Enable SSAO", &m_ui.EnableSsao);
			ImGui::Checkbox("Enable Bloom", &m_ui.EnableBloom);

			if (m_ui.EnableBloom && ImGui::CollapsingHeader("Bloom Settings"))
			{
				ImGui::Indent();
				ImGui::DragFloat("Bloom Sigma", &m_ui.BloomSigma, 0.01f, 0.1f, 100.f);
				ImGui::DragFloat("Bloom Alpha", &m_ui.BloomAlpha, 0.01f, 0.01f, 1.0f);
				ImGui::Unindent();
			}

			ImGui::Checkbox("Enable Shadows", &m_ui.EnableShadows);
			ImGui::Checkbox("Enable Tonemapping", &m_ui.EnableToneMapping);
			if (m_ui.EnableToneMapping && ImGui::CollapsingHeader("ToneMapping Params"))
			{
				ImGui::Indent();
				ImGui::DragFloat("Exposure Bias", &m_ui.ToneMappingParams.exposureBias, 0.1f, -2, 2);
				ImGui::Unindent();
			}

			if (m_ui.NIS_Mode == sl::NISMode::eOff)
			{
				// Viewport Extent
				ImGui::Separator();
				ImGui::PushStyleColor(ImGuiCol_Text, TITLE_COL);
				ImGui::Text("Backbuffer Viewport Extent");
				ImGui::PopStyleColor();

				uint32_t nViewports = (uint32_t)m_ui.BackBufferExtents.size();
				nViewports = std::max(1u, nViewports); // can't have 0 viewports
				std::vector<const char*> nViewports_strings = { "1", "2", "3" };
				if (ImGui::BeginCombo("nViewports", nViewports_strings[nViewports - 1]))
				{
					for (auto i = 0; i < nViewports_strings.size(); ++i)
					{
						bool is_selected = (i == (nViewports - 1));
						if (ImGui::Selectable(nViewports_strings[i], is_selected)) nViewports = i + 1;
						if (is_selected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				// check if we need to add/remove viewports based on the new selection
				if (nViewports == 3)
				{
					m_ui.BackBufferExtents.resize(3);
				}
				else if (nViewports == 2)
				{
					m_ui.BackBufferExtents.resize(2);
				}
				else
				{
					m_ui.BackBufferExtents.resize(1);
					float viewportX{ static_cast<float>(m_ui.BackBufferExtents[0].left) };
					float viewportY{ static_cast<float>(m_ui.BackBufferExtents[0].top) };
					float viewportW{ static_cast<float>(m_ui.BackBufferExtents[0].width) };
					float viewportH{ static_cast<float>(m_ui.BackBufferExtents[0].height) };

					// ensure values set to the slider are in the valid range.
					viewportX = std::clamp(viewportX, 0.f, static_cast<uint32_t>(viewportW) > 1 ? viewportW - 1 : 0);
					ImGui::SliderFloat("OffsetLeft", &(viewportX), 0.f, static_cast<uint32_t>(viewportW) > 1 ? viewportW - 1 : 0);

					viewportY = std::clamp(viewportY, 0.f, static_cast<uint32_t>(viewportH) > 1 ? viewportH - 1 : 0);
					ImGui::SliderFloat("OffsetTop", &(viewportY), 0.f, static_cast<uint32_t>(viewportH) > 1 ? viewportH - 1 : 0);

					int width = m_ui.Resolution.x;
					int height = m_ui.Resolution.y;
					viewportW = std::clamp(viewportW, 0.f, static_cast<float>(width - viewportX));
					ImGui::SliderFloat("Width", &(viewportW), 0.f, static_cast<float>(width - viewportX));

					viewportH = std::clamp(viewportH, 0.f, static_cast<float>(height - viewportY));
					ImGui::SliderFloat("Height", &(viewportH), 0.f, static_cast<float>(height - viewportY));

					m_ui.BackBufferExtents[0] = { static_cast<uint32_t>(viewportY), static_cast<uint32_t>(viewportX), static_cast<uint32_t>(viewportW), static_cast<uint32_t>(viewportH) };
				}
			}

			ImGui::Unindent();

		}

	}

	ImGui::End();

}


void AppBase::UpdateFeatureAvailable()
{
	sl::AdapterInfo adapterInfo;

	if (mPref.renderAPI == sl::RenderAPI::eD3D12) {
		// auto a = ((ID3D12Device*)m_Device->getNativeObject(nvrhi::ObjectTypes::D3D12_Device))->GetAdapterLuid();
		auto a = mDevice->GetAdapterLuid();
		adapterInfo.deviceLUID = (uint8_t*)&a;
		adapterInfo.deviceLUIDSizeInBytes = sizeof(LUID);
	}
	else
	{
		SL_LOG_WARN("Abnormal API");
	}

	sl::FeatureRequirements requirements{};
	if (SL_FAILED(result, slGetFeatureRequirements(sl::kFeatureDLSS, requirements)))
	{
		// Feature is not requested on slInit or failed to load, check logs, handle error
		SL_LOG_WARN("DLSS is not fully functional on this system.");
	}
	else
	{
		// Check if features are fully functional (2nd call of slIsFeatureSupported onwards)
		m_dlss_available = slIsFeatureSupported(sl::kFeatureDLSS, adapterInfo) == sl::Result::eOk;
		if (m_dlss_available)
		{
			SL_LOG_INFO("DLSS is supported on this system.");
		}
		else SL_LOG_WARN("DLSS is not fully functional on this system.");
	}
	m_dlss_consts;
	
	if (SL_FAILED(result, slGetFeatureRequirements(sl::kFeatureNIS, requirements)))
	{
		// Feature is not requested on slInit or failed to load, check logs, handle error
		SL_LOG_WARN("NIS is not fully functional on this system.");
	}
	else
	{
		m_nis_available = slIsFeatureSupported(sl::kFeatureNIS, adapterInfo) == sl::Result::eOk;
		if (m_nis_available)
		{
			SL_LOG_INFO("NIS is supported on this system.");
		}
		else SL_LOG_WARN("NIS is not fully functional on this system.");
	}
	m_nis_consts;

	if (SL_FAILED(result, slGetFeatureRequirements(sl::kFeatureDLSS_G, requirements)))
	{
		// Feature is not requested on slInit or failed to load, check logs, handle error
		SL_LOG_WARN("DLSS-G is not fully functional on this system.");
	}
	else
	{
		m_dlssg_available = slIsFeatureSupported(sl::kFeatureDLSS_G, adapterInfo) == sl::Result::eOk;
		if (m_dlssg_available)
		{
			SL_LOG_INFO("DLSS-G is supported on this system.");
		}
		else SL_LOG_WARN("DLSS-G is not fully functional on this system.");
	}
	m_dlssg_consts;

	if (SL_FAILED(result, slGetFeatureRequirements(sl::kFeatureReflex, requirements)))
	{
		// Feature is not requested on slInit or failed to load, check logs, handle error
		SL_LOG_WARN("Reflex is not fully functional on this system.");
	}
	else
	{
		m_reflex_available = slIsFeatureSupported(sl::kFeatureReflex, adapterInfo) == sl::Result::eOk;
		if (m_reflex_available)
		{
			SL_LOG_INFO("Reflex is supported on this system.");
		}
		else SL_LOG_WARN("Reflex is not fully functional on this system.");
	}
	m_reflex_consts;

	if (SL_FAILED(result, slGetFeatureRequirements(sl::kFeaturePCL, requirements)))
	{
		// Feature is not requested on slInit or failed to load, check logs, handle error
		SL_LOG_WARN("PCL is not fully functional on this system.");
	}
	else
	{
		m_pcl_available = SuccessCheck(slIsFeatureSupported(sl::kFeaturePCL, adapterInfo), str_temp);
		if (m_pcl_available)
		{
			SL_LOG_INFO("PCL is supported on this system.");
		}
		else SL_LOG_WARN("PCL is not fully functional on this system.");
	}
	m_pcl_consts;

	if (SL_FAILED(result, slGetFeatureRequirements(sl::kFeatureDeepDVC, requirements)))
	{
		// Feature is not requested on slInit or failed to load, check logs, handle error
		SL_LOG_WARN("DeepDVC is not fully functional on this system.");
	}
	else
	{
		m_deepdvc_available = slIsFeatureSupported(sl::kFeatureDeepDVC, adapterInfo) == sl::Result::eOk;
		if (m_deepdvc_available)
		{
			SL_LOG_INFO("DeepDVC is supported on this system.");
		}
		else SL_LOG_WARN("DeepDVC is not fully functional on this system.");
	}
	m_deepdvc_consts;


	if (SL_FAILED(result, slGetFeatureRequirements(sl::kFeatureLatewarp, requirements)))
	{
		// Feature is not requested on slInit or failed to load, check logs, handle error
		SL_LOG_WARN("Latewarp is not fully functional on this system.");
	}
	else
	{
		m_latewarp_available = slIsFeatureSupported(sl::kFeatureLatewarp, adapterInfo) == sl::Result::eOk;
		if (m_latewarp_available)
		{
			SL_LOG_INFO("Latewarp is supported on this system.");
		}
		else SL_LOG_WARN("Latewarp is not fully functional on this system.");
	}
}

std::wstring AppBase::GetSlInterposerDllLocation() {

	wchar_t path[MAX_PATH] = { 0 };
#ifdef _WIN32
	if (GetModuleFileNameW(nullptr, path, dim(path)) == 0)
		return std::wstring();
#else // _WIN32
#error Unsupported platform for GetSlInterposerDllLocation!
#endif // _WIN32

	auto basePath = std::filesystem::path(path).parent_path().parent_path().parent_path();
	// auto dllPath = basePath.wstring().append(L"\\StreamlineCore\\_bin\\sl.interposer.dll");
	// auto dllPath = basePath.wstring().append(L"\\Libraries\\Include\\DirectX\\Streamline\\bin\\x64\\sl.interposer.dll");
	// auto dllPath = basePath.wstring().append(L"\\Libraries\\Include\\DirectX\\Streamline2\\bin\\x64\\sl.interposer.dll");
	auto dllPath = basePath.wstring().append(L"\\StreamlineCore\\streamline\\bin\\x64\\sl.interposer.dll");
	return dllPath;
}

bool AppBase::InitSLLog()
{
	auto log = sl::log::getInterface();
	if (log == nullptr)
	{
		return false;
	}
	mPref.showConsole = true;
	mPref.logLevel = sl::LogLevel::eVerbose;
	log->enableConsole(mPref.showConsole);
	log->setLogLevel(mPref.logLevel);
	log->setLogPath(mPref.pathToLogsAndData);
	log->setLogCallback((void*)mPref.logMessageCallback);
	log->setLogName(L"sl.log");

	SL_LOG_VERBOSE("INIT SL Log");

	return true;
}


bool AppBase::LoadStreamline()
{
#ifdef _ST
	if (mSLInitialised) {
		SL_LOG_INFO("SLWrapper is already initialised.");
		return true;
	}
	auto pathDll = GetSlInterposerDllLocation();

	// use editted sl.interposer.lib (for debug)
	if (!sl::security::verifyEmbeddedSignature(pathDll.c_str()))
	{
		// SL module not signed, disable SL
		SL_LOG_WARN("Signature Error!");
	}
	// else
	{
		// 1. Initiailize (plugin load)
		{
			mPref.allocateCallback = nullptr;
			mPref.releaseCallback = nullptr;
			mPref.logMessageCallback = nullptr;
			mPref.showConsole = true;
			mPref.applicationId = APP_ID;
			mPref.logLevel = sl::LogLevel::eVerbose;
			sl::Feature myFeatures[] = {
				sl::kFeatureDLSS,
				sl::kFeatureNIS,
				sl::kFeatureDLSS_G,	// 현재 DLL 로드 시 오류가 발생하여 주석 처리
				sl::kFeatureReflex,
				sl::kFeatureDeepDVC,
				sl::kFeatureLatewarp,
				sl::kFeaturePCL
			};
			mPref.featuresToLoad = myFeatures;
			mPref.numFeaturesToLoad = static_cast<uint32_t>(std::size(myFeatures));
			mPref.renderAPI = sl::RenderAPI::eD3D12;
			mPref.pathToLogsAndData = L"..\\StreamlineCore\\streamline\\bin\\x64\\";
			// mPref.flags &= ~sl::PreferenceFlags::eAllowOTA | ~sl::PreferenceFlags::eLoadDownloadedPlugins;
			mPref.flags |= sl::PreferenceFlags::eAllowOTA | sl::PreferenceFlags::eLoadDownloadedPlugins;
			// mPref.flags |= PreferenceFlags::eUseFrameBasedResourceTagging; // 태깅 관련 사라진 기능

			mSLInitialised = SuccessCheck(slInit(mPref, SDK_VERSION), str_temp);
			if (!mSLInitialised) {
				SL_LOG_WARN("Failed to initailze SL: %s", str_temp);
				return false;
			}

			// 단 하나의 swapchain에 대해서만 적용을 진행해야한다.
			// DLSS_G 기능은 feature에서 제거가 되어있어 실제 동작은 불가
			if (mPref.renderAPI == sl::RenderAPI::eD3D12)
				slSetFeatureLoaded(sl::kFeatureDLSS_G, false);
		}

//		// 2. manual hooking (허나, slInit을 적용하여 manual hooking 없이 정상 동작이 이뤄진다.)
//		{
//			auto mod = LoadLibrary(pathDll.c_str());
//
//			// These are the exports from SL library
//			typedef HRESULT(WINAPI* PFunCreateDXGIFactory)(REFIID, void**);
//			typedef HRESULT(WINAPI* PFunCreateDXGIFactory1)(REFIID, void**);
//			typedef HRESULT(WINAPI* PFunCreateDXGIFactory2)(UINT, REFIID, void**);
//			typedef HRESULT(WINAPI* PFunD3D12GetDebugInterface)(REFIID, void**);
//			typedef HRESULT(WINAPI* PFunDXGIGetDebugInterface)(REFIID, void**);
//			typedef HRESULT(WINAPI* PFunDXGIGetDebugInterface1)(UINT, REFIID, void**);
//			typedef HRESULT(WINAPI* PFunD3D12CreateDevice)(IUnknown*, D3D_FEATURE_LEVEL, REFIID, void**);
//
//			// Map functions from SL and use them instead of standard DXGI/D3D12 API
//			auto slCreateDXGIFactory = reinterpret_cast<PFunCreateDXGIFactory>(GetProcAddress(mod, "CreateDXGIFactory"));
//			auto slCreateDXGIFactory1 = reinterpret_cast<PFunCreateDXGIFactory1>(GetProcAddress(mod, "CreateDXGIFactory1"));
//			auto slCreateDXGIFactory2 = reinterpret_cast<PFunCreateDXGIFactory2>(GetProcAddress(mod, "CreateDXGIFactory2"));
//			auto slD3D12CreateDevice = reinterpret_cast<PFunD3D12CreateDevice>(GetProcAddress(mod, "D3D12CreateDevice"));
//			// Debug Layer
//#if defined(DEBUG) || defined(_DEBUG) 
//			auto slD3D12GetDebugInterface = reinterpret_cast<PFunD3D12GetDebugInterface>(GetProcAddress(mod, "D3D12GetDebugInterface"));
//			auto slDXGIGetDebugInterface1 = reinterpret_cast<PFunDXGIGetDebugInterface1>(GetProcAddress(mod, "DXGIGetDebugInterface1"));
//			ThrowIfFailed(slD3D12GetDebugInterface(IID_PPV_ARGS(&mD12Debug)));
//			mD12Debug->EnableDebugLayer();
//			ThrowIfFailed(slDXGIGetDebugInterface1(0, IID_PPV_ARGS(&mDxgiDebug)));
//
//			SL_LOG_INFO("EnableLeakTrackingForThread");
//			mDxgiDebug->EnableLeakTrackingForThread();
//#endif
//
//			// Create DXGI factory
//			ThrowIfFailed(slCreateDXGIFactory2(_DEBUG, IID_PPV_ARGS(&mDxgiFactory)));
//			// Try to create hardware device.
//			HRESULT hardwareResult = slD3D12CreateDevice(
//				nullptr,             // default adapter
//				D3D_FEATURE_LEVEL_11_0,
//				IID_PPV_ARGS(&mDevice));
//			// Fallback to WARP device.
//			if (FAILED(hardwareResult))
//			{
//				Microsoft::WRL::ComPtr<IDXGIAdapter> pWarpAdapter;
//				ThrowIfFailed(mDxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));
//
//				ThrowIfFailed(slD3D12CreateDevice(
//					pWarpAdapter.Get(),
//					D3D_FEATURE_LEVEL_11_0,
//					IID_PPV_ARGS(&mDevice)));
//			}
//		}
	}
#endif
#if defined(DEBUG) || defined(_DEBUG) 
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&mD12Debug)));
	ThrowIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&mDxgiDebug)));
#endif
	ThrowIfFailed(CreateDXGIFactory2(_DEBUG, IID_PPV_ARGS(&mDxgiFactory)));
	// Try to create hardware device.
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // default adapter
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&mDevice));
	// Fallback to WARP device.
	if (FAILED(hardwareResult))
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(mDxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&mDevice)));
	}

#ifdef _ST
	// if (mPref.renderAPI == sl::RenderAPI::eD3D11)
	// 	SuccessCheck(slSetD3DDevice((ID3D11Device*)mDevice.Get()), "slSetD3DDevice");
	if (mPref.renderAPI == sl::RenderAPI::eD3D12)
		if (!SuccessCheck(slSetD3DDevice((ID3D12Device*)mDevice.Get()), str_temp))
		{
			SL_LOG_WARN(str_temp.c_str());
		}
	// if (mPref.renderAPI == sl::RenderAPI::eVULKAN)
	// 	SuccessCheck(slSetVulkanInfo(*((sl::VulkanInfo*)mDevice.Get())), "slSetVulkanInfo");
#endif

	ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)));

	mParam.cbvSrvUavDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels
	{
		/*_In_  DXGI_FORMAT Format							*/mParam.swapChainFormat,
		/*_In_  UINT SampleCount							*/4,
		/*_In_  D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS Flags	*/D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE,
		/*_Out_  UINT NumQualityLevels						*/0
	};
	ThrowIfFailed(mDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels)));

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	// m4xMsaaState = true;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

	#ifdef _DEBUG
	LogAdapters();
	#endif

	mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (mFenceEvent == nullptr)
	return false;

	CreateCommandObjects();
	CreateRtvAndDsvDescriptorHeaps(APP_NUM_BACK_BUFFERS + RTV_USER_SIZE, 1, RTV_TOY_SIZE);
	CreateSwapChain();


	// 사용 가능 기능 검사
	UpdateFeatureAvailable();

	return true;
}

bool AppBase::SuccessCheck(sl::Result result, std::string& o_log)
{
	if (result == sl::Result::eOk)
		return true;

	const std::map< const sl::Result, const std::string> errors = {
			{sl::Result::eErrorIO,"eErrorIO"},
			{sl::Result::eErrorDriverOutOfDate,"eErrorDriverOutOfDate"},
			{sl::Result::eErrorOSOutOfDate,"eErrorOSOutOfDate"},
			{sl::Result::eErrorOSDisabledHWS,"eErrorOSDisabledHWS"},
			{sl::Result::eErrorDeviceNotCreated,"eErrorDeviceNotCreated"},
			{sl::Result::eErrorAdapterNotSupported,"eErrorAdapterNotSupported"},
			{sl::Result::eErrorNoPlugins,"eErrorNoPlugins"},
			{sl::Result::eErrorVulkanAPI,"eErrorVulkanAPI"},
			{sl::Result::eErrorDXGIAPI,"eErrorDXGIAPI"},
			{sl::Result::eErrorD3DAPI,"eErrorD3DAPI"},
			{sl::Result::eErrorNRDAPI,"eErrorNRDAPI"},
			{sl::Result::eErrorNVAPI,"eErrorNVAPI"},
			{sl::Result::eErrorReflexAPI,"eErrorReflexAPI"},
			{sl::Result::eErrorNGXFailed,"eErrorNGXFailed"},
			{sl::Result::eErrorJSONParsing,"eErrorJSONParsing"},
			{sl::Result::eErrorMissingProxy,"eErrorMissingProxy"},
			{sl::Result::eErrorMissingResourceState,"eErrorMissingResourceState"},
			{sl::Result::eErrorInvalidIntegration,"eErrorInvalidIntegration"},
			{sl::Result::eErrorMissingInputParameter,"eErrorMissingInputParameter"},
			{sl::Result::eErrorNotInitialized,"eErrorNotInitialized"},
			{sl::Result::eErrorComputeFailed,"eErrorComputeFailed"},
			{sl::Result::eErrorInitNotCalled,"eErrorInitNotCalled"},
			{sl::Result::eErrorExceptionHandler,"eErrorExceptionHandler"},
			{sl::Result::eErrorInvalidParameter,"eErrorInvalidParameter"},
			{sl::Result::eErrorMissingConstants,"eErrorMissingConstants"},
			{sl::Result::eErrorDuplicatedConstants,"eErrorDuplicatedConstants"},
			{sl::Result::eErrorMissingOrInvalidAPI,"eErrorMissingOrInvalidAPI"},
			{sl::Result::eErrorCommonConstantsMissing,"eErrorCommonConstantsMissing"},
			{sl::Result::eErrorUnsupportedInterface,"eErrorUnsupportedInterface"},
			{sl::Result::eErrorFeatureMissing,"eErrorFeatureMissing"},
			{sl::Result::eErrorFeatureNotSupported,"eErrorFeatureNotSupported"},
			{sl::Result::eErrorFeatureMissingHooks,"eErrorFeatureMissingHooks"},
			{sl::Result::eErrorFeatureFailedToLoad,"eErrorFeatureFailedToLoad"},
			{sl::Result::eErrorFeatureWrongPriority,"eErrorFeatureWrongPriority"},
			{sl::Result::eErrorFeatureMissingDependency,"eErrorFeatureMissingDependency"},
			{sl::Result::eErrorFeatureManagerInvalidState,"eErrorFeatureManagerInvalidState"},
			{sl::Result::eErrorInvalidState,"eErrorInvalidState"},
			{sl::Result::eWarnOutOfVRAM,"eWarnOutOfVRAM"} };

	auto a = errors.find(result);
	if (a != errors.end()) {
		o_log = std::string("Error: ") + a->second;
	}	
	else
	{
		o_log = std::string("Unknown error ") + std::to_string(static_cast<int>(result));
	}

	return false;
}

bool AppBase::BeginFrame()
{
	bool turn_on;

	// STREAMLINE
	if (mParam.dlssgEnable) {
		//waitForQueue();

		//SLWrapper::Get().CleanupDLSSG(true);

		//// Get new sizes
		//DXGI_SWAP_CHAIN_DESC1 newSwapChainDesc;
		//if (SUCCEEDED(m_SwapChain_native->GetDesc1(&newSwapChainDesc))) {
		//	m_SwapChainDesc.Width = newSwapChainDesc.Width;
		//	m_SwapChainDesc.Height = newSwapChainDesc.Height;
		//	m_DeviceParams.backBufferWidth = newSwapChainDesc.Width;
		//	m_DeviceParams.backBufferHeight = newSwapChainDesc.Height;
		//}

		//BackBufferResizing();

		//// Delete swapchain and resources
		//m_SwapChain->SetFullscreenState(false, nullptr);
		//ReleaseRenderTargets();

		//m_SwapChain = nullptr;
		//m_SwapChain_native = nullptr;

		//// If we turn off dlssg, then unload dlssg featuree
		//if (turn_on)
		//	SLWrapper::Get().FeatureLoad(sl::kFeatureDLSS_G, true);
		//else {
		//	SLWrapper::Get().FeatureLoad(sl::kFeatureDLSS_G, false);
		//}

		//m_UseProxySwapchain = turn_on;

		//// Recreate Swapchain and resources 
		//RefCountPtr<IDXGISwapChain1> pSwapChain1_base;
		//auto hr = m_DxgiFactory2->CreateSwapChainForHwnd(m_GraphicsQueue, m_hWnd, &m_SwapChainDesc, &m_FullScreenDesc, nullptr, &pSwapChain1_base);
		//if (hr != S_OK)  donut::log::fatal("CreateSwapChainForHwnd failed");
		//hr = pSwapChain1_base->QueryInterface(IID_PPV_ARGS(&m_SwapChain));
		//if (hr != S_OK)  donut::log::fatal("QueryInterface failed");
		//SLWrapper::Get().ProxyToNative(m_SwapChain, (void**)&m_SwapChain_native);

		//if (!CreateRenderTargets())
		//	donut::log::fatal("CreateRenderTarget failed");

		//BackBufferResized();

		//// Reload DLSSG
		//SLWrapper::Get().FeatureLoad(sl::kFeatureDLSS_G, true);
		//SLWrapper::Get().Quiet_DLSSG_SwapChainRecreation();
	}
	else if (mParam.latewarpEnable)
	{
		//waitForQueue();

		//// Get new sizes
		//DXGI_SWAP_CHAIN_DESC1 newSwapChainDesc;
		//if (SUCCEEDED(m_SwapChain_native->GetDesc1(&newSwapChainDesc))) {
		//	m_SwapChainDesc.Width = newSwapChainDesc.Width;
		//	m_SwapChainDesc.Height = newSwapChainDesc.Height;
		//	m_DeviceParams.backBufferWidth = newSwapChainDesc.Width;
		//	m_DeviceParams.backBufferHeight = newSwapChainDesc.Height;
		//}

		//BackBufferResizing();

		//// Delete swapchain and resources
		//m_SwapChain->SetFullscreenState(false, nullptr);
		//ReleaseRenderTargets();

		//m_SwapChain = nullptr;
		//m_SwapChain_native = nullptr;

		//// If we turn off Latewarp, then unload Latewarp feature
		//if (turn_on) {
		//	SLWrapper::Get().FeatureLoad(sl::kFeatureLatewarp, true);
		//}
		//else {
		//	SLWrapper::Get().FeatureLoad(sl::kFeatureLatewarp, false);
		//}

		//m_UseProxySwapchain = turn_on;

		//// Recreate Swapchain and resources 
		//RefCountPtr<IDXGISwapChain1> pSwapChain1_base;
		//auto hr = m_DxgiFactory2->CreateSwapChainForHwnd(m_GraphicsQueue, m_hWnd, &m_SwapChainDesc, &m_FullScreenDesc, nullptr, &pSwapChain1_base);
		//if (hr != S_OK)  donut::log::fatal("CreateSwapChainForHwnd failed");
		//hr = pSwapChain1_base->QueryInterface(IID_PPV_ARGS(&m_SwapChain));
		//if (hr != S_OK)  donut::log::fatal("QueryInterface failed");
		//SLWrapper::Get().ProxyToNative(m_SwapChain, (void**)&m_SwapChain_native);

		//if (!CreateRenderTargets())
		//	donut::log::fatal("CreateRenderTarget failed");

		//BackBufferResized();

		//// Reload Latewarp
		//SLWrapper::Get().FeatureLoad(sl::kFeatureLatewarp, true);
		//SLWrapper::Get().Quiet_Latewarp_SwapChainRecreation();
	}
	else
	{
		// DXGI_SWAP_CHAIN_DESC1 newSwapChainDesc;
		// DXGI_SWAP_CHAIN_FULLSCREEN_DESC newFullScreenDesc;
		// if (SUCCEEDED(mSwapChain->GetDesc1(&newSwapChainDesc)) && SUCCEEDED(mSwapChain->GetFullscreenDesc(&newFullScreenDesc)))
		// {
		// 	if (m_FullScreenDesc.Windowed != newFullScreenDesc.Windowed)
		// 	{
		// 		waitForQueue();
		// 
		// 		BackBufferResizing();
		// 
		// 		m_FullScreenDesc = newFullScreenDesc;
		// 		m_SwapChainDesc = newSwapChainDesc;
		// 		m_DeviceParams.backBufferWidth = newSwapChainDesc.Width;
		// 		m_DeviceParams.backBufferHeight = newSwapChainDesc.Height;
		// 
		// 		if (newFullScreenDesc.Windowed)
		// 			glfwSetWindowMonitor(m_Window, nullptr, 50, 50, newSwapChainDesc.Width, newSwapChainDesc.Height, 0);
		// 
		// 		ResizeSwapChain();
		// 		BackBufferResized();
		// 	}
		// }
	}

	return true;
}

void AppBase::SLFrameInit()
{
	// ID3D12Resource& framebuffer = mFrameResources;
	// ID3D12Resource& framebuffer = mSwapChainBuffer[mCurrBackBuffer];
	
	uint32_t backbufferWidth = mSwapChainBuffer[mCurrBackBuffer]->GetDesc().Width;
	uint32_t backbufferHeight = mSwapChainBuffer[mCurrBackBuffer]->GetDesc().Height;
	// Initialize
	bool needNewPasses = false;

	sl::Extent nullExtent{};
	bool validViewportExtent = (m_backbufferViewportExtent != nullExtent);
	if (validViewportExtent)
	{
		m_DisplaySize = donut::math::int2(m_backbufferViewportExtent.width, m_backbufferViewportExtent.height);
	}
	else
	{
		m_DisplaySize = donut::math::int2(backbufferWidth, backbufferHeight);
	}

	float lodBias = 0.f;

	// RESIZE (from ui)
	if (m_ui.Resolution_changed) {
		m_ui.Resolution.x = mParam.backBufferWidth;
		m_ui.Resolution.y = mParam.backBufferHeight;
		m_ui.Resolution_changed = false;
	}
	else
	{
		m_ui.Resolution.x = backbufferWidth;
		m_ui.Resolution.y = backbufferHeight;
	}

	// DeepDVC VRAM Usage
	QueryDeepDVCState(m_ui.DeepDVC_VRAM);

#pragma region DLSSG
	// DLSS-G Setup
	if (m_dlssg_available)
	{
		bool& prevDlssgWanted = m_dlssg_shoudLoad;
		bool dlssgWanted = (m_ui.DLSSG_mode != sl::DLSSGMode::eOff);

		// If there is a change, trigger a swapchain recreation
		if (prevDlssgWanted != dlssgWanted)
		{
			m_dlssg_triggerswapchainRecreation = true;
			m_dlssg_shoudLoad = dlssgWanted;
		}

		// This is where DLSS-G is toggled On and Off (using dlssgConst.mode) and where we set DLSS-G parameters.  
		auto dlssgConst = sl::DLSSGOptions{};
		dlssgConst.mode = m_ui.DLSSG_mode;
		dlssgConst.numFramesToGenerate = m_ui.DLSSG_numFrames - 1; // ui is multiplier (e.g. 2x), subtract 1 to count generated frames only

		// Explicitly manage DLSS-G resources in order to prevent stutter when
		// temporarily disabled.
		dlssgConst.flags |= sl::DLSSGFlags::eRetainResourcesWhenOff;

		// Turn off DLSSG if we are changing the UI
		if (m_ui.MouseOverUI) {
			dlssgConst.mode = sl::DLSSGMode::eOff;
		}
		if (m_ui.DLSS_Resolution_Mode == UIData::RenderingResolutionMode::DYNAMIC)
		{
			dlssgConst.flags |= sl::DLSSGFlags::eDynamicResolutionEnabled;
			dlssgConst.dynamicResWidth = m_DisplaySize.x / 2;
			dlssgConst.dynamicResHeight = m_DisplaySize.y / 2;
		}

		// This is where we query DLSS-G minimum swapchain size
		uint64_t estimatedVramUsage;
		sl::DLSSGStatus status;
		int fps_multiplier;
		int minSize = 0;
		int numFramesMaxMultiplier = 0;
		void* pDLSSGInputsProcessingFence{};
		uint64_t lastPresentDLSSGInputsProcessingFenceValue{};
		auto lastDLSSGFenceValue = m_dlssg_settings.lastPresentInputsProcessingCompletionFenceValue;
		QueryDLSSGState(estimatedVramUsage, fps_multiplier, status, minSize, numFramesMaxMultiplier, pDLSSGInputsProcessingFence, lastPresentDLSSGInputsProcessingFenceValue);
		m_ui.DLSSG_numFramesMaxMultiplier = (numFramesMaxMultiplier + 1);
		
		if (static_cast<int>(mSwapChainBuffer[mCurrBackBuffer]->GetDesc().Width) < minSize ||
			static_cast<int>(mSwapChainBuffer[mCurrBackBuffer]->GetDesc().Height) < minSize) {
			SL_LOG_INFO("Swapchain is too small. DLSSG is disabled.");
			dlssgConst.mode = sl::DLSSGMode::eOff;
		}

		auto dlssgEnabledLastFrame = GetDLSSGLastEnable();
		SetDLSSGOptions(dlssgConst);

		auto fenceValue = lastPresentDLSSGInputsProcessingFenceValue;
		// This is where we query DLSS-G FPS, estimated VRAM usage and status
		QueryDLSSGState(estimatedVramUsage, fps_multiplier, status, minSize, numFramesMaxMultiplier, pDLSSGInputsProcessingFence, lastPresentDLSSGInputsProcessingFenceValue);
		assert(fenceValue == lastPresentDLSSGInputsProcessingFenceValue);

		if (pDLSSGInputsProcessingFence != nullptr)
		{
			if (dlssgEnabledLastFrame)
			{
				if (lastPresentDLSSGInputsProcessingFenceValue == 0 || lastPresentDLSSGInputsProcessingFenceValue > lastDLSSGFenceValue)
				{
					// This wait is redundant until SL DLSS FG allows SMSCG but done for now for demonstration purposes.
					// It needs to be queued before any of the inputs are modified in the subsequent command list submission.
					// SLWrapper::Get().QueueGPUWaitOnSyncObjectSet(GetDevice(), nvrhi::CommandQueue::Graphics, pDLSSGInputsProcessingFence, lastPresentDLSSGInputsProcessingFenceValue);
					if (mDevice != nullptr)
						mCommandQueue->Wait(reinterpret_cast<ID3D12Fence*>(pDLSSGInputsProcessingFence), lastPresentDLSSGInputsProcessingFenceValue);
				}
			}
			else
			{
				if (lastPresentDLSSGInputsProcessingFenceValue < lastDLSSGFenceValue)
				{
					assert(false);
					SL_LOG_ERROR("Inputs synchronization fence value retrieved from DLSSGState object out of order: \
                    current frame: %ld, last frame: %ld ", lastPresentDLSSGInputsProcessingFenceValue, lastDLSSGFenceValue);
				}
				else if (lastPresentDLSSGInputsProcessingFenceValue != 0)
				{
					SL_LOG_INFO("DLSSG was inactive in the last preseting frame!");
				}
			}
		}

		m_ui.DLSSG_fps = static_cast<float>(fps_multiplier * 1.0 / mSpFrame);

		if (status != sl::DLSSGStatus::eOk) {
			if (status == sl::DLSSGStatus::eFailResolutionTooLow)
				m_ui.DLSSG_status = "Resolution Too Low";
			else if (status == sl::DLSSGStatus::eFailReflexNotDetectedAtRuntime)
				m_ui.DLSSG_status = "Reflex Not Detected";
			else if (status == sl::DLSSGStatus::eFailHDRFormatNotSupported)
				m_ui.DLSSG_status = "HDR Format Not Supported";
			else if (status == sl::DLSSGStatus::eFailCommonConstantsInvalid)
				m_ui.DLSSG_status = "Common Constants Invalid";
			else if (status == sl::DLSSGStatus::eFailGetCurrentBackBufferIndexNotCalled)
				m_ui.DLSSG_status = "Common Constants Invalid";
			SL_LOG_WARN("Encountered DLSSG State Error: ", m_ui.DLSSG_status.c_str());
		}
		else {
			m_ui.DLSSG_status = "";
		}
	}

	// After we've actually set DLSS-G on/off, free resources
	if (m_ui.DLSSG_cleanup_needed)
	{
		CleanupDLSSG(false);
		m_ui.DLSSG_cleanup_needed = false;
	}
#pragma endregion DLSSG

#pragma region Latewarp
	// Latewarp
	bool prevLatewarpWanted;
	Get_Latewarp_SwapChainRecreation(prevLatewarpWanted);
	// If there is a change, trigger a swapchain recreation
	if (prevLatewarpWanted != !!m_ui.Latewarp_active)
	{
		Set_Latewarp_SwapChainRecreation(m_ui.Latewarp_active);
	}
#pragma endregion Latewarp

#pragma region REFLEX
	// REFLEX Setup
	auto reflexConst = sl::ReflexOptions{};
	reflexConst.mode = (sl::ReflexMode)m_ui.REFLEX_Mode;
	reflexConst.useMarkersToOptimize = true;
	reflexConst.virtualKey = VK_F13;
	reflexConst.frameLimitUs = m_ui.REFLEX_CapedFPS == 0 ? 0 : int(1000000. / m_ui.REFLEX_CapedFPS);
	SetReflexConsts(reflexConst);

	bool flashIndicatorDriverAvailable;
	QueryReflexStats(m_ui.REFLEX_LowLatencyAvailable, flashIndicatorDriverAvailable, m_ui.REFLEX_Stats);
	SetReflexFlashIndicator(flashIndicatorDriverAvailable);
#pragma endregion REFLEX
#pragma region DLSS
	// DLSS SETUP

	//Make sure DLSS is available
	if (m_ui.AAMode == UIData::AntiAliasingMode::DLSS && !GetDLSSAvailable())
	{
		SL_LOG_WARN("DLSS antialiasing is not available. Switching to TAA. ");
		m_ui.AAMode = UIData::AntiAliasingMode::TEMPORAL;
	}

	// Reset DLSS vars if we stop using it
	if (m_ui.DLSS_Last_AA == UIData::AntiAliasingMode::DLSS && m_ui.AAMode != UIData::AntiAliasingMode::DLSS) {
		DLSS_Last_Mode = sl::DLSSMode::eOff;
		m_ui.DLSS_Mode = sl::DLSSMode::eOff;
		m_DLSS_Last_DisplaySize = { 0,0 };
		CleanupDLSS(true); // We can also expressly tell SL to cleanup DLSS resources.
	}
	// If we turn on DLSS then we set its default values
	else if (m_ui.DLSS_Last_AA != UIData::AntiAliasingMode::DLSS && m_ui.AAMode == UIData::AntiAliasingMode::DLSS) {
		DLSS_Last_Mode = sl::DLSSMode::eBalanced;
		m_ui.DLSS_Mode = sl::DLSSMode::eBalanced;
		m_DLSS_Last_DisplaySize = { 0,0 };
	}
	m_ui.DLSS_Last_AA = m_ui.AAMode;

	// If we are using DLSS set its constants
	if ((m_ui.AAMode == UIData::AntiAliasingMode::DLSS && m_ui.DLSS_Mode != sl::DLSSMode::eOff))
	{
		sl::DLSSOptions dlssConstants = {};
		dlssConstants.mode = m_ui.DLSS_Mode;
		dlssConstants.outputWidth = m_DisplaySize.x;
		dlssConstants.outputHeight = m_DisplaySize.y;
		dlssConstants.colorBuffersHDR = sl::Boolean::eTrue;
		dlssConstants.sharpness = m_RecommendedDLSSSettings.sharpness;

		if (m_ui.DLSSPresetsAnyNonDefault())
		{
			dlssConstants.dlaaPreset = m_ui.DLSS_presets[static_cast<int>(sl::DLSSMode::eDLAA)];
			dlssConstants.qualityPreset = m_ui.DLSS_presets[static_cast<int>(sl::DLSSMode::eMaxQuality)];
			dlssConstants.balancedPreset = m_ui.DLSS_presets[static_cast<int>(sl::DLSSMode::eBalanced)];
			dlssConstants.performancePreset = m_ui.DLSS_presets[static_cast<int>(sl::DLSSMode::eMaxPerformance)];
			dlssConstants.ultraPerformancePreset = m_ui.DLSS_presets[static_cast<int>(sl::DLSSMode::eUltraPerformance)];
		}

		dlssConstants.useAutoExposure = sl::Boolean::eFalse;

		// Changing presets requires a restart of DLSS
		if (m_ui.DLSSPresetsChanged())
			CleanupDLSS(true);

		m_ui.DLSSPresetsUpdate();

		SetDLSSOptions(dlssConstants);

		// Check if we need to update the rendertarget size.
		bool DLSS_resizeRequired = (m_ui.DLSS_Mode != DLSS_Last_Mode) || (m_DisplaySize.x != m_DLSS_Last_DisplaySize.x) || (m_DisplaySize.y != m_DLSS_Last_DisplaySize.y);
		if (DLSS_resizeRequired) {
			// Only quality, target width and height matter here
			QueryDLSSOptimalSettings(m_RecommendedDLSSSettings);

			if (m_RecommendedDLSSSettings.optimalRenderSize.x <= 0 || m_RecommendedDLSSSettings.optimalRenderSize.y <= 0) {
				m_ui.AAMode = UIData::AntiAliasingMode::NONE;
				m_ui.DLSS_Mode = sl::DLSSMode::eBalanced;
				m_RenderingRectSize = m_DisplaySize;
			}
			else {
				DLSS_Last_Mode = m_ui.DLSS_Mode;
				m_DLSS_Last_DisplaySize = m_DisplaySize;
			}
		}

		// in variable ratio mode, pick a random ratio between min and max rendering resolution
		donut::math::int2 maxSize = m_RecommendedDLSSSettings.maxRenderSize;
		donut::math::int2 minSize = m_RecommendedDLSSSettings.minRenderSize;
		float texLodXDimension;
		if (m_ui.DLSS_Resolution_Mode == UIData::RenderingResolutionMode::DYNAMIC)
		{
			// Even if we request dynamic res, it is possible that the DLSS mode has max==min
			if (any(maxSize != minSize))
			{
				if (m_ui.DLSS_Dynamic_Res_change)
				{
					m_ui.DLSS_Dynamic_Res_change = false;
					std::uniform_int_distribution<int> distributionWidth(minSize.x, maxSize.x);
					int newWidth = distributionWidth(m_Generator);

					// Height is initially based on width and aspect
					int newHeight = (int)(newWidth * (float)m_DisplaySize.y / (float)m_DisplaySize.x);

					// But that height might be too small or too large for the min/max settings of the DLSS
					// mode (in theory); skip changing the res if it is out of range.
					// We predict this never to happen. It is more of a safety measure.
					if (newHeight >= minSize.y && newHeight <= maxSize.y) m_RenderingRectSize = { newWidth , newHeight };
				}

				// For dynamic ratio, we want to choose the minimum rendering size
				// to select a Texture LOD that will preserve its sharpness over a large range of rendering resolution.
				// Ideally, the texture LOD would be allowed to be variable as well based on the dynamic scale
				// but we don't support that here yet.
				texLodXDimension = (float)minSize.x;

				// If the OUTPUT buffer resized or the DLSS mode changed, we need to recreate passes in dynamic mode.
				// In fixed resolution DLSS, this just happens when we change DLSS mode because it causes one of the
				// other cases below to hit (likely texLod).
				if (DLSS_resizeRequired) needNewPasses = true;
			}
			else
			{
				m_RenderingRectSize = maxSize;
				texLodXDimension = (float)m_RenderingRectSize.x;
			}
		}
		else if (m_ui.AAMode == UIData::AntiAliasingMode::DLSS)
		{
			m_RenderingRectSize = m_RecommendedDLSSSettings.optimalRenderSize;
			texLodXDimension = (float)m_RenderingRectSize.x;
		}

		// Use the formula of the DLSS programming guide for the Texture LOD Bias...
		lodBias = std::log2f(texLodXDimension / m_DisplaySize.x) - 1;
	}
	else {
		sl::DLSSOptions dlssConstants = {};
		dlssConstants.mode = sl::DLSSMode::eOff;
		SetDLSSOptions(dlssConstants);
		m_RenderingRectSize = m_DisplaySize;
	}
#pragma endregion DLSS
	// PASS SETUP
	{
		bool needNewPasses = false;

		// Here, we intentionally leave the renderTargets oversized: (displaySize, displaySize) instead of (m_RenderingRectSize, displaySize), to show the power of sl::Extent
		bool useFullSizeRenderingBuffers = m_ui.DLSS_always_use_extents || (m_ui.DLSS_Resolution_Mode == UIData::RenderingResolutionMode::DYNAMIC);

		donut::math::int2 renderSize = useFullSizeRenderingBuffers ? m_DisplaySize : m_RenderingRectSize;	// 이 구간을 통해 실제 렌더링 사이즈와 디스플레이 사이즈 중 한가지를 선택하게 만든다. (즉, 메모리 초기화 구문을 해당 위치로 수정하거나, 그외 수단을 통해 제어 조작이 필요)

		if (IsUpdateRequired(renderSize, m_DisplaySize))
		{
			// TODO: Check This Point!
			// m_BindingCache.Clear();
			// 
			// m_RenderTargets = nullptr;
			// m_RenderTargets = std::make_unique<RenderTargets>();
			// m_RenderTargets->Init(GetDevice(), renderSize, m_DisplaySize, framebuffer->getDesc().colorAttachments[0].texture->getDesc().format);
			// 
			needNewPasses = true;
		}

		// Render scene, change bias
		if (m_ui.DLSS_lodbias_useoveride) lodBias = m_ui.DLSS_lodbias_overide;
		if (m_PreviousLodBias != lodBias)
		{
			needNewPasses = true;
			m_PreviousLodBias = lodBias;
		}

		if (SetupView())
		{
			needNewPasses = true;
		}

		if (needNewPasses)
		{
			// CreateRenderPasses(exposureResetRequired, lodBias);
		}

	}

}

void AppBase::SLFrameSetting()
{
	if (m_dlss_available)
	{
		//! We can also add all tags here
		//!
		//! NOTE: These are considered local tags and will NOT impact any tags set in global scope.
		// const sl::BaseStructure* inputs[] = { &myViewport, &depthTag, &mvecTag, &colorInTag, &colorOutTag };
		
		//! Evaluates feature
		//! 
		//! Use this method to mark the section in your rendering pipeline
		//! where specific feature should be injected.
		//!
		//! @param feature Feature we are working with
		//! @param frame Current frame handle obtained from SL
		//! @param inputs The chained structures providing the input data (viewport, tags, constants etc)
		//! @param numInputs Number of inputs
		//! @param cmdBuffer Command buffer to use (must be created on device where feature is supported)
		//! @return sl::ResultCode::eOk if successful, error code otherwise (see sl_result.h for details)
		//! 
		//! IMPORTANT: Frame and viewport must match whatever is used to set common and or feature options and constants (if any)
		//! 
		//! NOTE: It is allowed to pass in buffer tags as inputs, they are considered to be a "local" tags and do NOT interact with
		//! same tags sent in the global scope using slSetTag API.
		//!
		//! This method is NOT thread safe and requires DX/VK device to be created before calling it.
		// SL_API sl::Result slEvaluateFeature(sl::Feature feature, const sl::FrameToken & frame, const sl::BaseStructure * *inputs, uint32_t numInputs, sl::CommandBuffer * cmdBuffer);
		// 
		// sl::FrameToken;
		EvaluateDLSS(mCommandList.Get());
	}
	else
	{
		// Default up-scaling pass like, for example, TAAU goes here
	}

	// DLSS

	// NOTE: Showing only depth tag for simplicity
	//
	// LOCAL tagging for the evaluate call, using state which is valid now on the command list we are using to evaluate DLSS
	// sl::Resource depth = sl::Resource{ sl::ResourceType::eTex2d, myNativeObject, nullptr, nullptr, depthStateOnEval };
	// sl::ResourceTag depthTag = sl::ResourceTag{ &depth, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent, &depthExtent };
	// const sl::BaseStructure* inputs[] = { &myViewport, &depthTag };
	// if (SL_FAILED(result, slEvaluateFeature(sl::kFeatureDLSS, currentFrameToken, inputs, _countof(inputs), myCmdList)))
	// {
	// 	// Handle error, check the logs
	// }
	// else
	// {
	// 	// IMPORTANT: Host is responsible for restoring state on the command list used
	// 	restoreState(myCmdList);
	// }

	// DLSS-G

	// Now we tag depth GLOBALLY with state which is valid when frame present is called
	// sl::Resource depth = sl::Resource{ sl::ResourceType::eTex2d, myNativeObject, nullptr, nullptr, depthStateOnPresent };
	// sl::ResourceTag depthTag = sl::ResourceTag{ &depth, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent, &depthExtent };
	// if (SL_FAILED(result, slSetTagForFrame(*currentFrame, viewport, &depthTag, 1, myCmdList)))
	// {
	// 	// Handle error, check the logs
	// }


	//// 프레임 생성 추적
	//sl::FrameToken* prevFrame;
	//uint32_t prevIndex = *currentFrame - 1;
	//slGetNewFrameToken(prev, &prevIndex); // NOTE: providing optional frame index

	//sl::FrameToken* nextFrame;
	//uint32_t nextIndex = *currentFrame + 1;
	//slGetNewFrameToken(next, &nextIndex); // NOTE: providing optional frame index


	//// 기본 세팅 옵션 확인
	//// Using helpers from sl_dlss.h

	//sl::DLSSOptimalSettings dlssSettings;
	//sl::DLSSOptions dlssOptions;
	//// These are populated based on user selection in the UI
	//dlssOptions.mode = myUI->getDLSSMode(); // e.g. sl::eDLSSModeBalanced;
	//dlssOptions.outputWidth = myUI->getOutputWidth();    // e.g 1920;
	//dlssOptions.outputHeight = myUI->getOutputHeight(); // e.g. 1080;
	//// Now let's check what should our rendering resolution be
	//if (SL_FAILED(result, slDLSSGetOptimalSettings(dlssOptions, dlssSettings))
	//{
	//	// Handle error here
	//}
	//// Setup rendering based on the provided values in the sl::DLSSSettings structure
	//myViewport->setSize(dlssSettings.renderWidth, dlssSettings.renderHeight);

	//// 명시적으로 기능 로드
	//SL_API sl::Result slSetFeatureLoaded(sl::Feature feature, bool loaded);
}

bool AppBase::InitDebugLayer()
{
	// [DEBUG] Enable debug interface
#if defined(DEBUG) || defined(_DEBUG) 
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&mD12Debug)));
	SL_LOG_INFO("*Info, EnableDebugLayer");
	mD12Debug->EnableDebugLayer();
	// Init DXGI Debug
	ThrowIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&mDxgiDebug)));
	SL_LOG_INFO("*Info, EnableLeakTrackingForThread");
	mDxgiDebug->EnableLeakTrackingForThread();
#endif

	return true;
}

bool AppBase::ShutdownDebugLayer()
{
#if defined(DEBUG) || defined(_DEBUG) 
	if (mDxgiDebug)
	{
		OutputDebugStringW(L"*Info, DXGI Reports living device objects:\n");
		mDxgiDebug->ReportLiveObjects(
			DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL
		);
	}
	if (mD12Debug)
	{

	}
	if(mDxgiDebug)
		mDxgiDebug->Release();
	if(mD12Debug)
		mD12Debug->Release();
#endif
	return true;
}


void AppBase::SetSLConsts(const sl::Constants& consts)
{
	if (!mSLInitialised) {
		SL_LOG_WARN("SL not initialised.");
		return;
	}


	if (!SuccessCheck(slSetConstants(consts, *m_currentFrame, m_viewport), str_temp))
		SL_LOG_WARN(str_temp.c_str());
}

void AppBase::FeatureLoad(sl::Feature feature, const bool turn_on)
{
	if (mPref.renderAPI == sl::RenderAPI::eD3D12) {
		bool loaded;
		slIsFeatureLoaded(feature, loaded);
		if (loaded && !turn_on) {
			slSetFeatureLoaded(feature, turn_on);
		}
		else if (!loaded && turn_on) {
			slSetFeatureLoaded(feature, turn_on);
		}
	}
}

void AppBase::TagResources_General(ID3D12GraphicsCommandList* commandList, ID3D12Resource* motionVectors, ID3D12Resource* depth, ID3D12Resource* finalColorHudless)
{
	if (!mSLInitialised) {
		SL_LOG_WARN("Streamline not initialised.");
		return;
	}
	sl::Extent renderExtent{ 0, 0, depth->GetDesc().Width, depth->GetDesc().Height };
	sl::Extent fullExtent{ 0, 0, finalColorHudless->GetDesc().Width, finalColorHudless->GetDesc().Height };
	void* cmdbuffer = commandList;
	sl::Resource motionVectorsResource{}, depthResource{}, finalColorHudlessResource{};
	GetSLResource(commandList, motionVectorsResource, motionVectors);
	GetSLResource(commandList, depthResource, depth);
	GetSLResource(commandList, finalColorHudlessResource, finalColorHudless);

	sl::ResourceTag motionVectorsResourceTag = sl::ResourceTag{ &motionVectorsResource, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent };
	sl::ResourceTag depthResourceTag = sl::ResourceTag{ &depthResource, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent };
	sl::ResourceTag finalColorHudlessResourceTag = sl::ResourceTag{ &finalColorHudlessResource, sl::kBufferTypeHUDLessColor, sl::ResourceLifecycle::eValidUntilPresent, &fullExtent };
	sl::ResourceTag inputs[] = { motionVectorsResourceTag, depthResourceTag, finalColorHudlessResourceTag };

	if(!SuccessCheck(slSetTag(m_viewport, inputs, _countof(inputs), cmdbuffer), str_temp))
		SL_LOG_WARN(str_temp.c_str());
}

void AppBase::TagResources_DLSS_NIS(ID3D12GraphicsCommandList* commandList, ID3D12Resource* output, ID3D12Resource* input)
{
	if (!mSLInitialised) {
		SL_LOG_WARN("Streamline not initialised.");
		return;
	}

	sl::Extent renderExtent{ 0, 0, input->GetDesc().Width, input->GetDesc().Height };
	sl::Extent fullExtent{ 0, 0, output->GetDesc().Width, output->GetDesc().Height };
	void* cmdbuffer = commandList;
	sl::Resource outputResource{}, inputResource{};

	GetSLResource(commandList, outputResource, output);
	GetSLResource(commandList, inputResource, input);

	sl::ResourceTag inputResourceTag = sl::ResourceTag{ &inputResource, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent };
	sl::ResourceTag outputResourceTag = sl::ResourceTag{ &outputResource, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilPresent, &fullExtent };

	sl::ResourceTag inputs[] = { inputResourceTag, outputResourceTag };
	if (!SuccessCheck(slSetTag(m_viewport, inputs, _countof(inputs), cmdbuffer), str_temp))
		SL_LOG_WARN(str_temp.c_str());
}

//void AppBase::TagResources_General(nvrhi::ICommandList* commandList, const donut::engine::IView* view, nvrhi::ITexture* motionVectors, nvrhi::ITexture* depth, nvrhi::ITexture* finalColorHudless)
//{
//	if (!mSLInitialised) {
//		SL_LOG_WARN("Streamline not initialised.");
//		return;
//	}
//
//	sl::Extent renderExtent{ 0, 0, depth->getDesc().width, depth->getDesc().height };
//	sl::Extent fullExtent{ 0, 0, finalColorHudless->getDesc().width, finalColorHudless->getDesc().height };
//	void* cmdbuffer = mCommandList.Get();
//	sl::Resource motionVectorsResource{}, depthResource{}, finalColorHudlessResource{};
//
//	motionVectorsResource = 
//	GetSLResource(commandList, motionVectorsResource, motionVectors, view);
//	GetSLResource(commandList, depthResource, depth, view);
//	GetSLResource(commandList, finalColorHudlessResource, finalColorHudless, view);
//
//	sl::ResourceTag motionVectorsResourceTag = sl::ResourceTag{ &motionVectorsResource, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent };
//	sl::ResourceTag depthResourceTag = sl::ResourceTag{ &depthResource, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent };
//	sl::ResourceTag finalColorHudlessResourceTag = sl::ResourceTag{ &finalColorHudlessResource, sl::kBufferTypeHUDLessColor, sl::ResourceLifecycle::eValidUntilPresent, &fullExtent };
//
//	sl::ResourceTag inputs[] = { motionVectorsResourceTag, depthResourceTag, finalColorHudlessResourceTag };
//	successCheck(slSetTag(m_viewport, inputs, _countof(inputs), cmdbuffer), "slSetTag_General");
//}
//
//void AppBase::TagResources_DLSS_NIS(nvrhi::ICommandList& commandList, const donut::engine::IView* view, Texture& output, Texture& input)
//{
//	if (!mSLInitialised) {
//		SL_LOG_WARN("Streamline not initialised.");
//		return;
//	}
//
//	sl::Extent renderExtent{ 0, 0, Input->getDesc().width, Input->getDesc().height };
//	sl::Extent fullExtent{ 0, 0, Output->getDesc().width, Output->getDesc().height };
//	void* cmdbuffer = GetNativeCommandList(commandList);
//	sl::Resource inputResource{};
//
//	// GetSLResource(commandList, outputResource, Output, view);
//	// GetSLResource(commandList, inputResource, Input, view);
//	// sl::Resource outputResource{
//	// 	/* ResourceType	*/ .type              = sl::ResourceType::eTex2d		, //! Indicates the type of resource
//	// 	/* void*		*/ .native            = output.Resource					, //! ID3D11Resource/ID3D12Resource/VkBuffer/VkImage
//	// 	/* void*		*/ .memory            =	nullptr							, //! vkDeviceMemory or nullptr
//	// 	/* void*		*/ .view              = nullptr							, //! VkImageView/VkBufferView or nullptr
//	// 	/* uint32_t		*/ .state             = D3D12_RESOURCE_STATE_COMMON		, //! State as D3D12_RESOURCE_STATES or VkImageLayout		//! IMPORTANT: State is MANDATORY and needs to be correct when tagged resources are actually used.
//	// 	/* uint32_t		*/ .width             =	0								, //! Width in pixels
//	// 	/* uint32_t		*/ .height            =	0								, //! Height in pixels
//	// 	/* uint32_t		*/ .nativeFormat      =	0								, //! Native format
//	// 	/* uint32_t		*/ .mipLevels         =	0								, //! Number of mip-map levels
//	// 	/* uint32_t		*/ .arrayLayers       =	0								, //! Number of arrays
//	// 	/* uint64_t		*/ .gpuVirtualAddress =	0								, //! Virtual address on GPU (if applicable)
//	// 	/* uint32_t		*/ .flags             =									, //! VkImageCreateFlags
//	// 	/* uint32_t		*/ .usage             =	0								, //! VkImageUsageFlags
//	// 	/* uint32_t		*/ .reserved          =	0								 //! Reserved for internal use
//	// };
//	sl::Resource outputResource{
//		/* ResourceType	.type              */ sl::ResourceType::eTex2d		, //! Indicates the type of resource
//		/* void*		.native            */ output.Resource					, //! ID3D11Resource/ID3D12Resource/VkBuffer/VkImage
//		/* void*		.memory            */ nullptr							, //! vkDeviceMemory or nullptr
//		/* void*		.view              */ nullptr							, //! VkImageView/VkBufferView or nullptr
//		/* uint32_t		.state             */ D3D12_RESOURCE_STATE_COMMON		 //! State as D3D12_RESOURCE_STATES or VkImageLayout		//! IMPORTANT: State is MANDATORY and needs to be correct when tagged resources are actually used.
//		// /* uint32_t		*/ .width             =	0								, //! Width in pixels
//		// /* uint32_t		*/ .height            =	0								, //! Height in pixels
//		// /* uint32_t		*/ .nativeFormat      =	0								, //! Native format
//		// /* uint32_t		*/ .mipLevels         =	0								, //! Number of mip-map levels
//		// /* uint32_t		*/ .arrayLayers       =	0								, //! Number of arrays
//		// /* uint64_t		*/ .gpuVirtualAddress =	0								, //! Virtual address on GPU (if applicable)
//		// /* uint32_t		*/ .flags             =									, //! VkImageCreateFlags
//		// /* uint32_t		*/ .usage             =	0								, //! VkImageUsageFlags
//		// /* uint32_t		*/ .reserved          =	0								 //! Reserved for internal use
//	}
//
//
//	sl::ResourceTag inputResourceTag = sl::ResourceTag{ &inputResource, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent };
//	sl::ResourceTag outputResourceTag = sl::ResourceTag{ &outputResource, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilPresent, &fullExtent };
//
//	sl::ResourceTag inputs[] = { inputResourceTag, outputResourceTag };
//	successCheck(slSetTag(m_viewport, inputs, _countof(inputs), cmdbuffer), "slSetTag_dlss_nis");
//}


void AppBase::SetDLSSOptions(const sl::DLSSOptions consts)
{
	if (!mSLInitialised || !m_dlss_available) {
		SL_LOG_WARN("SL not initialised or DLSS not available.");
		return;
	}

	m_dlss_consts = consts;
	
	if(!SuccessCheck(slDLSSSetOptions(m_viewport, m_dlss_consts), str_temp))
		SL_LOG_WARN(str_temp.c_str());
	if(!SuccessCheck(slDLSSGetOptimalSettings(m_dlss_consts, m_dlss_settings), str_temp))
		SL_LOG_WARN(str_temp.c_str());

	// if (!SuccessCheck(slAllocateResources(sl::kFeatureDLSS, m_viewport), str_temp))
	// 	SL_LOG_WARN(str_temp.c_str());
	
}

void AppBase::QueryDLSSOptimalSettings(DLSSSettings& settings)
{
}

void AppBase::EvaluateDLSS(ID3D12CommandList* commandList)
{
	void* nativeCommandList = nullptr;

	if (mPref.renderAPI == sl::RenderAPI::eD3D12)
		nativeCommandList = commandList;

	if (nativeCommandList == nullptr) {
		SL_LOG_WARN("Failed to retrieve context for DLSS evaluation.");
		return;
	}

	sl::ViewportHandle view(m_viewport);
	const sl::BaseStructure* inputs[] = { &view };
	if(!SuccessCheck(slEvaluateFeature(sl::kFeatureDLSS, *m_currentFrame, inputs, _countof(inputs), nativeCommandList), str_temp))
		SL_LOG_WARN(str_temp.c_str());

	//Our pipeline is very simple so we can simply clear it, but normally state tracking should be implemented.
	// commandList->clearState();
}

void AppBase::CleanupDLSS(bool wfi)
{
	if (!mSLInitialised) {
		SL_LOG_WARN("SL not initialised.");
		return;
	}
	if (!m_dlss_available)
	{
		return;
	}

	if (wfi) {
		// mDevice->waitForIdle();
		FlushCommandQueue();
	}

	sl::Result status = slFreeResources(sl::kFeatureDLSS, m_viewport);
	// if we've never ran the feature on this viewport, this call may return 'eErrorInvalidParameter'
	assert(status == sl::Result::eOk || status == sl::Result::eErrorInvalidParameter);
}

void AppBase::SetNISOptions(const sl::NISOptions consts)
{
}

void AppBase::EvaluateNIS(ID3D12CommandList* commandList)
{
}

void AppBase::CleanupNIS(bool wfi)
{
}

void AppBase::SetDeepDVCOptions(const sl::DeepDVCOptions consts)
{
}
void AppBase::QueryDeepDVCState(uint64_t& estimatedVRamUsage)
{
	if (!mSLInitialised || !m_deepdvc_available) {
		SL_LOG_WARN("SL not initialised or DeepDVC not available.");
		return;
	}
	sl::DeepDVCState state;
	if (!SuccessCheck(slDeepDVCGetState(m_viewport, state), str_temp))
		SL_LOG_WARN(str_temp.c_str());
	
	estimatedVRamUsage = state.estimatedVRAMUsageInBytes;
}

void AppBase::EvaluateDeepDVC(ID3D12CommandList* commandList)
{
}

void AppBase::CleanupDeepDVC()
{
}

void AppBase::Callback_FrameCount_Reflex_Sleep_Input_SimStart(AppBase& manager)
{
}

void AppBase::ReflexTriggerFlash()
{
}

void AppBase::ReflexTriggerPcPing()
{
}

void AppBase::QueryReflexStats(bool& reflex_lowLatencyAvailable, bool& reflex_flashAvailable, std::string& stats)
{
	if (m_reflex_available) {
		sl::ReflexState state;
		if(!SuccessCheck(slReflexGetState(state), str_temp))
			SL_LOG_WARN(str_temp.c_str());

		reflex_lowLatencyAvailable = state.lowLatencyAvailable;
		reflex_flashAvailable = state.flashIndicatorDriverControlled;

		auto rep = state.frameReport[63];
		if (state.latencyReportAvailable && rep.gpuRenderEndTime != 0) {

			auto frameID = rep.frameID;
			auto totalGameToRenderLatencyUs = rep.gpuRenderEndTime - rep.inputSampleTime;
			auto simDeltaUs = rep.simEndTime - rep.simStartTime;
			auto renderDeltaUs = rep.renderSubmitEndTime - rep.renderSubmitStartTime;
			auto presentDeltaUs = rep.presentEndTime - rep.presentStartTime;
			auto driverDeltaUs = rep.driverEndTime - rep.driverStartTime;
			auto osRenderQueueDeltaUs = rep.osRenderQueueEndTime - rep.osRenderQueueStartTime;
			auto gpuRenderDeltaUs = rep.gpuRenderEndTime - rep.gpuRenderStartTime;

			stats = "frameID: " + std::to_string(frameID);
			stats += "\ntotalGameToRenderLatencyUs: " + std::to_string(totalGameToRenderLatencyUs);
			stats += "\nsimDeltaUs: " + std::to_string(simDeltaUs);
			stats += "\nrenderDeltaUs: " + std::to_string(renderDeltaUs);
			stats += "\npresentDeltaUs: " + std::to_string(presentDeltaUs);
			stats += "\ndriverDeltaUs: " + std::to_string(driverDeltaUs);
			stats += "\nosRenderQueueDeltaUs: " + std::to_string(osRenderQueueDeltaUs);
			stats += "\ngpuRenderDeltaUs: " + std::to_string(gpuRenderDeltaUs);
		}
		else {
			stats = "Latency Report Unavailable";
		}
	}
}

void AppBase::SetReflexConsts(const sl::ReflexOptions options)
{
	if (!mSLInitialised || !m_reflex_available)
	{
		SL_LOG_WARN("SL not initialised or Reflex not available.");
		return;
	}

	m_reflex_consts = options;
	if(!SuccessCheck(slReflexSetOptions(m_reflex_consts), str_temp))
		SL_LOG_WARN(str_temp.c_str());

	return;
}


void AppBase::ReflexCallback_Sleep(AppBase& manager, uint32_t frameID)
{
	if (m_reflex_available)
	{
		if(!SuccessCheck(slGetNewFrameToken(m_currentFrame, &frameID), str_temp))
			SL_LOG_WARN(str_temp.c_str());
		if(!SuccessCheck(slReflexSleep(*m_currentFrame), str_temp))
			SL_LOG_WARN(str_temp.c_str());
	}
}

void AppBase::ReflexCallback_SimStart(AppBase& manager, uint32_t frameID)
{
	if (m_pcl_available) {
		sl::FrameToken* temp;
		if(!SuccessCheck(slGetNewFrameToken(temp, &frameID), str_temp))
			SL_LOG_WARN(str_temp.c_str());
		if(!SuccessCheck(slPCLSetMarker(sl::PCLMarker::eSimulationStart, *temp), str_temp))
			SL_LOG_WARN(str_temp.c_str());
	}
}

void AppBase::ReflexCallback_SimEnd(AppBase& manager, uint32_t frameID)
{
	if (m_pcl_available)
	{
		sl::FrameToken* temp;
		if(!SuccessCheck(slGetNewFrameToken(temp, &frameID), str_temp))
			SL_LOG_WARN(str_temp.c_str());
		if(!SuccessCheck(slPCLSetMarker(sl::PCLMarker::eSimulationEnd, *temp), str_temp))
			SL_LOG_WARN(str_temp.c_str());
	}
}

void AppBase::ReflexCallback_RenderStart(AppBase& manager, uint32_t frameID)
{
	if (m_pcl_available)
	{
		sl::FrameToken* temp;
		if(!SuccessCheck(slGetNewFrameToken(temp, &frameID), str_temp))
			SL_LOG_WARN(str_temp.c_str());
		if(!SuccessCheck(slPCLSetMarker(sl::PCLMarker::eRenderSubmitStart, *temp), str_temp))
			SL_LOG_WARN(str_temp.c_str());
	}
}

void AppBase::ReflexCallback_RenderEnd(AppBase& manager, uint32_t frameID)
{
	if (m_pcl_available)
	{
		sl::FrameToken* temp;
		if(!SuccessCheck(slGetNewFrameToken(temp, &frameID), str_temp))
			SL_LOG_WARN(str_temp.c_str());
		if (!SuccessCheck(slPCLSetMarker(sl::PCLMarker::eRenderSubmitEnd, *temp), str_temp))
			SL_LOG_WARN(str_temp.c_str());
	}
}

void AppBase::ReflexCallback_PresentStart(AppBase& manager, uint32_t frameID)
{
	if (m_pcl_available)
	{
		sl::FrameToken* temp;
		if(!SuccessCheck(slGetNewFrameToken(temp, &frameID), str_temp))
			SL_LOG_WARN(str_temp.c_str());
		if (!SuccessCheck(slPCLSetMarker(sl::PCLMarker::ePresentStart, *temp), str_temp))
			SL_LOG_WARN(str_temp.c_str());
	}
}

void AppBase::ReflexCallback_PresentEnd(AppBase& manager, uint32_t frameID)
{
	if (m_pcl_available)
	{
		sl::FrameToken* temp;
		if(!SuccessCheck(slGetNewFrameToken(temp, &frameID), str_temp))
			SL_LOG_WARN(str_temp.c_str());
		if (!SuccessCheck(slPCLSetMarker(sl::PCLMarker::ePresentEnd, *temp), str_temp))
			SL_LOG_WARN(str_temp.c_str());
	}
}

void AppBase::SetDLSSGOptions(const sl::DLSSGOptions consts)
{
	if (!mSLInitialised || !m_dlssg_available) {
		SL_LOG_WARN("SL not initialised or DLSSG not available.");
		return;
	}

	m_dlssg_consts = consts;

	if(!SuccessCheck(slDLSSGSetOptions(m_viewport, m_dlssg_consts), str_temp))
		SL_LOG_WARN(str_temp.c_str());
}

void AppBase::QueryDLSSGState(uint64_t& estimatedVRamUsage, int& fps_multiplier, sl::DLSSGStatus& status, int& minSize, int& maxFrameCount, void*& pFence, uint64_t& fenceValue)
{
	if (!mSLInitialised || !m_dlssg_available) {
		SL_LOG_WARN("SL not initialised or DLSSG not available.");
		return;
	}

	if(!SuccessCheck(slDLSSGGetState(m_viewport, m_dlssg_settings, &m_dlssg_consts), str_temp))
		SL_LOG_WARN(str_temp.c_str());

	estimatedVRamUsage = m_dlssg_settings.estimatedVRAMUsageInBytes;
	fps_multiplier = m_dlssg_settings.numFramesActuallyPresented;
	status = m_dlssg_settings.status;
	minSize = m_dlssg_settings.minWidthOrHeight;
	maxFrameCount = m_dlssg_settings.numFramesToGenerateMax;
	pFence = m_dlssg_settings.inputsProcessingCompletionFence;
	fenceValue = m_dlssg_settings.lastPresentInputsProcessingCompletionFenceValue;
}

bool AppBase::Get_DLSSG_SwapChainRecreation(bool& turn_on) const
{
	turn_on = m_dlssg_shoudLoad;
	auto tmp = m_dlssg_triggerswapchainRecreation;
	return tmp;
}

void AppBase::CleanupDLSSG(bool wfi)
{
	if (!mSLInitialised) {
		SL_LOG_WARN("SL not initialised.");
		return;
	}
	if (!m_dlssg_available)
	{
		return;
	}

	if (wfi) {
		// m_Device->waitForIdle();
		FlushCommandQueue();
	}

	sl::Result status = slFreeResources(sl::kFeatureDLSS_G, m_viewport);
	// if we've never ran the feature on this viewport, this call may return 'eErrorInvalidParameter'
	assert(status == sl::Result::eOk || status == sl::Result::eErrorInvalidParameter || status == sl::Result::eErrorFeatureMissing);
}

bool AppBase::SetupView()
{
	// if (m_TemporalAntiAliasingPass) m_TemporalAntiAliasingPass->SetJitter(m_ui.TemporalAntiAliasingJitter);

	return false;
}
