#pragma once
#include <d3d12.h>
#include <wrl/client.h> // ComPtr
#include <dxgi1_4.h>
#include <string>
#include <memory>
#include "../EngineCore/Camera.h"
#include "../EngineCore/GameTimer.h"
#include "../EngineCore/D3DUtil.h"

#include "../ImGuiCore/imgui.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")

extern class AppBase* g_appBasse;

#pragma region ImGui
// Simple free list based allocator
struct ExampleDescriptorHeapAllocator
{
	ID3D12DescriptorHeap* Heap = nullptr;
	D3D12_DESCRIPTOR_HEAP_TYPE  HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
	D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
	D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
	UINT                        HeapHandleIncrement;
	ImVector<int>               FreeIndices;

	void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap)
	{
		IM_ASSERT(Heap == nullptr && FreeIndices.empty());
		Heap = heap;
		D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
		HeapType = desc.Type;
		HeapStartCpu = Heap->GetCPUDescriptorHandleForHeapStart();
		HeapStartGpu = Heap->GetGPUDescriptorHandleForHeapStart();
		HeapHandleIncrement = device->GetDescriptorHandleIncrementSize(HeapType);
		FreeIndices.reserve((int)desc.NumDescriptors);
		for (int n = desc.NumDescriptors; n > 0; n--)
			FreeIndices.push_back(n);
	}
	void Destroy()
	{
		Heap = nullptr;
		FreeIndices.clear();
	}
	void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
	{
		IM_ASSERT(FreeIndices.Size > 0);
		int idx = FreeIndices.back();
		FreeIndices.pop_back();
		out_cpu_desc_handle->ptr = HeapStartCpu.ptr + (idx * HeapHandleIncrement);
		out_gpu_desc_handle->ptr = HeapStartGpu.ptr + (idx * HeapHandleIncrement);
	}
	void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
	{
		int cpu_idx = (int)((out_cpu_desc_handle.ptr - HeapStartCpu.ptr) / HeapHandleIncrement);
		int gpu_idx = (int)((out_gpu_desc_handle.ptr - HeapStartGpu.ptr) / HeapHandleIncrement);
		IM_ASSERT(cpu_idx == gpu_idx);
		FreeIndices.push_back(cpu_idx);
	}
};
#pragma endregion ImGui

class AppBase
{
public:
	static constexpr int APP_NUM_FRAME_RESOURCES = 3;	// must bigger than 1
	static constexpr int APP_NUM_BACK_BUFFERS = 3;
	static constexpr int APP_SRV_HEAP_SIZE = 64;
	static constexpr uint32_t WND_PADDING = 5;
public:
	AppBase();
	AppBase(uint32_t width, uint32_t height, std::wstring name);
	virtual ~AppBase();

	void Set4xMsaaState(bool value);

	virtual bool Initialize();
	virtual void CleanUp();
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
	virtual void Update() = 0;
	virtual void Render() = 0;

	// Convenience overrides for handling mouse input.
	virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
	virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

#pragma region Window
protected:
	bool RegisterWindowClass();
	bool MakeWindowHandle();
	bool InitMainWindow();
public:
	// Viewport dimensions.
	uint32_t mClientWidth;
	uint32_t mClientHeight;
	float mAspectRatio;
	WNDCLASSEXW mWindowClass;
	RECT mWindowRect;
	HWND mHwndWindow;

	// Window title.
	std::wstring mTitle;
	std::wstring mWndCaption = L"d3d App";
#pragma endregion Window

protected:
	bool InitDirect3D();
	void CreateCommandObjects();
	void CreateSwapChain();

	void FlushCommandQueue();

	void CalculateFrameStats();

	std::wstring GetAssetFullPath(LPCWSTR assetName);

#pragma region ImGui
protected:
	void CreateImGuiDescriptorHeaps();
	bool InitImgui();
	virtual void UpdateImGui();
	void ShowImguiViewport(bool* open);
public:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mImGuiHeap;
	bool mShowDemoWindow = false;
	bool mShowAnotherWindow = false;
	bool mShowViewport = false;
	ID3D12DescriptorHeap* mSrvDescHeap;
	ExampleDescriptorHeapAllocator mSrvDescHeapAlloc;

	Microsoft::WRL::ComPtr<ID3D12Resource> mRtvTexture;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvTextureHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE mRtvTextureDescriptor;
#pragma endregion ImGui

public:
	// Root assets path.
	std::wstring mAssetsPath;

	bool mAppPaused = false;		// is the application paused?
	bool mMinimized = false;		// is the application minimized?
	bool mMaximized = false;		// is the application maximized?
	bool mResizing = false;			// are the resize bars being dragged?
	bool mFullscreenState = false;	// fullscreen enabled
	bool m4xMsaaState = false;		// 4X MSAA enabled
	UINT m4xMsaaQuality = 0;		// quality level of 4X MSAA

	GameTimer mTimer;

	// Pipeline objects.
	Microsoft::WRL::ComPtr<IDXGIFactory4> mDxgiFactory;
	Microsoft::WRL::ComPtr<ID3D12Device> mDevice;

	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
	UINT64 mCurrentFence = 0;
	HANDLE mFenceEvent = nullptr;

	UINT mRtvDescriptorSize = 0;
	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[APP_NUM_BACK_BUFFERS];
	D3D12_CPU_DESCRIPTOR_HANDLE mSwapChainDescriptor[APP_NUM_BACK_BUFFERS];
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	int mCurrBackBuffer;

	UINT mDsvDescriptorSize = 0;
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE mDepthStencilDescriptor;

	UINT mCbvSrvUavDescriptorSize = 0;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCommandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> mCommandList;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	D3D_DRIVER_TYPE mD3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
};