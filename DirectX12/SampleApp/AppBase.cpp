#include <iostream>
#include <dxgi1_6.h>
#include <windows.h>
#include <wrl/client.h> // ComPtr
#include <string>
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include "AppBase.h"
#include "AppBaseHelper.h"
#include "GameTimer.h"
#include "d3dx12.h"
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")


// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Singleton object so that worker threads can share members.
static AppBase* g_appBase = nullptr;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	return g_appBase->MsgProc(hWnd, msg, wParam, lParam);
}
//===================================
// Constructor
//===================================
AppBase::AppBase() : AppBase(1080, 720, L"AppBase") {}

AppBase::AppBase(uint32_t width, uint32_t height, std::wstring name) :
	mClientWidth(width),
	mClientHeight(height),
	mWindowRect({ 0,0,0,0 }),
	mTitle(name),
	mAspectRatio(0.0f)
{
	WCHAR assetsPath[512];
	GetAssetsPath(assetsPath, _countof(assetsPath));

	g_appBase = this;

	UpdateForSizeChange(width, height);
	// mCamera = make_unique<Camera>();
}

AppBase::~AppBase()
{
	g_appBase = nullptr;
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
	if (!InitMainWindow())
		return false;

	if (!InitDirect3D())
		return false;

	if (!InitImgui())
		return false;

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
		// Otherwise, do animation/game stuff.
		else if (mSwapChainOccluded && mSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
		{
			::Sleep(10);
		}
		else {
			mTimer.Tick();
			if (!mAppPaused)
			{
				mSwapChainOccluded = false;
				UpdateImGui();		// override this function

				//==========================================
				// My Render
				//==========================================
				CalculateFrameStats();
				Update(mTimer);
				Render(mTimer);

				// Rendering
				ImGui::Render();

				//==========================================
				// ImGui
				//==========================================
				// Update and Render additional Platform Windows
				if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
				{
					ImGui::UpdatePlatformWindows();
					ImGui::RenderPlatformWindowsDefault();
				}

				// GUI 렌더링
				// ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());
				/*D3D12_RESOURCE_BARRIER RenderBarrier = CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
				mCommandList->ResourceBarrier(1, &RenderBarrier);*/


				// swap the back and front buffers
				ThrowIfFailed(mSwapChain->Present(1, 0)); // Present with vsync
				// ThrowIfFailed(mSwapChain->Present(0, 0)); // Present without vsync
				mCurrBackBuffer = (mCurrBackBuffer + 1) % APP_NUM_BACK_BUFFERS;

				// Wait until frame commands are complete.  This waiting is inefficient and is
				// done for simplicity.  Later we will show how to organize our rendering code
				// so we do not have to wait per frame.
				FlushCommandQueue();
			}
			else
			{
				Sleep(100);
			}
		}
	}

	CleanUp();
	return (int)msg.wParam;
}

void AppBase::UpdateForSizeChange(uint32_t clientWidth, uint32_t clientHeight)
{
	mClientWidth = clientWidth;
	mClientHeight = clientHeight;
	mAspectRatio = static_cast<float>(mClientWidth) / static_cast<float>(mClientHeight);
}

void AppBase::SetWindowBounds(int left, int top, int right, int bottom)
{
	mWindowRect.left = static_cast<LONG>(left);
	mWindowRect.top = static_cast<LONG>(top);
	mWindowRect.right = static_cast<LONG>(right);
	mWindowRect.bottom = static_cast<LONG>(bottom);
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
		text += L"\n";

		OutputDebugString(text.c_str());

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
		text += L"\n";
		OutputDebugString(text.c_str());

		LogOutputDisplayModes(output, mBackBufferFormat);

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
			L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
			L"\n";

		::OutputDebugString(text.c_str());
	}
}

