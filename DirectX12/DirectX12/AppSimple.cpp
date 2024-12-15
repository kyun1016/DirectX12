#include "AppSimple.h"
#include "AppBaseHelper.h"
#include "Camera.h"
#include "StepTimer.h"

namespace kyun
{
	AppSimple::AppSimple() : AppSimple(1080, 720, L"AppSimple")	{	}
	AppSimple::AppSimple(uint32_t width, uint32_t height, std::wstring name)
		: AppBase(width, height, name)
        , m_rtvDescriptorSize(0)
        , m_dsvDescriptorSize(0)
        , m_constantBufferData{}
        , m_cbvDataBegin(nullptr)
        , m_frameIndex(0)
        , m_frameCounter(0)
        , m_fenceEvent{}
        , m_fenceValues{}
	{
        m_timer = make_unique<StepTimer>();
        m_camera = make_unique<Camera>();
	}
	void AppSimple::OnInit()
	{
        InitMainWindow();
		LoadPipeline();
		LoadAssets();
		LoadSizeDependentResources();
	}
    bool AppSimple::InitMainWindow()
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
    bool AppSimple::InitCamera()
    {
        
        return false;
    }
    void AppSimple::LoadPipeline()
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

        if (m_useWarpDevice)
        {
            ComPtr<IDXGIAdapter> warpAdapter;
            ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

            ThrowIfFailed(D3D12CreateDevice(
                warpAdapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(&m_device)
            ));
        }
        else
        {
            ComPtr<IDXGIAdapter1> hardwareAdapter;
            GetHardwareAdapter(factory.Get(), &hardwareAdapter, true);

            ThrowIfFailed(D3D12CreateDevice(
                hardwareAdapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(&m_device)
            ));
        }
        // NAME_D3D12_OBJECT(m_device);

