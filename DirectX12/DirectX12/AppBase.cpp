#include <iostream>
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include <dxgi1_6.h>
#include <windows.h>
#include "AppBase.h"
#include "AppBaseHelper.h"
#include "D3DUtil.h"
#include "Camera.h"
#include "StepTimer.h"
#include "d3dx12.h"

// imgui_impl_win32.cpp에 정의된 메시지 처리 함수에 대한 전방 선언
// Vcpkg를 통해 IMGUI를 사용할 경우 빨간줄로 경고가 뜰 수 있음
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam);

namespace kyun
{
	using namespace std;

	//===================================
	// Constructor
	//===================================
	AppBase::AppBase() : AppBase(1080, 720, L"AppBase") {}

	AppBase::AppBase(uint32_t width, uint32_t height, std::wstring name) :
		m_clientWidth(width),
		m_clientHeight(height),
		m_windowRect({ 0,0,0,0 }),
		m_title(name),
		m_aspectRatio(0.0f)
	{
		WCHAR assetsPath[512];
		GetAssetsPath(assetsPath, _countof(assetsPath));

		g_appBase = this;

		UpdateForSizeChange(width, height);

		m_timer = make_unique<StepTimer>();
		m_camera = make_unique<Camera>();
	}

	AppBase::~AppBase()
	{
		g_appBase = nullptr;

		DestroyWindow(m_hwnd);
	}

	void AppBase::UpdateForSizeChange(uint32_t clientWidth, uint32_t clientHeight)
	{
		m_clientWidth = clientWidth;
		m_clientHeight = clientHeight;
		m_aspectRatio = static_cast<float>(clientWidth) / static_cast<float>(clientHeight);
	}

	void AppBase::SetWindowBounds(int left, int top, int right, int bottom)
	{
		m_windowRect.left = static_cast<LONG>(left);
		m_windowRect.top = static_cast<LONG>(top);
		m_windowRect.right = static_cast<LONG>(right);
		m_windowRect.bottom = static_cast<LONG>(bottom);
	}

	wstring AppBase::GetAssetFullPath(LPCWSTR assetName)
	{
		return m_assetsPath + assetName;
	}

