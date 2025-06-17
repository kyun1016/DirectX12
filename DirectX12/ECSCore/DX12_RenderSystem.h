#pragma once
#include "ECSCoordinator.h"

#include "DX12_Config.h"
#include "DX12_DeviceSystem.h"
#include "DX12_CommandSystem.h"
#include "DX12_RTVHeapRepository.h"
#include "DX12_DSVHeapRepository.h"
#include "DX12_SwapChainSystem.h"
#include "DX12_RootSignatureSystem.h"
#include "DX12_InputLayoutSystem.h"
#include "DX12_ShaderCompileSystem.h"
#include "DX12_PSOSystem.h"
#include "WindowSystem.h"


class DX12_RenderSystem : public ECS::ISystem {
public:
    DX12_RenderSystem()
    {
        Initialize();
        // cmdList->SetPipelineState(pipelineState.Get());
        // 
        // D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
        // D3D12_RECT scissorRect = { 0, 0, width, height };
        // cmdList->RSSetViewports(1, &viewport);
        // cmdList->RSSetScissorRects(1, &scissorRect);
        // 
        // cmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
        // cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        // cmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        // 
        // cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        // cmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
        // // Index Buffer 사용 시
        // // cmdList->IASetIndexBuffer(&indexBufferView);
        // 
        // cmdList->SetGraphicsRootConstantBufferView(0, cbGpuAddress);
        // // 또는
        // // cmdList->SetDescriptorHeaps(...);
        // // cmdList->SetGraphicsRootDescriptorTable(...);
        // 
        // cmdList->DrawInstanced(vertexCount, 1, 0, 0);
        // // 또는
        // // cmdList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
    }
    void Update() override {
		mCommandAllocator->Reset();
		mCommandList->Reset(mCommandAllocator, nullptr);

        mCommandList->SetGraphicsRootSignature(DX12_RootSignatureSystem::GetInstance().GetGraphicsSignature("test"));
		mCommandList->RSSetViewports(1, &DX12_SwapChainSystem::GetInstance().GetViewport());
		mCommandList->RSSetScissorRects(1, &DX12_SwapChainSystem::GetInstance().GetScissorRect());
        // mCommandList->OMSetRenderTargets(1, &DX12_RTVHeapRepository::GetInstance().GetHandleByIndex(0), FALSE, &DX12_DSVHeapRepository::GetInstance().GetHandleByIndex(0));
		mCommandList->OMSetRenderTargets(1, &DX12_SwapChainSystem::GetInstance().GetBackBufferDescriptorHandle(), FALSE, nullptr);


        auto& coordinator = ECS::Coordinator::GetInstance();
        for (ECS::Entity entity : mEntities) {

        }
    }
private:
    ID3D12Device* mDevice;
    ID3D12CommandQueue* mCommandQueue;
	ID3D12CommandAllocator* mCommandAllocator;
    ID3D12GraphicsCommandList6* mCommandList;
    D3D12_VIEWPORT mScreenViewport;
    D3D12_RECT mScissorRect;

    inline void Initialize() {
        // auto& coordinator = ECS::Coordinator::GetInstance();
        // auto deviceSystem = coordinator.RegisterSystem<DX12_DeviceSystem>();
        // mDevice = deviceSystem->GetDevice();
        WindowComponent& wc = ECS::Coordinator::GetInstance().GetSingletonComponent<WindowComponent>();

        DX12_DeviceSystem::GetInstance().Initialize();
		mDevice = DX12_DeviceSystem::GetInstance().GetDevice();
        DX12_CommandSystem::GetInstance().Initialize(mDevice); // 시스템 간 공유가 필요 없는 로직은 DX12_Core에서 처리하는 방식으로 구현
		mCommandQueue = DX12_CommandSystem::GetInstance().GetCommandQueue();
		mCommandAllocator = DX12_CommandSystem::GetInstance().GetCommandAllocator();
        mCommandList = DX12_CommandSystem::GetInstance().GetCommandList();
        DX12_RTVHeapRepository::GetInstance().Initialize(mDevice);
        DX12_DSVHeapRepository::GetInstance().Initialize(mDevice);
        DX12_SwapChainSystem::GetInstance().Initialize(mDevice, DX12_DeviceSystem::GetInstance().GetFactory(), mCommandQueue, wc.hwnd, wc.width, wc.height);
        DX12_RootSignatureSystem::GetInstance().Initialize(mDevice);

        DX12_InputLayoutSystem::GetInstance().Initialize();
        DX12_ShaderCompileSystem::GetInstance().Initialize();

        DX12_PSOSystem::GetInstance().Initialize(mDevice);
        // Heap에 Texture 관련 데이터 업로드 공간 초기화
        // Frame 관련 데이터 데이터 업로드 공간 초기화
        // PSO 설정 초기화
    }
};