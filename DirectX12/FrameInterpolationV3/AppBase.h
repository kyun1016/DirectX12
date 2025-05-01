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
#include "FrameResource.h"
#include "DeviceManager.h"

#pragma comment(lib, "..\\StreamlineCore\\streamline\\lib\\x64\\sl.interposer.lib")
#define SL_PROVIDE_IMPLEMENTATION
#define SL_WINDOWS 1

// Streamline Core
#include <sl.h>
#include <sl_consts.h>
#include <sl_hooks.h>
#include <sl_version.h>

// Streamline Features
#include <sl_dlss.h>
#include <sl_reflex.h>
#include <sl_nis.h>
#include <sl_dlss_g.h>
#include <sl_deepdvc.h>

// #include <sl_core_api.h>
// #include <sl_core_types.h>

// donut/core
#include "log.h"


#pragma comment(lib, "d3dcompiler.lib")

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
	inline static AppBase* g_appBase;
public:
	static constexpr int APP_ID = 231313132;
	static constexpr uint64_t SDK_VERSION = sl::kSDKVersion;

	static constexpr int APP_NUM_FRAME_RESOURCES = 5;	// must bigger than 1
	static constexpr int APP_NUM_BACK_BUFFERS = 3;
	
	static constexpr int APP_SRV_HEAP_SIZE = 64;
	static constexpr int RTV_TOY_SIZE = 13;
	static constexpr int RTV_USER_SIZE = 1;	// 0: Render Target, 1: Copy Target
	static constexpr int SRV_USER_SIZE = RTV_USER_SIZE + 1 + RTV_TOY_SIZE;	// 0: Render Target, 1: CS Copy, 2~N: Shader Toy
	static constexpr uint32_t WND_PADDING = 5;
public:
	AppBase();
	AppBase(uint32_t width, uint32_t height, std::wstring name);
	virtual ~AppBase();


	// virtual AppBase* GetInstance() = 0;
	void Release();

public:
	AppBase(const AppBase&) = delete;
	AppBase(AppBase&&) = delete;
	AppBase& operator=(const AppBase&) = delete;
	AppBase& operator=(AppBase&&) = delete;

	float AspectRatio() const;

	void Set4xMsaaState(bool value);

	virtual bool Initialize();
	virtual void CleanUp();
	int Run();

	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM waram, LPARAM lParam);

	void UpdateForSizeChange(uint32_t clientWidth, uint32_t clientHeight);
	void SetWindowBounds(RECT& rect, int left, int top, int right, int bottom);

	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter* adapter);
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

	void FlushCommandQueue();
protected:
	virtual void CreateRtvAndDsvDescriptorHeaps(UINT numRTV = APP_NUM_BACK_BUFFERS + RTV_USER_SIZE, UINT numDSV = 1, UINT numRTVST = RTV_TOY_SIZE);
	virtual void OnResize();
	virtual void Update() = 0;
	virtual void Render() = 0;
	virtual void Sync() = 0;

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
	WNDCLASSEXW mWindowClass;
	HWND mHwndWindow;
	std::wstring mWndCaption = L"d3d App"; // Window title.

	DeviceParameter mParam;
	uint32_t mLastClientWidth = 0;
	uint32_t mLastClientHeight = 0;
#pragma endregion Window

protected:
	bool InitDirect3D();
	void CreateCommandObjects();
	void CreateSwapChain();

	void CalculateFrameStats();

	std::wstring GetAssetFullPath(LPCWSTR assetName);

#pragma region ImGui
protected:
	void CreateImGuiDescriptorHeaps();
	bool InitImgui();
	virtual void UpdateImGui();
	void RenderImGui();
	void ShowImguiViewport(bool* open);
public:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mImGuiHeap;
	bool mShowDemoWindow = false;
	bool mShowAnotherWindow = false;
	bool mShowViewport = false;
	ID3D12DescriptorHeap* mSrvDescHeap;
	ExampleDescriptorHeapAllocator mSrvDescHeapAlloc;