	int AppBase::Run()
	{
		g_appBase->OnInit();

		// Main sample loop.
		MSG msg = {};
		while (msg.message != WM_QUIT)
		{
			// Process any messages in the queue.
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		// Return this part of the WM_QUIT message to Windows.
		return static_cast<char>(msg.wParam);
	}

	void AppBase::LogAdapters()
	{
		UINT i = 0;
		IDXGIAdapter* adapter = nullptr;
		std::vector<IDXGIAdapter*> adapterList;
		while (m_dxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
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

			LogOutputDisplayModes(output, m_backBufferFormat);

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
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
		rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		rtvHeapDesc.NodeMask = 0;
		ThrowIfFailed(m_device->CreateDescriptorHeap(
			&rtvHeapDesc, IID_PPV_ARGS(m_rtvHeap.GetAddressOf())));


		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		dsvHeapDesc.NodeMask = 0;
		ThrowIfFailed(m_device->CreateDescriptorHeap(
			&dsvHeapDesc, IID_PPV_ARGS(m_dsvHeap.GetAddressOf())));
	}

	void AppBase::OnResize()
	{
		assert(m_device);
		assert(m_swapChain);
		assert(m_commandAllocator);

		// Flush before changing any resources.
		FlushCommandQueue();

		ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

		// Release the previous resources we will be recreating.
		for (int i = 0; i < SwapChainBufferCount; ++i)
			m_swapChainBuffer[i].Reset();
		m_depthStencilBuffer.Reset();

		// Resize the swap chain.
		ThrowIfFailed(m_swapChain->ResizeBuffers(
			SwapChainBufferCount,
			m_clientWidth, m_clientHeight,
			m_backBufferFormat,
			DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

		m_currentBackBuffer = 0;

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
		for (UINT i = 0; i < SwapChainBufferCount; i++)
		{
			ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_swapChainBuffer[i])));
			m_device->CreateRenderTargetView(m_swapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
			rtvHeapHandle.Offset(1, m_rtvDescriptorSize);
		}

		// Create the depth/stencil buffer and view.
		D3D12_RESOURCE_DESC depthStencilDesc;
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Alignment = 0;
		depthStencilDesc.Width = m_clientWidth;
		depthStencilDesc.Height = m_clientHeight;
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;

		// Correction 11/12/2016: SSAO chapter requires an SRV to the depth buffer to read from 
		// the depth buffer.  Therefore, because we need to create two views to the same resource:
		//   1. SRV format: DXGI_FORMAT_R24_UNORM_X8_TYPELESS
		//   2. DSV Format: DXGI_FORMAT_D24_UNORM_S8_UINT
		// we need to create the depth buffer resource with a typeless format.  
		depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

		depthStencilDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
		depthStencilDesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
		depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE optClear;
		optClear.Format = m_depthStencilFormat;
		optClear.DepthStencil.Depth = 1.0f;
		optClear.DepthStencil.Stencil = 0;
		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_COMMON,
			&optClear,
			IID_PPV_ARGS(m_depthStencilBuffer.GetAddressOf())));

		// Create descriptor to mip level 0 of entire resource using the format of the resource.
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = m_depthStencilFormat;
		dsvDesc.Texture2D.MipSlice = 0;
		m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), &dsvDesc, DepthStencilView());

		// Transition the resource from its initial state to be used as a depth buffer.
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_depthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

		// Execute the resize commands.
		ThrowIfFailed(m_commandList->Close());
		ID3D12CommandList* cmdsLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

		// Wait until resize is complete.
		FlushCommandQueue();

		// Update the viewport transform to cover the client area.
		m_screenViewport.TopLeftX = 0;
		m_screenViewport.TopLeftY = 0;
		m_screenViewport.Width = static_cast<float>(m_clientWidth);
		m_screenViewport.Height = static_cast<float>(m_clientHeight);
		m_screenViewport.MinDepth = 0.0f;
		m_screenViewport.MaxDepth = 1.0f;

		m_scissorRect = { 0, 0, static_cast<LONG>(m_clientWidth), static_cast<LONG>(m_clientHeight) };
	}

	bool AppBase::OnInit()
	{
		if (!InitMainWindow())
			return false;

		if (!InitDirect3D())
			return false;

		// Do the initial resize code.
		OnResize();

		return true;
	}

	bool AppBase::InitMainWindow()
	{
		WNDCLASSEX wc = { sizeof(WNDCLASSEX),
				 CS_CLASSDC,
				 WndProc,
				 0L,
				 0L,
				 GetModuleHandle(NULL),
				 NULL,
				 NULL,
				 NULL,
				 NULL,
				 m_title.c_str(), // lpszClassName, L-string
				 NULL };

		if (!RegisterClassEx(&wc)) {
			cout << "RegisterClassEx() failed." << endl;
			MessageBox(0, L"CreateWindow Failed.", 0, 0);
			return false;
		}

		RECT wr = { 0, 0, m_clientWidth, m_clientHeight };
		AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false);
		m_hwnd = CreateWindow(wc.lpszClassName, m_wndCaption.c_str(),
			WS_OVERLAPPEDWINDOW,
			100, // 윈도우 좌측 상단의 x 좌표
			100, // 윈도우 좌측 상단의 y 좌표
			wr.right - wr.left, // 윈도우 가로 방향 해상도
			wr.bottom - wr.top, // 윈도우 세로 방향 해상도
			NULL, NULL, wc.hInstance, NULL);

		if (!m_hwnd) {
			cout << "CreateWindow() failed." << endl;
			MessageBox(0, L"CreateWindow Failed.", 0, 0);
			return false;
		}

		ShowWindow(m_hwnd, SW_SHOWDEFAULT);
		UpdateWindow(m_hwnd);

		return true;
	}

	bool AppBase::InitDirect3D()
	{
		#if defined(DEBUG) || defined(_DEBUG) 
		// Enable the D3D12 debug layer.
		{
			ComPtr<ID3D12Debug> debugController;
			ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
			debugController->EnableDebugLayer();
		}
		#endif

		ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgiFactory)));

		// Try to create hardware device.
		HRESULT hardwareResult = D3D12CreateDevice(
			nullptr,             // default adapter
			D3D_FEATURE_LEVEL_12_0,
			IID_PPV_ARGS(&m_device));

		// Fallback to WARP device.
		if (FAILED(hardwareResult))
		{
			ComPtr<IDXGIAdapter> pWarpAdapter;
			ThrowIfFailed(m_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

			ThrowIfFailed(D3D12CreateDevice(
				pWarpAdapter.Get(),
				D3D_FEATURE_LEVEL_11_0,
				IID_PPV_ARGS(&m_device)));
		}

		ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(&m_fence)));

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		m_cbvSrvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// Check 4X MSAA quality support for our back buffer format.
		// All Direct3D 11 capable devices support 4X MSAA for all render 
		// target formats, so we only need to check quality support.

		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
		msQualityLevels.Format = m_backBufferFormat;
		msQualityLevels.SampleCount = 4;
		msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
		msQualityLevels.NumQualityLevels = 0;
		ThrowIfFailed(m_device->CheckFeatureSupport(
			D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
			&msQualityLevels,
			sizeof(msQualityLevels)));

		m_4xMsaaState = msQualityLevels.NumQualityLevels;
		assert(m_4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

#ifdef _DEBUG
		LogAdapters();
#endif

		CreateCommandObjects();
		CreateSwapChain();
		CreateRtvAndDsvDescriptorHeaps();

		return true;
	}

	void AppBase::CreateCommandObjects()
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

		ThrowIfFailed(m_device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(m_commandAllocator.GetAddressOf())));

		ThrowIfFailed(m_device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_commandAllocator.Get(), // Associated command allocator
			nullptr,                   // Initial PipelineStateObject
			IID_PPV_ARGS(m_commandList.GetAddressOf())));

		// Start off in a closed state.  This is because the first time we refer 
		// to the command list we will Reset it, and it needs to be closed before
		// calling Reset.
		m_commandList->Close();
	}

	void AppBase::CreateSwapChain()
	{
		// Release the previous swapchain we will be recreating.
		m_swapChain.Reset();

		DXGI_SWAP_CHAIN_DESC sd;
		sd.BufferDesc.Width = m_clientWidth;
		sd.BufferDesc.Height = m_clientHeight;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferDesc.Format = m_backBufferFormat;
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		sd.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
		sd.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = SwapChainBufferCount;
		sd.OutputWindow = m_hwnd;
		sd.Windowed = true;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		// Note: Swap chain uses queue to perform flush.
		ThrowIfFailed(m_dxgiFactory->CreateSwapChain(
			m_commandQueue.Get(),
			&sd,
			m_swapChain.GetAddressOf()));
	}

	void AppBase::FlushCommandQueue()
	{
		// Advance the fence value to mark commands up to this fence point.
		m_currentfence++;

		// Add an instruction to the command queue to set a new fence point.  Because we 
		// are on the GPU timeline, the new fence point won't be set until the GPU finishes
		// processing all the commands prior to this Signal().
		ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_currentfence));

		// Wait until the GPU has completed commands up to this fence point.
		if (m_fence->GetCompletedValue() < m_currentfence)
		{
			HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

			// Fire event when GPU hits current fence.  
			ThrowIfFailed(m_fence->SetEventOnCompletion(m_currentfence, eventHandle));

			// Wait until the GPU hits current fence event is fired.
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
	}

	ID3D12Resource* AppBase::CurrentBackBuffer() const
	{
		return m_swapChainBuffer[m_currentBackBuffer].Get();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE AppBase::CurrentBackBufferView() const
	{
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(
			m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
			m_currentBackBuffer,
			m_rtvDescriptorSize);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE AppBase::DepthStencilView() const
	{
		return m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
	}


	LRESULT AppBase::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
			return true;

		//switch (msg) {
		//case WM_SIZE:
		//	// 화면 해상도가 바뀌면 SwapChain을 다시 생성
		//	if (!m_swapChain) {
		//		return;
		//	}

		//	m_clientWidth = uint32_t(LOWORD(lParam));
		//	m_clientHeight = uint32_t(HIWORD(lParam));

		//	RECT windowRect = {};
		//	GetWindowRect(hwnd, &windowRect);
		//	SetWindowBounds(windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);

		//	// 윈도우가 Minimize 모드에서는 screenWidth/Height가 0
		//	if (m_clientWidth && m_clientHeight) {

		//		cout << "Resize SwapChain to " << m_clientWidth << " " << m_clientHeight << endl;

		//		//m_backBufferRTV.Reset();
		//		//m_swapChain->ResizeBuffers(
		//		//	0,                    // 현재 개수 유지
		//		//	(UINT)LOWORD(lParam), // 해상도 변경
		//		//	(UINT)HIWORD(lParam),
		//		//	DXGI_FORMAT_UNKNOWN, // 현재 포맷 유지
		//		//	0);
		//		//CreateBuffers();
		//		//SetMainViewport();
		//		//m_camera.SetAspectRatio(this->GetAspectRatio());

		//		//m_postProcess.Initialize(
		//		//	m_device, m_context, { m_postEffectsSRV, m_prevSRV },
		//		//	{ m_backBufferRTV }, m_clientWidth, m_clientHeight, 4);
		//	}
		//	break;
		//case WM_SYSCOMMAND:
		//	if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
		//		return 0;
		//	break;
		//case WM_MOUSEMOVE:
		//	OnMouseMove(LOWORD(lParam), HIWORD(lParam));
		//	break;
		//case WM_LBUTTONDOWN:
		//	if (!m_leftButton) {
		//		m_dragStartFlag = true; // 드래그를 새로 시작하는지 확인
		//	}
		//	m_leftButton = true;
		//	OnMouseClick(LOWORD(lParam), HIWORD(lParam));
		//	break;
		//case WM_LBUTTONUP:
		//	m_leftButton = false;
		//	break;
		//case WM_RBUTTONDOWN:
		//	if (!m_rightButton) {
		//		m_dragStartFlag = true; // 드래그를 새로 시작하는지 확인
		//	}
		//	m_rightButton = true;
		//	break;
		//case WM_RBUTTONUP:
		//	m_rightButton = false;
		//	break;
		//case WM_KEYDOWN:
		//	m_keyPressed[wParam] = true;
		//	if (wParam == VK_ESCAPE) { // ESC키 종료
		//		DestroyWindow(hwnd);
		//	}
		//	if (wParam == VK_SPACE) {
		//		m_lightRotate = !m_lightRotate;
		//	}
		//	break;
		//case WM_KEYUP:
		//	if (wParam == 'F') { // f키 일인칭 시점
		//		m_camera.m_useFirstPersonView = !m_camera.m_useFirstPersonView;
		//	}
		//	if (wParam == 'C') { // c키 화면 캡쳐
		//		ComPtr<ID3D11Texture2D> backBuffer;
		//		m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
		//		D3D11Utils::WriteToPngFile(m_device, m_context, backBuffer,
		//			"captured.png");
		//	}
		//	if (wParam == 'P') { // 애니메이션 일시중지할 때 사용
		//		m_pauseAnimation = !m_pauseAnimation;
		//	}
		//	if (wParam == 'Z') { // 카메라 설정 화면에 출력
		//		m_camera.PrintView();
		//	}

		//	m_keyPressed[wParam] = false;
		//	break;
		//case WM_MOUSEWHEEL:
		//	m_wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		//	break;
		//case WM_DESTROY:
		//	::PostQuitMessage(0);
		//	return 0;
		//}

		return ::DefWindowProc(hwnd, msg, wParam, lParam);
	}
	}