void AppBase::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc
	{
		/* D3D12_DESCRIPTOR_HEAP_TYPE Type	*/D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		/* UINT NumDescriptors				*/APP_NUM_BACK_BUFFERS + 1,
		/* D3D12_DESCRIPTOR_HEAP_FLAGS Flags*/D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		/* UINT NodeMask					*/0
	};
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));


	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc
	{
		/* D3D12_DESCRIPTOR_HEAP_TYPE Type	*/D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
		/* UINT NumDescriptors				*/1,
		/* D3D12_DESCRIPTOR_HEAP_FLAGS Flags*/D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		/* UINT NodeMask					*/0
	};
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void AppBase::OnResize()
{
	assert(mDevice);
	assert(mSwapChain);
	assert(mCommandAllocator[mFrameIndex % gNumFrameResources]);

	// Flush before changing any resources.
	FlushCommandQueue();

	ThrowIfFailed(mCommandList->Reset(mCommandAllocator[mFrameIndex % gNumFrameResources].Get(), nullptr));

	// Release the previous resources we will be recreating.
	for (int i = 0; i < APP_NUM_BACK_BUFFERS; ++i)
		mSwapChainBuffer[i].Reset();
	mDepthStencilBuffer.Reset();

	// Resize the swap chain.
	ThrowIfFailed(mSwapChain->ResizeBuffers(
		APP_NUM_BACK_BUFFERS,
		mClientWidth, mClientHeight,
		mBackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurrBackBuffer = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; i++)
	{
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		mDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

	// Create the depth/stencil buffer and view.
	D3D12_RESOURCE_DESC depthStencilDesc
	{
		/* D3D12_RESOURCE_DIMENSION Dimension	*/	D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		/* UINT64 Alignment						*/	0,
		/* UINT64 Width							*/	mClientWidth,
		/* UINT Height							*/	mClientHeight,
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

	D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_CLEAR_VALUE optClear
	{
		/* DXGI_FORMAT Format;							*/mDepthStencilFormat,
		/* union {										*/{
		/*		FLOAT Color[4];							*/	
		/*		D3D12_DEPTH_STENCIL_VALUE DepthStencil{	*/	{
		/*			FLOAT Depth;						*/		1.0f,
		/*			UINT8 Stencil;						*/		0
		/*		}										*/	},
		/* } 											*/}
	};
	ThrowIfFailed(mDevice->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

	// Create descriptor to mip level 0 of entire resource using the format of the resource.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc
	{
		/* DXGI_FORMAT Format;								*/.Format = mDepthStencilFormat,
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
	// dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	// dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	// dsvDesc.Format = mDepthStencilFormat;
	// dsvDesc.Texture2D.MipSlice = 0;
	// dsvDesc.Texture2D.MipSlice = 0;
	mDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, DepthStencilView());

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
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, static_cast<LONG>(mClientWidth), static_cast<LONG>(mClientHeight) };
}

#pragma region Window
bool AppBase::RegisterWindowClass()
{
	mWindowClass = {
		/*UINT        cbSize		*/	sizeof(WNDCLASSEX),
		/*UINT        style			*/	CS_CLASSDC,
		/*WNDPROC     lpfnWndProc	*/	WndProc,
		/*int         cbClsExtra	*/	0L,
		/*int         cbWndExtra	*/	0L,
		/*HINSTANCE   hInstance		*/	GetModuleHandle(nullptr),
		/*HICON       hIcon			*/	nullptr,
		/*HCURSOR     hCursor		*/	nullptr,
		/*HBRUSH      hbrBackground	*/	nullptr,
		/*LPCWSTR     lpszMenuName	*/	L"", // MAKEINTRESOURCEW(IDR_MENU1),
		/*LPCWSTR     lpszClassName	*/	mTitle.c_str(), // lpszClassName, L-string
		/*HICON       hIconSm		*/	nullptr
	};

	if (!RegisterClassEx(&mWindowClass)) {
		std::cout << "RegisterClassEx() failed." << std::endl;
		MessageBox(0, L"Register WindowClass Failed.", 0, 0);
		return false;
	}

	return true;
}

bool AppBase::MakeWindowHandle()
{
	SetWindowBounds(0, 0, mClientWidth, mClientHeight);
	AdjustWindowRect(&mWindowRect, WS_OVERLAPPEDWINDOW, false);
	mHwndWindow = CreateWindow(
		/* _In_opt_ LPCWSTR lpClassName	*/ mWindowClass.lpszClassName,
		/* _In_opt_ LPCWSTR lpWindowName*/ mWndCaption.c_str(),
		/* _In_ DWORD dwStyle			*/ WS_OVERLAPPEDWINDOW,
		/* _In_ int X					*/ 100, // 윈도우 좌측 상단의 x 좌표
		/* _In_ int Y					*/ 100, // 윈도우 좌측 상단의 y 좌표
		/* _In_ int nWidth				*/ mClientWidth, // 윈도우 가로 방향 해상도
		/* _In_ int nHeight				*/ mClientHeight, // 윈도우 세로 방향 해상도
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
	// [DEBUG] Enable debug interface
#if defined(DEBUG) || defined(_DEBUG) 
	Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
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

	// [DEBUG] Setup debug interface to break on any warnings/errors
#if defined(DEBUG) || defined(_DEBUG) 
	if (debugController != nullptr)
	{
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> pInfoQueue = nullptr;
		mDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
	}
#endif

	ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)));

	mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Check 4X MSAA quality support for our back buffer format.
	// All Direct3D 11 capable devices support 4X MSAA for all render 
	// target formats, so we only need to check quality support.

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels
	{
		/*_In_  DXGI_FORMAT Format							*/mBackBufferFormat,
		/*_In_  UINT SampleCount							*/4,
		/*_In_  D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS Flags	*/D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE,
		/*_Out_  UINT NumQualityLevels						*/0
	};
	ThrowIfFailed(mDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels)));

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

