#pragma once
#include <random>
#include <d3d12.h>
#include <wrl/client.h> // ComPtr
#include <dxgi1_4.h>
#include <string>
#include <memory>
#include "../EngineCore/Camera.h"
#include "../EngineCore/GameTimer.h"
#include "../EngineCore/D3DUtil.h"
#include "../ImGuiCore/imgui.h"
#include "../ImGuiCore/imgui_internal.h"
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

using namespace donut::math;
#include "taa_cb.h"
#include "view_cb.h"
#include "gbuffer_cb.h"

#pragma comment(lib, "d3dcompiler.lib")

#pragma region ImGui

namespace DirectX
{
	namespace Detail
	{
		static const UINT PIX_EVENT_UNICODE_VERSION = 0;
		static const UINT PIX_EVENT_ANSI_VERSION = 1;
		static const size_t PIX_STRING_BUFFER_COUNT = 1024;
	}
}

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

	static constexpr int APP_NUM_FRAME_RESOURCES = 2;	// must bigger than 1
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
	void ShowStreamlineWindow();

	bool mShowStreamlineWindow = false;
public:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mImGuiHeap;
	bool mShowDemoWindow = false;
	bool mShowAnotherWindow = false;
	bool mShowViewport = false;

	bool m_dev_view = false;
	int m_dev_view_TopLevelDLSS = 1;
	int m_dev_view_dlss_mode = 0;

	static constexpr ImU32 TITLE_COL = IM_COL32(0, 255, 0, 255);

	ID3D12DescriptorHeap* mSrvDescHeap;
	ExampleDescriptorHeapAllocator mSrvDescHeapAlloc;

	void pushDisabled() {
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	}
	void popDisabled() {
		ImGui::PopItemFlag();
		ImGui::PopStyleVar();
	}