        D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_5 };
        if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
            || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_5))
        {
            OutputDebugStringA("ERROR: Shader Model 6.5 is not supported\n");
            throw std::exception("Shader Model 6.5 is not supported");
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS7 features = {};
        if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &features, sizeof(features)))
            || (features.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED))
        {
            OutputDebugStringA("ERROR: Mesh Shaders aren't supported!\n");
            throw std::exception("Mesh Shaders aren't supported!");
        }

        // Describe and create the command queue.
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
        // NAME_D3D12_OBJECT(m_commandQueue);

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
        ThrowIfFailed(factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER));

        ThrowIfFailed(swapChain.As(&m_swapChain));
        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

        // Create descriptor heaps.
        {
            // Describe and create a render target view (RTV) descriptor heap.
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
            rtvHeapDesc.NumDescriptors = FrameCount;
            rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

            m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

            // Describe and create a render target view (RTV) descriptor heap.
            D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
            dsvHeapDesc.NumDescriptors = 1;
            dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

            m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        }

        // Create frame resources.
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

            // Create a RTV and a command allocator for each frame.
            for (UINT n = 0; n < FrameCount; n++)
            {
                ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
                m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
                rtvHandle.Offset(1, m_rtvDescriptorSize);

                ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[n])));
            }
        }

        // Create the depth stencil view.
        {
            D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
            depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
            depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

            D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
            depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
            depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
            depthOptimizedClearValue.DepthStencil.Stencil = 0;

            const CD3DX12_HEAP_PROPERTIES depthStencilHeapProps(D3D12_HEAP_TYPE_DEFAULT);
            const CD3DX12_RESOURCE_DESC depthStencilTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_width, m_height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

            ThrowIfFailed(m_device->CreateCommittedResource(
                &depthStencilHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &depthStencilTextureDesc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &depthOptimizedClearValue,
                IID_PPV_ARGS(&m_depthStencil)
            ));

            NAME_D3D12_OBJECT(m_depthStencil);

            m_device->CreateDepthStencilView(m_depthStencil.Get(), &depthStencilDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
        }

        // Create the constant buffer.
        {
            const UINT64 constantBufferSize = sizeof(SceneConstantBuffer) * FrameCount;

            const CD3DX12_HEAP_PROPERTIES constantBufferHeapProps(D3D12_HEAP_TYPE_UPLOAD);
            const CD3DX12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);

            ThrowIfFailed(m_device->CreateCommittedResource(
                &constantBufferHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &constantBufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_constantBuffer)));

            // Describe and create a constant buffer view.
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
            cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
            cbvDesc.SizeInBytes = constantBufferSize;

            // Map and initialize the constant buffer. We don't unmap this until the
            // app closes. Keeping things mapped for the lifetime of the resource is okay.
            CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
            ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_cbvDataBegin)));
        }
	}
	void AppSimple::LoadAssets()
	{
        // Create the pipeline state, which includes compiling and loading shaders.
        {
            struct
            {
                u_char* data;
                uint32_t size;
            } meshShader, pixelShader;

            ReadDataFromFile(GetAssetFullPath(c_meshShaderFilename).c_str(), &meshShader.data, &meshShader.size);
            ReadDataFromFile(GetAssetFullPath(c_pixelShaderFilename).c_str(), &pixelShader.data, &pixelShader.size);

            // Pull root signature from the precompiled mesh shader.
            ThrowIfFailed(m_device->CreateRootSignature(0, meshShader.data, meshShader.size, IID_PPV_ARGS(&m_rootSignature)));

            D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.pRootSignature = m_rootSignature.Get();
            psoDesc.MS = { meshShader.data, meshShader.size };
            psoDesc.PS = { pixelShader.data, pixelShader.size };
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = m_renderTargets[0]->GetDesc().Format;
            psoDesc.DSVFormat = m_depthStencil->GetDesc().Format;
            psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);    // CW front; cull back
            psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);         // Opaque
            psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // Less-equal depth test w/ writes; no stencil
            psoDesc.SampleMask = UINT_MAX;
            psoDesc.SampleDesc = DefaultSampleDesc();

            auto psoStream = CD3DX12_PIPELINE_MESH_STATE_STREAM(psoDesc);

            D3D12_PIPELINE_STATE_STREAM_DESC streamDesc;
            streamDesc.pPipelineStateSubobjectStream = &psoStream;
            streamDesc.SizeInBytes = sizeof(psoStream);

            ThrowIfFailed(m_device->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&m_pipelineState)));
        }

        // Create the command list.
        ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[m_frameIndex].Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));

        // Command lists are created in the recording state, but there is nothing
        // to record yet. The main loop expects it to be closed, so close it now.
        ThrowIfFailed(m_commandList->Close());

        m_model.LoadFromFile(c_meshFilename);
        m_model.UploadGpuResources(m_device.Get(), m_commandQueue.Get(), m_commandAllocators[m_frameIndex].Get(), m_commandList.Get());

#ifdef _DEBUG
        // Mesh shader file expects a certain vertex layout; assert our mesh conforms to that layout.
        const D3D12_INPUT_ELEMENT_DESC c_elementDescs[2] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 1 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 1 },
        };

        for (auto& mesh : m_model)
        {
            assert(mesh.LayoutDesc.NumElements == 2);

            for (uint32_t i = 0; i < _countof(c_elementDescs); ++i)
                assert(std::memcmp(&mesh.LayoutElems[i], &c_elementDescs[i], sizeof(D3D12_INPUT_ELEMENT_DESC)) == 0);
        }
#endif

        // Create synchronization objects and wait until assets have been uploaded to the GPU.
        {
            ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
            m_fenceValues[m_frameIndex]++;

            // Create an event handle to use for frame synchronization.
            m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (m_fenceEvent == nullptr)
            {
                ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
            }

            // Wait for the command list to execute; we are reusing the same command 
            // list in our main loop but for now, we just want to wait for setup to 
            // complete before continuing.
            WaitForGpu();
        }
	}
	void AppSimple::LoadSizeDependentResources()
	{
	}
}