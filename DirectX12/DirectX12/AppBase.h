#pragma once
#include<d3d12.h>
#include <wrl/client.h> // ComPtr
#include<dxgi.h>
#include<string>

namespace kyun
{
	using Microsoft::WRL::ComPtr;
	using namespace std;

	class AppBase
	{
	public:
		AppBase();
		AppBase(uint32_t width, uint32_t height, std::wstring name);
		virtual ~AppBase();

		virtual void OnInit() = 0;
		virtual void OnUpdate() = 0;
		virtual void OnRender() = 0;
		virtual void OnSizeChanged(uint32_t width, uint32_t height, bool minimized) = 0;
		virtual void OnDestroy() = 0;

		// Samples override the event handlers to handle specific messages.
		virtual void OnKeyDown(UINT8 /*key*/) {}
		virtual void OnKeyUp(UINT8 /*key*/) {}
		virtual void OnWindowMoved(int /*x*/, int /*y*/) {}
		virtual void OnMouseMove(UINT /*x*/, UINT /*y*/) {}
		virtual void OnLeftButtonDown(UINT /*x*/, UINT /*y*/) {}
		virtual void OnLeftButtonUp(UINT /*x*/, UINT /*y*/) {}
		virtual void OnDisplayChanged() {}

		// Accessors.
		uint32_t GetWidth() const { return m_width; }
		uint32_t GetHeight() const { return m_height; }
		const wchar_t* GetTitle() const { return m_title.c_str(); }
		bool GetTearingSupport() const { return m_tearingSupport; }
		RECT GetWindowsBounds() const { return m_windowRect; }
		virtual IDXGISwapChain* GetSwapchain() { return nullptr; }

		void UpdateForSizeChange(uint32_t clientWidth, uint32_t clientHeight);
		void SetWindowBounds(int left, int top, int right, int bottom);

		// Operator
		virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM waram, LPARAM lParam);
		int Run();
		void ToggleFullscreenWindow(IDXGISwapChain* pOutput = nullptr);
		void SetWindowZorderToTopMost(bool setToTopMost);

	protected:
		void GetHardwareAdapter(
			_In_ IDXGIFactory1* pFactory,
			_Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter,
			bool requestHighPerformanceAdapter = false);
		void CheckTearingSupport();
	public:
		// Viewport dimensions.
		uint32_t m_width;
		uint32_t m_height;
		float m_aspectRatio;
		// Window bounds
		RECT m_windowRect;
		HWND m_hwnd;

		// Whether or not tearing is available for fullscreen borderless windowed mode.
		bool m_tearingSupport;
		// Adapter info.
		bool m_useWarpDevice;
		// Override to be able to start without Dx11on12 UI for PIX. PIX doesn't support 11 on 12. 
		bool m_enableUI;

		bool m_fullscreenMode;
	private:
		// Root assets path.
		std::wstring m_assetsPath;

		// Window title.
		std::wstring m_title;
		const UINT m_windowStyle = WS_OVERLAPPEDWINDOW;
	};
}


