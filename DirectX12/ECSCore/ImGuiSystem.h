#pragma once
#include "ECSCoordinator.h"
#include "DX12_Config.h"
#include "DX12_SwapChainSystem.h"

#include "../ImGuiCore/imgui.h"
#include "../ImGuiCore/imgui_impl_win32.h"
#include "../ImGuiCore/imgui_impl_dx12.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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

enum class eImguiWindows : std::uint64_t
{
	None = 0,
	MainWindow = 1 << 0,
	DemoWindow = 1 << 1,
	AnotherWindow = 1 << 2,
	Viewport1 = 1 << 2,
	Viewport2 = 1 << 3
};
ENUM_OPERATORS_64(eImguiWindows)

class ImGuiSystem
{
public:
	inline static ImGuiSystem& GetInstance() {
		static ImGuiSystem instance; return instance;
	}

	void Initialize(const HWND& hwnd, ID3D12Device* device, ID3D12CommandQueue* commandQueue, ID3D12GraphicsCommandList6* commandList)
	{
		CreateDescriptorHeap(device);
		mCommandList = commandList;

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
		ImGui_ImplWin32_Init(hwnd);


		ImGui_ImplDX12_InitInfo init_info = {};
		init_info.Device = device;
		init_info.CommandQueue = commandQueue;
		init_info.NumFramesInFlight = APP_NUM_BACK_BUFFERS;
		init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
		// Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
		// (current version of the backend will only allocate one descriptor, future versions will need to allocate more)
		mSrvDescHeapAlloc.Create(device, mHeap.Get());
		init_info.SrvDescriptorHeap = mHeap.Get();
		init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return ImGuiSystem::GetInstance().mSrvDescHeapAlloc.Alloc(out_cpu_handle, out_gpu_handle); };
		init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return ImGuiSystem::GetInstance().mSrvDescHeapAlloc.Free(cpu_handle, gpu_handle); };
		ImGui_ImplDX12_Init(&init_info);
	}

	void Render()
	{
		Update();
		ImGui::Render();

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = DX12_SwapChainSystem::GetInstance().GetBackBuffer();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		mCommandList->ResourceBarrier(1, &barrier);

		ID3D12DescriptorHeap* descriptorHeaps[] = { mHeap.Get() };
		mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList);

		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		mCommandList->ResourceBarrier(1, &barrier);
	}

	void RenderMultiViewport()
	{
		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	ExampleDescriptorHeapAllocator mSrvDescHeapAlloc;
private:
	static constexpr int SRV_IMGUI_SIZE = 64;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mHeap;
	ID3D12GraphicsCommandList6* mCommandList = nullptr;
	bool showMainWindow = true;
	bool showDemoWindow = true;
	bool showInstanceWindow = true;


	void CreateDescriptorHeap(ID3D12Device* device)
	{
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc
		{
			/* D3D12_DESCRIPTOR_HEAP_TYPE Type	*/.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			/* UINT NumDescriptors				*/.NumDescriptors = SRV_IMGUI_SIZE,
			/* D3D12_DESCRIPTOR_HEAP_FLAGS Flags*/.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			/* UINT NodeMask					*/.NodeMask = 0
		};
		ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mHeap)));
	}

	void Update()
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		if (showMainWindow)     ShowMainWindow(&showMainWindow);
		if (showDemoWindow)     ImGui::ShowDemoWindow(&showDemoWindow);
		if (showInstanceWindow) ShowInstanceWindow(&showInstanceWindow);
	}

	void ShowMainWindow(bool* p_open)
	{
		ImGuiWindowFlags window_flags = 0;
		// if (no_titlebar)        window_flags |= ImGuiWindowFlags_NoTitleBar;
		// if (no_scrollbar)       window_flags |= ImGuiWindowFlags_NoScrollbar;
		// if (!no_menu)           window_flags |= ImGuiWindowFlags_MenuBar;
		// if (no_move)            window_flags |= ImGuiWindowFlags_NoMove;
		// if (no_resize)          window_flags |= ImGuiWindowFlags_NoResize;
		// if (no_collapse)        window_flags |= ImGuiWindowFlags_NoCollapse;
		// if (no_nav)             window_flags |= ImGuiWindowFlags_NoNav;
		// if (no_background)      window_flags |= ImGuiWindowFlags_NoBackground;
		// if (no_bring_to_front)  window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
		// if (no_docking)         window_flags |= ImGuiWindowFlags_NoDocking;
		// if (unsaved_document)   window_flags |= ImGuiWindowFlags_UnsavedDocument;
		// if (no_close)           p_open = NULL; // Don't pass our bool* to Begin

		// Main body of the Demo window starts here.
		if (!ImGui::Begin("Main Window", p_open, window_flags))
		{
			// Early out if the window is collapsed, as an optimization.
			ImGui::End();
			return;
		}
		ImGui::Text("Hello, ImGui!");
		ImGui::Checkbox("Demo Window", &showDemoWindow);      // Edit bools storing our window open/close state
		ImGui::Checkbox("Instance Window", &showInstanceWindow);      // Edit bools storing our window open/close state
		ImGui::End();
	}

	void ShowInstanceWindow(bool* p_open)
	{
		ImGuiWindowFlags window_flags = 0;
		if (!ImGui::Begin("Instance Window", p_open, window_flags))
		{
			ImGui::End();
			return;
		}
		ImGui::Text("Hello, Instance!");
		auto& renderItem = DX12_SceneSystem::GetInstance().GetRenderItems();
		float3 w_Position;
		float3 w_Scale;
		float3 w_RotationEuler;
		float4 w_RotationQuat;

		float3 r_Position;
		float3 r_Scale;
		float3 r_RotationEuler;
		float4 r_RotationQuat;


		ImGui::End();
	}

	ImGuiSystem() = default;
	~ImGuiSystem()
	{
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	};
	ImGuiSystem(const ImGuiSystem&) = delete;
	ImGuiSystem& operator=(const ImGuiSystem&) = delete;
	ImGuiSystem(ImGuiSystem&&) = delete;
	ImGuiSystem& operator=(ImGuiSystem&&) = delete;
};
