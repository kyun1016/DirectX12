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
#include "DX12_MeshSystem.h"
#include "DX12_FrameResourceSystem.h"
#include "WindowSystem.h"
#include "TimeSystem.h"
#include "ImGuiSystem.h"

class DX12_RenderSystem : public ECS::ISystem {
public:
	DX12_RenderSystem()
	{
		Initialize();
	}

	virtual void Sync() override {
		DX12_FrameResourceSystem::GetInstance().BeginFrame();
	}

	virtual void Update() override {
		BeginRenderPass();
		DrawRenderItems(1, eRenderLayer::Opaque);
		DrawSprites();
		EndRenderPass();
		ImGuiSystem::GetInstance().Render();
		DX12_CommandSystem::GetInstance().EndAndExecuteCommandList();
		DX12_SwapChainSystem::GetInstance().Present(true);
		ImGuiSystem::GetInstance().RenderMultiViewport();
		DX12_FrameResourceSystem::GetInstance().EndFrame();
	}
private:
	ID3D12Device* mDevice;
	ID3D12GraphicsCommandList6* mCommandList;
	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	inline void Initialize() {
		WindowComponent& wc = ECS::Coordinator::GetInstance().GetSingletonComponent<WindowComponent>();

		DX12_DeviceSystem::GetInstance().Initialize();
		mDevice = DX12_DeviceSystem::GetInstance().GetDevice();
		DX12_CommandSystem::GetInstance().Initialize(mDevice);
		mCommandList = DX12_CommandSystem::GetInstance().GetCommandList();
		DX12_RTVHeapRepository::GetInstance().Initialize(mDevice);
		DX12_DSVHeapRepository::GetInstance().Initialize(mDevice);
		DX12_SwapChainSystem::GetInstance().Initialize(mDevice, DX12_CommandSystem::GetInstance().GetCommandQueue(), DX12_DeviceSystem::GetInstance().GetFactory(), wc.hwnd, wc.width, wc.height);
		DX12_RootSignatureSystem::GetInstance().Initialize(mDevice);
		DX12_InputLayoutSystem::GetInstance().Initialize();
		DX12_ShaderCompileSystem::GetInstance().Initialize();
		DX12_PSOSystem::GetInstance().Initialize(mDevice);
		DX12_FrameResourceSystem::GetInstance().Initialize(mDevice);
		
		DX12_CommandSystem::GetInstance().BeginCommandList();
		DX12_MeshSystem::GetInstance().Initialize();
		ImGuiSystem::GetInstance().Initialize(wc.hwnd, mDevice, DX12_CommandSystem::GetInstance().GetCommandQueue(), mCommandList);

		DX12_CommandSystem::GetInstance().EndAndExecuteCommandList();
		DX12_CommandSystem::GetInstance().FlushCommandQueue();
	}

	inline void BeginRenderPass() {
		DX12_CommandSystem::GetInstance().SetViewportAndScissor(
			DX12_SwapChainSystem::GetInstance().GetViewport(),
			DX12_SwapChainSystem::GetInstance().GetScissorRect());
		D3D12_RESOURCE_BARRIER RenderBarrier = CD3DX12_RESOURCE_BARRIER::Transition(DX12_SwapChainSystem::GetInstance().GetBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		mCommandList->ResourceBarrier(1, &RenderBarrier);

		const auto& time = ECS::Coordinator::GetInstance().GetSingletonComponent<TimeComponent>();
		// float r = 0.5f;
		// float g = 0.5f;
		// float b = 0.5f;
		float r = std::fmod(time.totalTime * time.fixedDeltaTime, 1.0f); // Example: Use time to create a dynamic color
		float g = std::fmod(time.totalTime + time.totalTime, 1.0f); // Example: Use time to create a dynamic color
		float b = std::fmod(time.totalTime + time.totalTime + time.totalTime, 1.0f); // Example: Use time to create a dynamic color
		float4 FogColor = { r, g, b, 1.0f };
		mCommandList->ClearRenderTargetView(DX12_SwapChainSystem::GetInstance().GetBackBufferDescriptorHandle(), (float*)&FogColor, 0, nullptr);
		mCommandList->OMSetRenderTargets(1, &DX12_SwapChainSystem::GetInstance().GetBackBufferDescriptorHandle(), false, nullptr);
	}

	inline void DrawRenderItems(const ECS::RepoHandle handle, const eRenderLayer flag)
	{
		DX12_CommandSystem::GetInstance().SetRootSignature(DX12_RootSignatureSystem::GetInstance().GetGraphicsSignature("test"));
		mCommandList->SetPipelineState(DX12_PSOSystem::GetInstance().Get(flag));

		DX12_MeshGeometry* geo = DX12_MeshSystem::GetInstance().GetGeometry(handle);
		DX12_CommandSystem::GetInstance().SetMesh(geo);
		mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		for (const auto& [_, ri] : geo->DrawArgs)
		{
			mCommandList->DrawIndexedInstanced(ri.IndexCount, ri.InstanceCount, ri.StartIndexLocation, ri.BaseVertexLocation, 0);//ri.StartIndexLocation);
		}
	}

	inline void DrawSprites()
	{
		mCommandList->SetPipelineState(DX12_PSOSystem::GetInstance().Get(eRenderLayer::Sprite));
		DX12_CommandSystem::GetInstance().SetRootSignature(DX12_RootSignatureSystem::GetInstance().GetGraphicsSignature("sprite"));

		// mCommandList->SetGraphicsRootConstantBufferView(0, passCBAddress);
		// mCommandList->SetGraphicsRootShaderResourceView(1, instanceBufferAddress);

		DX12_MeshGeometry* geo = DX12_MeshSystem::GetInstance().GetGeometry(DX12_MeshRepository::GetInstance().Load("Sprite"));
		DX12_CommandSystem::GetInstance().SetMesh(geo);
		mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

		for (const auto& [_, ri] : geo->DrawArgs)
		{
			mCommandList->DrawIndexedInstanced(ri.IndexCount, ri.InstanceCount, ri.StartIndexLocation, ri.BaseVertexLocation, 0);
		}
	}

	inline void EndRenderPass() {
		D3D12_RESOURCE_BARRIER RenderBarrier = CD3DX12_RESOURCE_BARRIER::Transition(DX12_SwapChainSystem::GetInstance().GetBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		mCommandList->ResourceBarrier(1, &RenderBarrier);
		LOG_VERBOSE("Render Pass Ended");
	}
};