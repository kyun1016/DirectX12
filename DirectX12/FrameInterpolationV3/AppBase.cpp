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

				// New Function
				SLFrameInit();

				Update();
				Render();
				RenderImGui();

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

				// Swap the back and front buffers
				ThrowIfFailed(mSwapChain->Present(mParam.vsyncEnabled ? 1 : 0, 0));
				mCurrBackBuffer = (mCurrBackBuffer + 1) % APP_NUM_BACK_BUFFERS;

				// Add an instruction to the command queue to set a new fence point. 
				// Because we are on the GPU timeline, the new fence point won't be 
				// set until the GPU finishes processing all the commands prior to this Signal().
				mCommandQueue->Signal(mFence.Get(), mFrameCount);

				// Advance the fence value to mark commands up to this fence point.
				mCurrFrameResource->Fence = ++mFrameCount;
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
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring mspfStr = std::to_wstring(mspf);

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
				// sl::kFeatureDLSS_G,	// 현재 DLL 로드 시 오류가 발생하여 주석 처리
				sl::kFeatureReflex,
				sl::kFeatureDeepDVC,
				sl::kFeatureLatewarp,
				sl::kFeaturePCL
			};
			mPref.featuresToLoad = myFeatures;
			mPref.numFeaturesToLoad = static_cast<uint32_t>(std::size(myFeatures));
			mPref.renderAPI = sl::RenderAPI::eD3D12;
			mPref.pathToLogsAndData = L"..\\StreamlineCore\\streamline\\bin\\x64\\";

			mSLInitialised = SuccessCheck(slInit(mPref, SDK_VERSION), "slInit");
			if (!mSLInitialised) {
				SL_LOG_WARN("Failed to initailze SL");
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
		SuccessCheck(slSetD3DDevice((ID3D12Device*)mDevice.Get()), "slSetD3DDevice");
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

	return true;
}

bool AppBase::SuccessCheck(sl::Result result, const char* location)
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
		SL_LOG_WARN("%s", std::string("Error: ") + a->second + (location == nullptr ? "" : (std::string(" encountered in ") + std::string(location))));
	}	
	else
	{
		SL_LOG_WARN("%s", std::string("Unknown error ") + std::to_string(static_cast<int>(result)) + (location == nullptr ? "" : (std::string(" encountered in ") + std::string(location))));
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


