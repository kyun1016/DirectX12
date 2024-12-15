#include <iostream>
#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include <dxgi1_6.h>
#include <windows.h>
#include "AppBase.h"
#include "AppBaseHelper.h"

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
		m_width(width),
		m_height(height),
		m_windowRect({ 0,0,0,0 }),
		m_title(name),
		m_aspectRatio(0.0f),
		m_useWarpDevice(false),
		m_enableUI(true)
	{
		WCHAR assetsPath[512];
		GetAssetsPath(assetsPath, _countof(assetsPath));

		g_appBase = this;

		UpdateForSizeChange(width, height);
		CheckTearingSupport();
	}

	AppBase::~AppBase()
	{
		g_appBase = nullptr;

		DestroyWindow(m_hwnd);
	}

	void AppBase::UpdateForSizeChange(uint32_t clientWidth, uint32_t clientHeight)
	{
		m_width = clientWidth;
		m_height = clientHeight;
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

	void AppBase::GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter)
	{
		*ppAdapter = nullptr;

		ComPtr<IDXGIAdapter1> adapter;

		ComPtr<IDXGIFactory6> factory6;
		if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
		{
			for (
				UINT adapterIndex = 0;
				SUCCEEDED(factory6->EnumAdapterByGpuPreference(
					adapterIndex,
					requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
					IID_PPV_ARGS(&adapter)));
				++adapterIndex)
			{
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);

				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					// Don't select the Basic Render Driver adapter.
					// If you want a software adapter, pass in "/warp" on the command line.
					continue;
				}

				// Check to see whether the adapter supports Direct3D 12, but don't create the
				// actual device yet.
				if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
				{
					break;
				}
			}
		}

		if (adapter.Get() == nullptr)
		{
			for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
			{
				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);

				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					// Don't select the Basic Render Driver adapter.
					// If you want a software adapter, pass in "/warp" on the command line.
					continue;
				}

				// Check to see whether the adapter supports Direct3D 12, but don't create the
				// actual device yet.
				if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
				{
					break;
				}
			}
		}

		*ppAdapter = adapter.Detach();
	}
	void AppBase::CheckTearingSupport()
	{
#ifndef PIXSUPPORT
		ComPtr<IDXGIFactory6> factory;
		HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
		BOOL allowTearing = FALSE;
		if (SUCCEEDED(hr))
		{
			hr = factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
		}

		m_tearingSupport = SUCCEEDED(hr) && allowTearing;
#else
		m_tearingSupport = TRUE;
#endif
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

		g_appBase->OnDestroy();

		// Return this part of the WM_QUIT message to Windows.
		return static_cast<char>(msg.wParam);
	}

	void AppBase::ToggleFullscreenWindow(IDXGISwapChain* pSwapChain)
	{
		if (m_fullscreenMode)
		{
			// Restore the window's attributes and size.
			SetWindowLong(m_hwnd, GWL_STYLE, m_windowStyle);

			SetWindowPos(
				m_hwnd,
				HWND_NOTOPMOST,
				m_windowRect.left,
				m_windowRect.top,
				m_windowRect.right - m_windowRect.left,
				m_windowRect.bottom - m_windowRect.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);

			ShowWindow(m_hwnd, SW_NORMAL);
		}
		else
		{
			// Save the old window rect so we can restore it when exiting fullscreen mode.
			GetWindowRect(m_hwnd, &m_windowRect);

			// Make the window borderless so that the client area can fill the screen.
			SetWindowLong(m_hwnd, GWL_STYLE, m_windowStyle & ~(WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU | WS_THICKFRAME));

			RECT fullscreenWindowRect;
			try
			{
				if (pSwapChain)
				{
					// Get the settings of the display on which the app's window is currently displayed
					ComPtr<IDXGIOutput> pOutput;
					ThrowIfFailed(pSwapChain->GetContainingOutput(&pOutput));
					DXGI_OUTPUT_DESC Desc;
					ThrowIfFailed(pOutput->GetDesc(&Desc));
					fullscreenWindowRect = Desc.DesktopCoordinates;
				}
				else
				{
					// Fallback to EnumDisplaySettings implementation
					throw HrException(S_FALSE);
				}
			}
			catch (HrException& e)
			{
				UNREFERENCED_PARAMETER(e);

				// Get the settings of the primary display
				DEVMODE devMode = {};
				devMode.dmSize = sizeof(DEVMODE);
				EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &devMode);

				fullscreenWindowRect = {
					devMode.dmPosition.x,
					devMode.dmPosition.y,
					devMode.dmPosition.x + static_cast<LONG>(devMode.dmPelsWidth),
					devMode.dmPosition.y + static_cast<LONG>(devMode.dmPelsHeight)
				};
			}

			SetWindowPos(
				m_hwnd,
				HWND_TOPMOST,
				fullscreenWindowRect.left,
				fullscreenWindowRect.top,
				fullscreenWindowRect.right,
				fullscreenWindowRect.bottom,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);


			ShowWindow(m_hwnd, SW_MAXIMIZE);
		}

		m_fullscreenMode = !m_fullscreenMode;
	}

	void AppBase::SetWindowZorderToTopMost(bool setToTopMost)
	{
		RECT windowRect;
		GetWindowRect(m_hwnd, &windowRect);

		SetWindowPos(
			m_hwnd,
			(setToTopMost) ? HWND_TOPMOST : HWND_NOTOPMOST,
			windowRect.left,
			windowRect.top,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			SWP_FRAMECHANGED | SWP_NOACTIVATE);
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

		//	m_width = uint32_t(LOWORD(lParam));
		//	m_height = uint32_t(HIWORD(lParam));

		//	RECT windowRect = {};
		//	GetWindowRect(hwnd, &windowRect);
		//	SetWindowBounds(windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);

		//	// 윈도우가 Minimize 모드에서는 screenWidth/Height가 0
		//	if (m_width && m_height) {

		//		cout << "Resize SwapChain to " << m_width << " " << m_height << endl;

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
		//		//	{ m_backBufferRTV }, m_width, m_height, 4);
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