#ifdef _DEBUG
	LogAdapters();
#endif

	CreateCommandObjects();
	CreateSwapChain();
	CreateRtvAndDsvDescriptorHeaps();

	CreateImGuiDescriptorHeaps();
	

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
	init_info.NumFramesInFlight = gNumFrameResources;
	init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
	// Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
	// (current version of the backend will only allocate one descriptor, future versions will need to allocate more)
	init_info.SrvDescriptorHeap = mSrvDescHeap;
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
	mSrvDescHeapAlloc.Create(mDevice.Get(), mSrvDescHeap);
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


	for (int i = 0; i < gNumFrameResources; ++i)
	{
		ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mCommandAllocator[i].GetAddressOf())));
	}

	ThrowIfFailed(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator[0].Get(), nullptr, IID_PPV_ARGS(mCommandList.GetAddressOf())));

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
		/* 	UINT Width									*/mClientWidth,
		/* 	UINT Height									*/mClientHeight,
		/* 	DXGI_RATIONAL RefreshRate					*/
		/*		UINT Numerator							*/60,
		/*		UINT Denominator						*/1,
		/* 	DXGI_FORMAT Format							*/mBackBufferFormat,
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
	ThrowIfFailed(mDxgiFactory->CreateSwapChain(mCommandQueue.Get(), &sd, mSwapChain.GetAddressOf()));
}

void AppBase::FlushCommandQueue()
{
	// Advance the fence value to mark commands up to this fence point.
	mFrameIndex++;

	// Add an instruction to the command queue to set a new fence point.  Because we 
	// are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mFrameIndex));

	// Wait until the GPU has completed commands up to this fence point.
	if (mFence->GetCompletedValue() < mFrameIndex)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, L"", false, EVENT_ALL_ACCESS);

		// Fire event when GPU hits current fence.  
		ThrowIfFailed(mFence->SetEventOnCompletion(mFrameIndex, eventHandle));

		// Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);					// TODO: Make More Fast Logic
		CloseHandle(eventHandle);
	}
}

ID3D12Resource* AppBase::CurrentBackBuffer() const
{
	return mSwapChainBuffer[mCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE AppBase::CurrentBackBufferView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(),mCurrBackBuffer, mRtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE AppBase::DepthStencilView() const
{
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
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
			L"   mspf: " + mspfStr;

		SetWindowText(mHwndWindow, windowText.c_str());

		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}


LRESULT AppBase::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	std::cout << "msg: " << std::hex << msg << std::hex << "  |  LPARAM: " << HIWORD(lParam) << " " << LOWORD(lParam) << "  |  WPARAM: " << HIWORD(wParam) << " " << LOWORD(wParam) << std::endl;

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
		mClientWidth = (UINT)LOWORD(lParam);
		mClientHeight = (UINT)HIWORD(lParam);
		UpdateForSizeChange(mClientWidth, mClientHeight);

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
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

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
		static_assert(sizeof(ImTextureID) >= sizeof(D3D12_CPU_DESCRIPTOR_HANDLE), "D3D12_CPU_DESCRIPTOR_HANDLE is too large to fit in an ImTextureID");

		D3D12_CPU_DESCRIPTOR_HANDLE my_texture_srv_cpu_handle;
		D3D12_GPU_DESCRIPTOR_HANDLE my_texture_srv_gpu_handle;
		mSrvDescHeapAlloc.Alloc(&my_texture_srv_cpu_handle, &my_texture_srv_gpu_handle);

		ImGui::SetNextWindowSize(ImVec2((float)mClientWidth, (float)mClientHeight));
		ImGui::Begin("Viewport1", &mShowViewport);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		
		
		// mSrvDescHeapAlloc.Alloc(&CurrentBackBufferView(), my_texture_srv_gpu_handle);
		//ImGui::Text("Hello from another window! = %s", my_texture_srv_gpu_handle);
		// ImGui::Image((ImTextureID)my_texture_srv_gpu_handle->ptr, ImVec2((float)mClientWidth, (float)mClientHeight));
		ImGui::End();
	}
}
