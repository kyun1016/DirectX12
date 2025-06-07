//#pragma once
//#include "ECSCoordinator.h"
//#include "WindowComponent.h"
//#include "ImGuiComponent.h"
//
//#include "../ImGuiCore/imgui.h"
//#include "../ImGuiCore/imgui_impl_dx12.h"
//
//
//class ImGuiSystem : public ECS::ISystem
//{
//public:
//    void BeginPlay() override {
//		IMGUI_CHECKVERSION();
//		ImGui::CreateContext();
//		ImGuiIO& imGuiIO = ImGui::GetIO();
//		imGuiIO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
//		imGuiIO.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
//		imGuiIO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
//		imGuiIO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
//
//		// Setup Dear ImGui style
//		ImGui::StyleColorsDark();
//		//ImGui::StyleColorsLight();
//
//		// Setup Platform/Renderer backends
//		ImGui_ImplWin32_Init(mHwndWindow);
//
//		ImGui_ImplDX12_InitInfo init_info = {};
//		init_info.Device = mDevice.Get();
//		init_info.CommandQueue = mCommandQueue.Get();
//		init_info.NumFramesInFlight = APP_NUM_FRAME_RESOURCES;
//		init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
//		init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
//		// Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
//		// (current version of the backend will only allocate one descriptor, future versions will need to allocate more)
//		//mSrvDescHeapAlloc.Create(mDevice.Get(), mSrvDescriptorHeap.Get());
//		//init_info.SrvDescriptorHeap = mSrvDescriptorHeap.Get();
//		//init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return g_appBase->mSrvDescHeapAlloc.Alloc(out_cpu_handle, out_gpu_handle); };
//		//init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return g_appBase->mSrvDescHeapAlloc.Free(cpu_handle, gpu_handle); };
//		ImGui_ImplDX12_Init(&init_info);
//
//		// Load Fonts
//		// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
//		// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
//		// - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
//		// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
//		// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
//		// - Read 'docs/FONTS.md' for more instructions and details.
//		// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
//		//io.Fonts->AddFontDefault();
//		//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
//		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
//		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
//		//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
//		//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
//		//IM_ASSERT(font != nullptr);
//
//		return true;
//    }
//
//    void Sync() override {
//        // 윈도우 상태 갱신
//        auto& windowComponent = ECS::Coordinator::GetInstance().GetSingletonComponent<WindowComponent>();
//        //RECT rect;
//        //GetClientRect(mHwnd, &rect);
//  //      windowComponent.left = rect.left;
//  //      windowComponent.top = rect.top;
//  //      windowComponent.right = rect.right;
//  //      windowComponent.bottom = rect.bottom;
//
//        // 메시지 처리 루프
//        MSG msg = {};
//        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
//            TranslateMessage(&msg);
//            DispatchMessage(&msg);
//        }
//        // 필요시 윈도우 상태 갱신
//    }
//    void Update() override {
//
//    }
//
//    void EndPlay() override {
//        auto& windowComponent = ECS::Coordinator::GetInstance().GetSingletonComponent<WindowComponent>();
//        if (mHwnd) {
//            DestroyWindow(mHwnd);
//            mHwnd = nullptr;
//        }
//    }
//
//private:
//    WNDCLASSEXW mWindowClass;
//    HWND mHwnd = nullptr;
//    std::wstring mCaption = L"App";
//};
