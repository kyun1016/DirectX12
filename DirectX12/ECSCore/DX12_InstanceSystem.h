#pragma once
#include "ECSCoordinator.h"
#include "DX12_InstanceComponent.h"

class DX12_InstanceSystem : public ECS::ISystem {
public:
    DX12_InstanceSystem()
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
        auto& coordinator = ECS::Coordinator::GetInstance();
        for (ECS::Entity entity : mEntities) {
            auto& instance = coordinator.GetComponent<InstanceData>(entity);
        }
    }
private:
    std::unique_ptr<UploadBuffer<InstanceData>> InstanceBuffer = nullptr;
};