#pragma endregion ImGui
#pragma region Streamline
	void BuildTexture2DSrv(const std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12Resource>>& texMap, D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc, CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE& hGpuSrv, UINT descriptorSize);
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12Resource>> mStreamlineTex;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mStreamlinePSOs;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUStreamline;
	bool mSLInitialised = false;
	sl::Preferences mPref;
	UIData m_ui;
	std::string str_temp;
	
	// Microsoft::WRL::ComPtr<ID3D12RootSignature> mStreamlineRootSignature;

	struct sl_FrameBuffer
	{
		D3D12_RESOURCE_DESC GBufferDiffuseDesc;
		D3D12_RESOURCE_DESC GBufferSpecularDesc;
		D3D12_RESOURCE_DESC GBufferNormalDesc;
		D3D12_RESOURCE_DESC GBufferEmissiveDesc;
		D3D12_RESOURCE_DESC GBufferDepthDesc;
		D3D12_RESOURCE_DESC GBufferMotionVectorsDesc;
		
		GBufferFillConstants GBufferConstants;
		std::unique_ptr<UploadBuffer<GBufferFillConstants>> GBufferCB;
		Microsoft::WRL::ComPtr<ID3D12Resource> GBufferDiffuse;
		Microsoft::WRL::ComPtr<ID3D12Resource> GBufferSpecular;
		Microsoft::WRL::ComPtr<ID3D12Resource> GBufferNormal;
		Microsoft::WRL::ComPtr<ID3D12Resource> GBufferEmissive;
		Microsoft::WRL::ComPtr<ID3D12Resource> GBufferDepth;
		Microsoft::WRL::ComPtr<ID3D12Resource> GBufferMotionVectors;
	} m_sl_framebuffer;

	struct sl_data_motion_vector
	{
		TemporalAntiAliasingConstants taaConstants;
		std::unique_ptr<UploadBuffer<TemporalAntiAliasingConstants>> taaCB;
		
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pso;
		Microsoft::WRL::ComPtr<ID3D12Resource> GBufferDepth;
		D3D12_GPU_DESCRIPTOR_HANDLE handleGPU;
		Microsoft::WRL::ComPtr<ID3DBlob> VS;
		Microsoft::WRL::ComPtr<ID3DBlob> PS;
		std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
		D3D12_PRIMITIVE_TOPOLOGY primType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	} m_sl_data_mv;

	void UpdateFeatureAvailable();
	void BuildStreamlineTexture(ID3D12Device* device, donut::math::uint2 renderSize);
	std::wstring GetSlInterposerDllLocation();
	bool InitSLLog();
	bool LoadStreamline();
	void InitSLPost();
	void BuildStreamlinePSOs();
	bool SuccessCheck(sl::Result result, std::string& o_log);

	bool BeginFrame();
	void SLFrameInit();
	bool mNeedNewPasses = false;
	void SLFrameSetting();

	void UpdateGBufferCB();
	void RenderGBuffer();
	void RenderMotionVectors();

	// help functions
	inline sl::float2 make_sl_float2(donut::math::float2 donutF) { return sl::float2{ donutF.x, donutF.y }; }
	inline sl::float3 make_sl_float3(donut::math::float3 donutF) { return sl::float3{ donutF.x, donutF.y, donutF.z }; }
	inline sl::float3 make_sl_float3(DirectX::XMFLOAT3 float3) { return sl::float3{ float3.x, float3.y, float3.z }; }
	inline donut::math::float3 make_donut_float3(DirectX::XMFLOAT3 float3) { return donut::math::float3{ float3.x, float3.y, float3.z }; }
	inline sl::float4 make_sl_float4(donut::math::float4 donutF) { return sl::float4{ donutF.x, donutF.y, donutF.z, donutF.w }; }
	inline sl::float4x4 make_sl_float4x4(donut::math::float4x4 donutF4x4) {
		sl::float4x4 outF4x4;
		outF4x4.setRow(0, make_sl_float4(donutF4x4.row0));
		outF4x4.setRow(1, make_sl_float4(donutF4x4.row1));
		outF4x4.setRow(2, make_sl_float4(donutF4x4.row2));
		outF4x4.setRow(3, make_sl_float4(donutF4x4.row3));
		return outF4x4;
	}
	inline sl::float4x4 make_sl_float4x4(DirectX::XMFLOAT4X4 float4x4) {
		sl::float4x4 outF4x4;
		outF4x4.setRow(0, sl::float4{ float4x4._11,float4x4._12,float4x4._13,float4x4._14 });
		outF4x4.setRow(1, sl::float4{ float4x4._21,float4x4._22,float4x4._23,float4x4._24 });
		outF4x4.setRow(2, sl::float4{ float4x4._31,float4x4._32,float4x4._33,float4x4._34 });
		outF4x4.setRow(3, sl::float4{ float4x4._41,float4x4._42,float4x4._43,float4x4._44 });
		return outF4x4;
	}

	inline donut::math::affine3 make_donut_affine3(DirectX::XMFLOAT4X4 float4x4) {
		return donut::math::affine3(
			float4x4._11, float4x4._12, float4x4._13,
			float4x4._21, float4x4._22, float4x4._23,
			float4x4._31, float4x4._32, float4x4._33,
			0.f, 0.f, 0.f);
	}

	inline donut::math::float4x4 make_donut_float4x4(DirectX::XMFLOAT4X4 float4x4) {
		return donut::math::float4x4{
			float4x4._11,float4x4._12,float4x4._13,float4x4._14,
			float4x4._21,float4x4._22,float4x4._23,float4x4._24,
			float4x4._31,float4x4._32,float4x4._33,float4x4._34,
			float4x4._41,float4x4._42,float4x4._43,float4x4._44
		};
	}

	donut::math::float4x4 perspProjD3DStyleReverse(float verticalFOV, float aspect, float zNear)
	{
		float yScale = 1.0f / tanf(0.5f * verticalFOV);
		float xScale = yScale / aspect;

		return donut::math::float4x4(
			xScale, 0, 0, 0,
			0, yScale, 0, 0,
			0, 0, 0, 1,
			0, 0, zNear, 0);
	}

	inline void BeginMarker(ID3D12GraphicsCommandList* pCommandList, UINT64 /*metadata*/, PCSTR pFormat)
	{
		using namespace DirectX::Detail;
		UINT size = static_cast<UINT>((strlen(pFormat) + 1) * sizeof(pFormat[0]));
		pCommandList->BeginEvent(PIX_EVENT_ANSI_VERSION, pFormat, size);
	}
	inline void BeginMarker(PCSTR pFormat)
	{
		BeginMarker(mCommandList.Get(), 0, pFormat);
	}

	inline void EndMarker(ID3D12GraphicsCommandList* pCommandList)
	{
		pCommandList->EndEvent();
	}
	inline void EndMarker()
	{
		EndMarker(mCommandList.Get());
	}

	// struct & support functions
	sl::Constants m_slConstants = {};

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
	sl::DLSSOptimalSettings m_dlss_settings{};

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
	// sl::eLowLatency;

	bool m_reflex_available = false;
	sl::ReflexOptions m_reflex_consts{};
	bool m_reflex_driverFlashIndicatorEnable = false;
	bool m_pcl_available = false;
	sl::PCLOptions m_pcl_consts{};

	// For Streamline
	sl::FrameToken* m_currentFrame;
	sl::ViewportHandle m_viewport = { 0 };
	sl::Extent m_backbufferViewportExtent{};

	struct DLSSSettings
	{
		donut::math::int2 optimalRenderSize;
		donut::math::int2 minRenderSize;
		donut::math::int2 maxRenderSize;
		float sharpness;
	} m_RecommendedDLSSSettings;

	donut::math::int2 m_DLSS_Last_DisplaySize = { 0,0 };

	donut::math::int2 m_RenderingRectSize = { 0, 0 };

	donut::math::int2 m_RenderSize;// size of render targets pre-DLSS
	donut::math::int2 m_DisplaySize; // size of render targets post-DLSS

	std::default_random_engine m_Generator;
	float m_PreviousLodBias;


	sl::DLSSMode DLSS_Last_Mode = sl::DLSSMode::eOff;

	void SetSLConsts(const sl::Constants& consts);
	void FeatureLoad(sl::Feature feature, const bool turn_on);
	
	// Tag 지정 방법
	// SL_API sl::Result slSetTagForFrame(const sl::FrameToken& frame, const sl::ViewportHandle& viewport, const sl::ResourceTag* resources, uint32_t numResources, sl::CommandBuffer* cmdBuffer);
	// SL_API sl::Result slSetTag(const sl::ViewportHandle& viewport, const sl::ResourceTag* tags, uint32_t numTags, sl::CommandBuffer* cmdBuffer);
	void GetSLResource(
		ID3D12GraphicsCommandList* commandList,
		sl::Resource& slResource,
		ID3D12Resource* inputTex)
	{
		slResource = sl::Resource{ sl::ResourceType::eTex2d, inputTex, nullptr, nullptr, static_cast<uint32_t>(inputTex->GetDesc().Flags) };
	}
	
	void TagResources_General(
		ID3D12GraphicsCommandList* commandList,
		ID3D12Resource* motionVectors,
		ID3D12Resource* depth,
		ID3D12Resource* finalColorHudless);

	void TagResources_DLSS_NIS(
		ID3D12GraphicsCommandList* commandList,
		ID3D12Resource* output,
		ID3D12Resource* input);


	//void TagResources_DLSS_FG(
	//	nvrhi::ICommandList* commandList,
	//	bool validViewportExtent = false,
	//	sl::Extent backBufferExtent = {});

	//void TagResources_DeepDVC(
	//	ID3D12CommandList& commandList,
	//	const donut::engine::IView* view,
	//	nvrhi::ITexture* output);

	//void UnTagResources_DeepDVC();

	//void TagResources_Latewarp(
	//	nvrhi::ICommandList* commandList,
	//	const donut::engine::IView* view,
	//	nvrhi::ITexture* backbuffer,
	//	nvrhi::ITexture* uiColorAlpha,
	//	nvrhi::ITexture* noWarpMask,
	//	sl::Extent backBufferExtent);

	// DLSS
	void SetDLSSOptions(const sl::DLSSOptions consts);
	bool GetDLSSAvailable() { return m_dlss_available; }
	bool GetDLSSLastEnable() { return m_dlss_consts.mode != sl::DLSSMode::eOff; }
	void QueryDLSSOptimalSettings(DLSSSettings& settings);
	void EvaluateDLSS(ID3D12CommandList* commandList);
	void CleanupDLSS(bool wfi);

	// NIS
	void SetNISOptions(const sl::NISOptions consts);
	bool GetNISAvailable() { return m_nis_available; }
	bool GetNISLastEnable() { return m_nis_consts.mode != sl::NISMode::eOff; }
	void EvaluateNIS(ID3D12CommandList* commandList);
	void CleanupNIS(bool wfi);

	// DeepDVC
	void SetDeepDVCOptions(const sl::DeepDVCOptions consts);
	bool GetDeepDVCAvailable() { return m_deepdvc_available; }
	bool GetDeepDVCLastEnable() { return m_deepdvc_consts.mode != sl::DeepDVCMode::eOff; }
	void QueryDeepDVCState(uint64_t& estimatedVRamUsage);
	void EvaluateDeepDVC(ID3D12CommandList* commandList);
	void CleanupDeepDVC();

	// Reflex
	bool GetReflexAvailable() { return m_reflex_available; }
	bool GetPCLAvailable() const { return m_pcl_available; }
	static void Callback_FrameCount_Reflex_Sleep_Input_SimStart(AppBase& manager);

	void ReflexTriggerFlash();
	void ReflexTriggerPcPing();
	void QueryReflexStats(bool& reflex_lowLatencyAvailable, bool& reflex_flashAvailable, std::string& stats);
	void SetReflexFlashIndicator(bool enabled) { m_reflex_driverFlashIndicatorEnable = enabled; }
	bool GetReflexFlashIndicatorEnable() { return m_reflex_driverFlashIndicatorEnable; }

	void SetReflexConsts(const sl::ReflexOptions consts);
	void ReflexCallback_Sleep(AppBase& manager, uint32_t frameID);
	void ReflexCallback_SimStart(AppBase& manager, uint32_t frameID);
	void ReflexCallback_SimEnd(AppBase& manager, uint32_t frameID);
	void ReflexCallback_RenderStart(AppBase& manager, uint32_t frameID);
	void ReflexCallback_RenderEnd(AppBase& manager, uint32_t frameID);
	void ReflexCallback_PresentStart(AppBase& manager, uint32_t frameID);
	void ReflexCallback_PresentEnd(AppBase& manager, uint32_t frameID);
	sl::FrameToken* GetCurrentFrameToken() { return m_currentFrame; }
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

	// Latewarp
	bool GetLatewarpAvailable() { return m_latewarp_available; }
	void Set_Latewarp_SwapChainRecreation(bool on) { m_latewarp_triggerSwapchainRecreation = true; m_latewarp_shouldLoad = on; }
	bool Get_Latewarp_SwapChainRecreation(bool& turn_on) const { turn_on = m_latewarp_shouldLoad; return m_latewarp_triggerSwapchainRecreation; }
	void Quiet_Latewarp_SwapChainRecreation() { m_latewarp_triggerSwapchainRecreation = false; }
	void EvaluateLatewarp() {}

	// 
	bool IsUpdateRequired(donut::math::int2 renderSize, donut::math::int2 displaySize) const
	{
		if (any(m_RenderSize != renderSize) || any(m_DisplaySize != displaySize)) return true;
		return false;
	}

	bool SetupView();


#pragma endregion Streamline
	double mMSpFrame = 0.0;
	double mSpFrame = 0.0;
	double mFPS = 0.0;

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
	Camera mCamera;
	Camera mCameraPrevious;

	// Pipeline objects.
	Microsoft::WRL::ComPtr<IDXGIFactory4> mDxgiFactory;
	Microsoft::WRL::ComPtr<ID3D12Device> mDevice;

	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
	UINT64 mFrameCount = 0;
	HANDLE mFenceEvent = nullptr;
	int mCurrBackBuffer = 0;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

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
	Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedDataSettings> mDredSettings;
#endif

	float mMouseNdcX = 0.0f;
	float mMouseNdcY = 0.0f;
	int mMouseX = -1;
	int mMouseY = -1;

	// Const Buffer
	ShaderToyConstants mCBShaderToy;
};