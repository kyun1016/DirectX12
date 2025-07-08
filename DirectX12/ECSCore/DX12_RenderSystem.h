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
#include "DX12_InstanceComponent.h" // Required for InstanceData access
#include "DX12_SceneSystem.h"
#include "CameraSystem.h"
struct ObjectConstants {
	float4x4 WorldViewProj;
};

class DX12_RenderSystem : public ECS::ISystem {
public:
	DX12_RenderSystem()
	{
		Initialize();
	}

	virtual void Sync() override {
		DX12_FrameResourceSystem::GetInstance().BeginFrame();
		DX12_SceneSystem::GetInstance().Update();
		CameraSystem::GetInstance().Sync();
	}

	virtual void Update() override {
		BeginRenderPass();
		DrawRenderItems(eRenderLayer::Opaque);
		// DrawRenderItems(eRenderLayer::Opaque);
		EndRenderPass();
		ImGuiSystem::GetInstance().Render();
		DX12_CommandSystem::GetInstance().EndAndExecuteCommandList();
		ImGuiSystem::GetInstance().RenderMultiViewport();
		DX12_SwapChainSystem::GetInstance().Present(true);
		DX12_FrameResourceSystem::GetInstance().EndFrame();
	}
private:
	ID3D12Device* mDevice;
	ID3D12GraphicsCommandList6* mCommandList;
	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	inline void Initialize() {
		ECS::Coordinator::GetInstance().RegisterSystem<WindowSystem>();
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
		DX12_SceneSystem::GetInstance().Initialize();
		CameraSystem::GetInstance().Initialize();
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
		float r = std::fmod(time.totalTime * 0.1f, 1.0f); // Example: Use time to create a dynamic color
		float g = std::fmod(time.totalTime * 0.2f, 1.0f); // Example: Use time to create a dynamic color
		float b = std::fmod(time.totalTime * 0.05f, 1.0f); // Example: Use time to create a dynamic color
		float4 FogColor = { r, g, b, 1.0f };
		mCommandList->ClearRenderTargetView(DX12_SwapChainSystem::GetInstance().GetBackBufferDescriptorHandle(), (float*)&FogColor, 0, nullptr);
		mCommandList->OMSetRenderTargets(1, &DX12_SwapChainSystem::GetInstance().GetBackBufferDescriptorHandle(), false, nullptr);
	}

	inline void DrawRenderItems(const eRenderLayer flag)
	{
		mCommandList->SetPipelineState(DX12_PSOSystem::GetInstance().Get(flag));
		DX12_CommandSystem::GetInstance().SetRootSignature(DX12_RootSignatureSystem::GetInstance().GetGraphicsSignature(flag));

		auto& allRenderItems = DX12_SceneSystem::GetInstance().GetRenderItems();
		mCommandList->SetGraphicsRootConstantBufferView(0, DX12_FrameResourceSystem::GetInstance().GetCameraDataGPUVirtualAddress());
		
		for (auto& ri : allRenderItems)
		{
			if (!(ri->TargetLayer & flag))
				continue;
			for (size_t geoIdx = 1; geoIdx < ri->MeshIndex.size(); ++geoIdx)
			{
				for (size_t meshIdx = 0; meshIdx < ri->MeshIndex[geoIdx].size(); ++meshIdx)
				{
					DX12_MeshHandle meshHandle = { static_cast<ECS::RepoHandle>(geoIdx), meshIdx };
					DX12_CommandSystem::GetInstance().SetMesh(DX12_MeshSystem::GetInstance().GetGeometry(meshHandle));
					auto* meshComponent = DX12_MeshSystem::GetInstance().GetMeshComponent(meshHandle);

					mCommandList->DrawIndexedInstanced(meshComponent->IndexCount, meshComponent->InstanceCount, meshComponent->StartIndexLocation, meshComponent->BaseVertexLocation, 0);
				}
			}
		}
	}

	inline void EndRenderPass() {
		D3D12_RESOURCE_BARRIER RenderBarrier = CD3DX12_RESOURCE_BARRIER::Transition(DX12_SwapChainSystem::GetInstance().GetBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		mCommandList->ResourceBarrier(1, &RenderBarrier);
		LOG_VERBOSE("Render Pass Ended");
	}
};