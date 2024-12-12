#include "AppBase.h"
#include <iostream>
// #include <imgui.h>
// #include <imgui_impl_dx12.h>
// #include <imgui_impl_win32.h>

namespace kyun
{
    using namespace std;

    AppBase* g_appBase = nullptr;

    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        return g_appBase->MsgProc(hWnd, msg, wParam, lParam);
    }

    //===================================
    // Constructor
    //===================================
    AppBase::AppBase() : m_screenWidth(1920), m_screenHeight(1080), m_mainWindow(0)
    {
        g_appBase = this;
    }

    AppBase::~AppBase()
    {
        g_appBase = nullptr;

        DestroyWindow(m_mainWindow);
    }

    //===================================
    // Initialize
    //===================================
    bool AppBase::Initialize() {

        if (!InitMainWindow())
            return false;

        if (!InitDirect3D())
            return false;

        if (!InitGUI())
            return false;

        if (!InitScene())
            return false;

        // 콘솔창이 렌더링 창을 덮는 것을 방지
        SetForegroundWindow(m_mainWindow);

        return true;
    }


    bool AppBase::InitMainWindow() {

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
                         L"Park Seung Kyun Template", // lpszClassName, L-string
                         NULL };

        if (!RegisterClassEx(&wc)) {
            cout << "RegisterClassEx() failed." << endl;
            return false;
        }

        RECT wr = { 0, 0, m_screenWidth, m_screenHeight };
        AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false);
        m_mainWindow = CreateWindow(wc.lpszClassName, L"Park Seung Kyun Template",
            WS_OVERLAPPEDWINDOW,
            100, // 윈도우 좌측 상단의 x 좌표
            100, // 윈도우 좌측 상단의 y 좌표
            wr.right - wr.left, // 윈도우 가로 방향 해상도
            wr.bottom - wr.top, // 윈도우 세로 방향 해상도
            NULL, NULL, wc.hInstance, NULL);

        if (!m_mainWindow) {
            cout << "CreateWindow() failed." << endl;
            return false;
        }

        ShowWindow(m_mainWindow, SW_SHOWDEFAULT);
        UpdateWindow(m_mainWindow);

        return true;
    }

    bool AppBase::InitDirect3D() {

//        const D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;
//        // const D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_WARP;
//
//        UINT createDeviceFlags = 0;
//#if defined(DEBUG) || defined(_DEBUG)
//        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
//#endif
//
//        const D3D_FEATURE_LEVEL featureLevels[2] = {
//            D3D_FEATURE_LEVEL_11_0, // 더 높은 버전이 먼저 오도록 설정
//            D3D_FEATURE_LEVEL_9_3 };
//        D3D_FEATURE_LEVEL featureLevel;
//
//        DXGI_SWAP_CHAIN_DESC sd;
//        ZeroMemory(&sd, sizeof(sd));
//        sd.BufferDesc.Width = m_screenWidth;
//        sd.BufferDesc.Height = m_screenHeight;
//        sd.BufferDesc.Format = m_backBufferFormat;
//        sd.BufferCount = 2;
//        sd.BufferDesc.RefreshRate.Numerator = 60;
//        sd.BufferDesc.RefreshRate.Denominator = 1;
//        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT |
//            DXGI_USAGE_UNORDERED_ACCESS; // Compute Shader
//        sd.OutputWindow = m_mainWindow;
//        sd.Windowed = TRUE;
//        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
//        // sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; //ImGui 폰트가
//        // 두꺼워짐
//        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
//        sd.SampleDesc.Count = 1; // _FLIP_은 MSAA 미지원
//        sd.SampleDesc.Quality = 0;
//
//        ThrowIfFailed(D3D11CreateDeviceAndSwapChain(
//            0, driverType, 0, createDeviceFlags, featureLevels, 1,
//            D3D11_SDK_VERSION, &sd, m_swapChain.GetAddressOf(),
//            m_device.GetAddressOf(), &featureLevel, m_context.GetAddressOf()));
//
//        if (featureLevel != D3D_FEATURE_LEVEL_11_0) {
//            cout << "D3D Feature Level 11 unsupported." << endl;
//            return false;
//        }
//
//        Graphics::InitCommonStates(m_device);
//
//        CreateBuffers();
//
//        SetMainViewport();
//
//        // 공통으로 쓰이는 ConstBuffers
//        D3D11Utils::CreateConstBuffer(m_device, m_globalConstsCPU,
//            m_globalConstsGPU);
//        D3D11Utils::CreateConstBuffer(m_device, m_reflectGlobalConstsCPU,
//            m_reflectGlobalConstsGPU);
//
//        // 그림자맵 렌더링할 때 사용할 GlobalConsts들 별도 생성
//        for (int i = 0; i < MAX_LIGHTS; i++) {
//            D3D11Utils::CreateConstBuffer(m_device, m_shadowGlobalConstsCPU[i],
//                m_shadowGlobalConstsGPU[i]);
//        }
//
//        // 후처리 효과용 ConstBuffer
//        D3D11Utils::CreateConstBuffer(m_device, m_postEffectsConstsCPU,
//            m_postEffectsConstsGPU);

        return true;
    }

    bool AppBase::InitGUI() {

        //IMGUI_CHECKVERSION();
        //ImGui::CreateContext();
        //ImGuiIO& io = ImGui::GetIO();
        //(void)io;
        //io.DisplaySize = ImVec2(float(m_screenWidth), float(m_screenHeight));
        //ImGui::StyleColorsLight();

        //// Setup Platform/Renderer backends
        //if (!ImGui_ImplDX11_Init(m_device.Get(), m_context.Get())) {
        //    return false;
        //}

        //if (!ImGui_ImplWin32_Init(m_mainWindow)) {
        //    return false;
        //}

        return true;
    }
    bool AppBase::InitScene() {
        return true;
    }


    //===================================
    // Run
    //===================================
    int AppBase::Run()
    {
        while (true)
        {

        }
        return 0;
    }


    LRESULT AppBase::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

        //if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        //    return true;

        //switch (msg) {
        //case WM_SIZE:
        //    // 화면 해상도가 바뀌면 SwapChain을 다시 생성
        //    if (m_swapChain) {

        //        m_screenWidth = int(LOWORD(lParam));
        //        m_screenHeight = int(HIWORD(lParam));

        //        // 윈도우가 Minimize 모드에서는 screenWidth/Height가 0
        //        if (m_screenWidth && m_screenHeight) {

        //            cout << "Resize SwapChain to " << m_screenWidth << " "
        //                << m_screenHeight << endl;

        //            m_backBufferRTV.Reset();
        //            m_swapChain->ResizeBuffers(
        //                0,                    // 현재 개수 유지
        //                (UINT)LOWORD(lParam), // 해상도 변경
        //                (UINT)HIWORD(lParam),
        //                DXGI_FORMAT_UNKNOWN, // 현재 포맷 유지
        //                0);
        //            CreateBuffers();
        //            SetMainViewport();
        //            m_camera.SetAspectRatio(this->GetAspectRatio());

        //            m_postProcess.Initialize(
        //                m_device, m_context, { m_postEffectsSRV, m_prevSRV },
        //                { m_backBufferRTV }, m_screenWidth, m_screenHeight, 4);
        //        }
        //    }
        //    break;
        //case WM_SYSCOMMAND:
        //    if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
        //        return 0;
        //    break;
        //case WM_MOUSEMOVE:
        //    OnMouseMove(LOWORD(lParam), HIWORD(lParam));
        //    break;
        //case WM_LBUTTONDOWN:
        //    if (!m_leftButton) {
        //        m_dragStartFlag = true; // 드래그를 새로 시작하는지 확인
        //    }
        //    m_leftButton = true;
        //    OnMouseClick(LOWORD(lParam), HIWORD(lParam));
        //    break;
        //case WM_LBUTTONUP:
        //    m_leftButton = false;
        //    break;
        //case WM_RBUTTONDOWN:
        //    if (!m_rightButton) {
        //        m_dragStartFlag = true; // 드래그를 새로 시작하는지 확인
        //    }
        //    m_rightButton = true;
        //    break;
        //case WM_RBUTTONUP:
        //    m_rightButton = false;
        //    break;
        //case WM_KEYDOWN:
        //    m_keyPressed[wParam] = true;
        //    if (wParam == VK_ESCAPE) { // ESC키 종료
        //        DestroyWindow(hwnd);
        //    }
        //    if (wParam == VK_SPACE) {
        //        m_lightRotate = !m_lightRotate;
        //    }
        //    break;
        //case WM_KEYUP:
        //    if (wParam == 'F') { // f키 일인칭 시점
        //        m_camera.m_useFirstPersonView = !m_camera.m_useFirstPersonView;
        //    }
        //    if (wParam == 'C') { // c키 화면 캡쳐
        //        ComPtr<ID3D11Texture2D> backBuffer;
        //        m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
        //        D3D11Utils::WriteToPngFile(m_device, m_context, backBuffer,
        //            "captured.png");
        //    }
        //    if (wParam == 'P') { // 애니메이션 일시중지할 때 사용
        //        m_pauseAnimation = !m_pauseAnimation;
        //    }
        //    if (wParam == 'Z') { // 카메라 설정 화면에 출력
        //        m_camera.PrintView();
        //    }

        //    m_keyPressed[wParam] = false;
        //    break;
        //case WM_MOUSEWHEEL:
        //    m_wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        //    break;
        //case WM_DESTROY:
        //    ::PostQuitMessage(0);
        //    return 0;
        //}

        return ::DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

