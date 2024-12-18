#pragma once
#include <d3d12.h>
#include <wrl/client.h> // ComPtr
#include <dxgi1_4.h>
#include <string>
#include "Camera.h"
#include "GameTimer.h"

//class GameTimer;
//class Camera;
//class Model;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")

class AppBase
{
public:
	AppBase();
	AppBase(uint32_t width, uint32_t height, std::wstring name);
	virtual ~AppBase();

	void Set4xMsaaState(bool value);

	virtual bool OnInit();
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
	virtual void OnUpdate(const GameTimer dt) = 0;
	virtual void OnRender(const GameTimer dt) = 0;

	// Convenience overrides for handling mouse input.
	virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
	virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

protected:
	bool InitMainWindow();
	bool InitDirect3D();
	void CreateCommandObjects();
	void CreateSwapChain();

	void FlushCommandQueue();

	ID3D12Resource* CurrentBackBuffer()const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

	void CalculateFrameStats();

	std::wstring GetAssetFullPath(LPCWSTR assetName);
public:
	// Viewport dimensions.
	uint32_t mClientWidth;
	uint32_t mClientHeight;
	float mAspectRatio;
	// Window bounds
	RECT mWindowRect;
	HWND mHwnd;
	// Window title.
	std::wstring mTitle;
	std::wstring mWndCaption = L"d3d App";
	const UINT mWindowStyle = WS_OVERLAPPEDWINDOW;
public:
	// Root assets path.
	std::wstring mAssetsPath;

	bool mAppPaused = false;	// is the application paused?
	bool mMinimized = false;	// is the application minimized?
	bool mMaximized = false;	// is the application maximized?
	bool mResizing = false;	// are the resize bars being dragged?
	bool mFullscreenState = false;	// fullscreen enabled
	bool m4xMsaaState = false;	// 4X MSAA enabled
	UINT m4xMsaaQuality = 0;		// quality level of 4X MSAA

	GameTimer mTimer;
	// unique_ptr<Camera> mCamera;
	// unique_ptr<Model> mModel;

	// Pipeline objects.
	Microsoft::WRL::ComPtr<IDXGIFactory4> mDxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
	Microsoft::WRL::ComPtr<ID3D12Device> mDevice;

	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
	UINT64 mCurrentfence;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCommandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> mCommandList;

	static const UINT SwapChainBufferCount = 2;
	int mCurrentBackBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;

	D3D_DRIVER_TYPE mD3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
};