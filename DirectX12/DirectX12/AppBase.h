#pragma once
#include<d3d12.h>
#include <wrl/client.h> // ComPtr
#include<dxgi.h>
#include<string>

namespace kyun
{
	using Microsoft::WRL::ComPtr;
	using namespace std;

	class StepTimer;
	class Camera;
	class Model;

	class AppBase
	{
	public:
		AppBase();
		AppBase(uint32_t width, uint32_t height, std::wstring name);
		virtual ~AppBase();

		int Run();

		virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM waram, LPARAM lParam);

		void UpdateForSizeChange(uint32_t clientWidth, uint32_t clientHeight);
		void SetWindowBounds(int left, int top, int right, int bottom);

		void LogAdapters();
		void LogAdapterOutputs(IDXGIAdapter* adapter);
		void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

	protected:
		virtual void CreateRtvAndDsvDescriptorHeaps();
		virtual void OnResize();
		virtual void OnUpdate(const StepTimer& dt) = 0;
		virtual void OnDraw(const StepTimer& dt) = 0;

		// Convenience overrides for handling mouse input.
		virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
		virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
		virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

	protected:
		virtual bool OnInit();
		bool InitMainWindow();
		bool InitDirect3D();
		void CreateCommandObjects();
		void CreateSwapChain();

		void FlushCommandQueue();

		ID3D12Resource* CurrentBackBuffer()const;
		D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
		D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

		wstring GetAssetFullPath(LPCWSTR assetName);
	public:
		// Viewport dimensions.
		uint32_t m_clientWidth;
		uint32_t m_clientHeight;
		float m_aspectRatio;
		// Window bounds
		RECT m_windowRect;
		HWND m_hwnd;
		// Window title.
		std::wstring m_title;
		std::wstring m_wndCaption = L"d3d App";
		const UINT m_windowStyle = WS_OVERLAPPEDWINDOW;
	public:
		// Root assets path.
		std::wstring m_assetsPath;

		bool m_appPaused		= false;	// is the application paused?
		bool m_minimized		= false;	// is the application minimized?
		bool m_maximized		= false;	// is the application maximized?
		bool m_resizing			= false;	// are the resize bars being dragged?
		bool m_fullscreenState	= false;	// fullscreen enabled
		bool m_4xMsaaState		= false;	// 4X MSAA enabled
		UINT m_4xMsaaQuality	= 0;		// quality level of 4X MSAA

		unique_ptr<StepTimer> m_timer;
		unique_ptr<Camera> m_camera;
		unique_ptr<Model> m_model;

		// Pipeline objects.
		ComPtr<IDXGIFactory4> m_dxgiFactory;
		ComPtr<IDXGISwapChain> m_swapChain;
		ComPtr<ID3D12Device> m_device;

		ComPtr<ID3D12Fence> m_fence;
		UINT64 m_currentfence;

		ComPtr<ID3D12CommandQueue> m_commandQueue;
		ComPtr<ID3D12CommandAllocator> m_commandAllocator;
		ComPtr<ID3D12GraphicsCommandList6> m_commandList;

		static const UINT SwapChainBufferCount = 2;
		int m_currentBackBuffer;
		ComPtr<ID3D12Resource> m_swapChainBuffer[SwapChainBufferCount];
		ComPtr<ID3D12Resource> m_depthStencilBuffer;

		ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
		ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

		D3D12_VIEWPORT m_screenViewport;
		D3D12_RECT m_scissorRect;

		UINT m_rtvDescriptorSize = 0;
		UINT m_dsvDescriptorSize = 0;
		UINT m_cbvSrvUavDescriptorSize = 0;

		D3D_DRIVER_TYPE m_d3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
		DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		DXGI_FORMAT m_depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	};
}


