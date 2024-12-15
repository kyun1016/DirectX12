#include "AppShading.h"
#include "AppBaseHelper.h"

namespace kyun
{
	AppShading::AppShading() : AppShading(1080, 720, L"AppShading")	{	}
	AppShading::AppShading(uint32_t width, uint32_t height, std::wstring name)
		: AppBase(width, height, name)
	{
	}
	void AppShading::OnInit()
	{
        InitMainWindow();
		LoadPipeline();
		LoadAssets();
		LoadSizeDependentResources();
	}
    bool AppShading::InitMainWindow()
    {
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

        RECT wr = { 0, 0, m_width, m_height };
        AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false);
        m_hwnd = CreateWindow(wc.lpszClassName, L"Park Seung Kyun Template",
            WS_OVERLAPPEDWINDOW,
            100, // 윈도우 좌측 상단의 x 좌표
            100, // 윈도우 좌측 상단의 y 좌표
            wr.right - wr.left, // 윈도우 가로 방향 해상도
            wr.bottom - wr.top, // 윈도우 세로 방향 해상도
            NULL, NULL, wc.hInstance, NULL);

        if (!m_hwnd) {
            cout << "CreateWindow() failed." << endl;
            return false;
        }

        ShowWindow(m_hwnd, SW_SHOWDEFAULT);
        UpdateWindow(m_hwnd);

        return true;
    }
    void AppShading::LoadPipeline()
	{
        UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
        // Enable the debug layer (requires the Graphics Tools "optional feature").
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        {
            ComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                debugController->EnableDebugLayer();

                // Enable additional debug layers.
                dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
            }
        }
#endif

        ComPtr<IDXGIFactory4> factory;
        ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

        ComPtr<IDXGIAdapter1> hardwareAdapter;
        GetHardwareAdapter(factory.Get(), &hardwareAdapter);

        ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)
        ));
        NAME_D3D12_OBJECT(m_device);

        // Describe and create the command queue.
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
        NAME_D3D12_OBJECT(m_commandQueue);

        // Describe and create the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = FrameCount;
        swapChainDesc.Width = m_width;
        swapChainDesc.Height = m_height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;

        // It is recommended to always use the tearing flag when it is available.
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        ComPtr<IDXGISwapChain1> swapChain;
        // DXGI does not allow creating a swapchain targeting a window which has fullscreen styles(no border + topmost).
        // Temporarily remove the topmost property for creating the swapchain.
        if (m_fullscreenMode)
        {
            SetWindowZorderToTopMost(false);
        }
        ThrowIfFailed(factory->CreateSwapChainForHwnd(
            m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
            m_hwnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain
        ));

        if (m_fullscreenMode)
        {
            SetWindowZorderToTopMost(true);
        }

        // With tearing support enabled we will handle ALT+Enter key presses in the
        // window message loop rather than let DXGI handle it by calling SetFullscreenState.
        factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER);

        ThrowIfFailed(swapChain.As(&m_swapChain));
        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

        // Create synchronization objects.
        {
            ThrowIfFailed(m_device->CreateFence(m_fenceValues[m_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
            m_fenceValues[m_frameIndex]++;

            // Create an event handle to use for frame synchronization.
            m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (m_fenceEvent == nullptr)
            {
                ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
            }
        }
	}
	void AppShading::LoadAssets()
	{
	}
	void AppShading::LoadSizeDependentResources()
	{
	}
}