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

class DX12_RenderSystem : public ECS::ISystem {
public:
	DX12_RenderSystem()
	{
		Initialize();
	}

	virtual void Sync() override {
		
		CameraSystem::GetInstance().Sync();
		DX12_SceneSystem::GetInstance().Update();
		DX12_FrameResourceSystem::GetInstance().BeginFrame();
	}

	virtual void Update() override {
		BeginRenderPass();
		// DrawRenderItems(eRenderLayer::Opaque);
		// DrawRenderItems(eRenderLayer::Test);
		DrawRenderItems(eRenderLayer::Sprite);
		EndRenderPass();
		ImGuiSystem::GetInstance().Render();
		DX12_CommandSystem::GetInstance().ExecuteCommandList();
		DX12_SwapChainSystem::GetInstance().Present(false);
		DX12_FrameResourceSystem::GetInstance().EndFrame();
	}
private:
	ID3D12Device* mDevice;
	ID3D12GraphicsCommandList6* mCommandList;
	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	inline void Initialize() {
		WindowSystem::GetInstance().Initialize();
		WindowComponent& wc = WindowSystem::GetInstance().GetWindowComponent();

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

		DX12_CommandSystem::GetInstance().ExecuteCommandList();
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
		ID3D12PipelineState* pso = DX12_PSOSystem::GetInstance().Get(flag);
		if (!pso)
		{
			LOG_ERROR("Pipeline State Object not found for layer: {}", static_cast<int>(flag));
			return;
		}
		//mCommandList->SetPipelineState(pso);
		//DX12_CommandSystem::GetInstance().SetRootSignature(DX12_RootSignatureSystem::GetInstance().GetGraphicsSignature(flag));
		//mCommandList->SetGraphicsRootConstantBufferView(0, DX12_FrameResourceSystem::GetInstance().GetInstanceIDDataGPUVirtualAddress());
		//mCommandList->SetGraphicsRootShaderResourceView(1, DX12_FrameResourceSystem::GetInstance().GetInstanceDataGPUVirtualAddress());
		//mCommandList->SetGraphicsRootShaderResourceView(2, DX12_FrameResourceSystem::GetInstance().GetCameraDataGPUVirtualAddress());
		//
		//auto& allRenderItems = DX12_SceneSystem::GetInstance().GetRenderItems();
		//for (auto& ri : allRenderItems)
		//{
		//	if (!(ri->TargetLayer & flag))
		//		continue;
		//	for (size_t geoIdx = 1; geoIdx < ri->MeshIndex.size(); ++geoIdx)
		//	{
		//		for (size_t meshIdx = 0; meshIdx < ri->MeshIndex[geoIdx].size(); ++meshIdx)
		//		{
		//			DX12_MeshHandle meshHandle = { static_cast<ECS::RepoHandle>(geoIdx), meshIdx };
		//			DX12_CommandSystem::GetInstance().SetMesh(DX12_MeshSystem::GetInstance().GetGeometry(meshHandle));
		//			auto* meshComponent = DX12_MeshSystem::GetInstance().GetMeshComponent(meshHandle);

		//			// mCommandList->DrawInstanced(meshComponent->IndexCount, meshComponent->InstanceCount, meshComponent->StartIndexLocation, meshComponent->InstanceCount);
		//			mCommandList->DrawIndexedInstanced(meshComponent->IndexCount, meshComponent->InstanceCount, meshComponent->StartIndexLocation, meshComponent->BaseVertexLocation, meshComponent->StartInstanceLocation);
		//		}
		//	}
		//}

		const D3D12_GPU_VIRTUAL_ADDRESS baseInstanceIDAddress = DX12_FrameResourceSystem::GetInstance().GetInstanceIDDataGPUVirtualAddress();
		const UINT objCBByteSize = CalcConstantBufferByteSize(sizeof(InstanceIDData));
		// const UINT objCBByteSize = sizeof(InstanceIDData);
		mCommandList->SetPipelineState(pso);
		DX12_CommandSystem::GetInstance().SetRootSignature(DX12_RootSignatureSystem::GetInstance().GetGraphicsSignature(flag));
		mCommandList->SetGraphicsRootShaderResourceView(1, DX12_FrameResourceSystem::GetInstance().GetInstanceDataGPUVirtualAddress());
		mCommandList->SetGraphicsRootShaderResourceView(2, DX12_FrameResourceSystem::GetInstance().GetCameraDataGPUVirtualAddress());

		auto& allRenderItems = DX12_SceneSystem::GetInstance().GetRenderItems();
		size_t totalMeshIdx = 0;
		for (size_t i = 0; i < allRenderItems.size(); ++i)
		{
			auto& ri = allRenderItems[i];
			if (!(ri.TargetLayer & flag))
				continue;
					
			DX12_CommandSystem::GetInstance().SetMesh(DX12_MeshSystem::GetInstance().GetGeometry(ri.MeshHandle));
			auto* meshComponent = DX12_MeshSystem::GetInstance().GetMeshComponent(ri.MeshHandle);
			D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = baseInstanceIDAddress + i * objCBByteSize;
			mCommandList->SetGraphicsRootConstantBufferView(0, objCBAddress);
			mCommandList->DrawIndexedInstanced(meshComponent->IndexCount, meshComponent->InstanceCount, meshComponent->StartIndexLocation, meshComponent->BaseVertexLocation, meshComponent->StartInstanceLocation);
		}
	}

	inline void EndRenderPass() {
		D3D12_RESOURCE_BARRIER RenderBarrier = CD3DX12_RESOURCE_BARRIER::Transition(DX12_SwapChainSystem::GetInstance().GetBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		mCommandList->ResourceBarrier(1, &RenderBarrier);
		LOG_VERBOSE("Render Pass Ended");
	}
};