#pragma endregion ImGui
#pragma region Streamline
	bool mSLInitialised = false;
	// sl::log::ILog* mLog;
	sl::Preferences mPref;
	UIData m_ui;

	void UpdateFeatureAvailable();
	// void logFunctionCallback(sl::LogType type, const char* msg);
	// sl::Resource allocateResourceCallback(const sl::ResourceAllocationDesc* resDesc, void* device);
	std::wstring GetSlInterposerDllLocation();
	bool InitSLLog();
	bool LoadStreamline();
	bool SuccessCheck(sl::Result result, const char* location);

	bool BeginFrame();
	void SLFrameInit();

	struct PipelineCallbacks {
		std::function<void(AppBase&, uint32_t)> beforeFrame = nullptr;
		std::function<void(AppBase&, uint32_t)> beforeAnimate = nullptr;
		std::function<void(AppBase&, uint32_t)> afterAnimate = nullptr;
		std::function<void(AppBase&, uint32_t)> beforeRender = nullptr;
		std::function<void(AppBase&, uint32_t)> afterRender = nullptr;
		std::function<void(AppBase&, uint32_t)> beforePresent = nullptr;
		std::function<void(AppBase&, uint32_t)> afterPresent = nullptr;
	} m_callbacks;

	bool m_dlss_available = false;
	sl::DLSSOptions m_dlss_consts{};

	bool m_nis_available = false;
	sl::NISOptions m_nis_consts{};

	bool m_deepdvc_available = false;
	sl::DeepDVCOptions m_deepdvc_consts{};  

	bool m_dlssg_available = false;
	bool m_dlssg_triggerswapchainRecreation = false;
	bool m_dlssg_shoudLoad = false;
	sl::DLSSGOptions m_dlssg_consts{};
	sl::DLSSGState m_dlssg_settings{};

	bool m_latewarp_available = false;
	bool m_latewarp_triggerSwapchainRecreation = false;
	bool m_latewarp_shouldLoad = false;

	bool m_reflex_available = false;
	sl::ReflexOptions m_reflex_consts{};
	bool m_reflex_driverFlashIndicatorEnable = false;
	bool m_pcl_available = false;

	sl::FrameToken* m_currentFrame;
	sl::ViewportHandle m_viewport = { 0 };

	void ReflexCallback_Sleep(AppBase& manager, uint32_t frameID);
	void ReflexCallback_SimStart(AppBase& manager, uint32_t frameID);
	void ReflexCallback_SimEnd(AppBase& manager, uint32_t frameID);
	void ReflexCallback_RenderStart(AppBase& manager, uint32_t frameID);
	void ReflexCallback_RenderEnd(AppBase& manager, uint32_t frameID);
	void ReflexCallback_PresentStart(AppBase& manager, uint32_t frameID);
	void ReflexCallback_PresentEnd(AppBase& manager, uint32_t frameID);

	void QueryDeepDVCState(uint64_t& estimatedVRamUsage);
	void SetReflexConsts(const sl::ReflexOptions options);

	// DLSSG
	void SetDLSSGOptions(const sl::DLSSGOptions consts);
	bool GetDLSSGAvailable() { return m_dlssg_available; }
	bool GetDLSSGLastEnable() { return m_dlssg_consts.mode != sl::DLSSGMode::eOff; }
	uint64_t GetDLSSGLastFenceValue() { return m_dlssg_settings.lastPresentInputsProcessingCompletionFenceValue; }
	void QueryDLSSGState(uint64_t& estimatedVRamUsage, int& fps_multiplier, sl::DLSSGStatus& status, int& minSize, int& maxFrameCount, void*& pFence, uint64_t& fenceValue);
	void Set_DLSSG_SwapChainRecreation(bool on) { m_dlssg_triggerswapchainRecreation = true; m_dlssg_shoudLoad = on; }
	bool Get_DLSSG_SwapChainRecreation(bool& turn_on) const;
	void Quiet_DLSSG_SwapChainRecreation() { m_dlssg_triggerswapchainRecreation = false; }
	void CleanupDLSSG(bool wfi);

#pragma endregion Streamline

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
	UINT64 mFrameCount = 0;
	HANDLE mFenceEvent = nullptr;
	int mCurrBackBuffer = 0;

	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> mSwapChainBuffer;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> mhCPUSwapChainBuffer;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> mhCPUDescHandleST;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> mSRVUserBuffer;

	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> mhCPUDSVBuffer;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mCommandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> mCommandList;

	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	D3D_DRIVER_TYPE mD3dDriverType = D3D_DRIVER_TYPE_HARDWARE;

	bool InitDebugLayer();
	bool ShutdownDebugLayer();

	bool mSyncEn = false;
#if defined(DEBUG) || defined(_DEBUG) 
	// Debug
	Microsoft::WRL::ComPtr<ID3D12Debug> mD12Debug;
	Microsoft::WRL::ComPtr<IDXGIDebug1> mDxgiDebug;
#endif

	float mMouseNdcX = 0.0f;
	float mMouseNdcY = 0.0f;
	int mMouseX = -1;
	int mMouseY = -1;

	// Const Buffer
	ShaderToyConstants mCBShaderToy;
};