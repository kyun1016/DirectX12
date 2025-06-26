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
#include "DX12_InstanceSystem.h" // Required for InstanceData access
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

	std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;

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
		DX12_InstanceSystem::GetInstance().Initialize();

		mObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(mDevice, 1, true);
		
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
		// 테스트를 위한 WVP 행렬 업데이트
		WindowComponent& wc = ECS::Coordinator::GetInstance().GetSingletonComponent<WindowComponent>();
		float4x4 world;
		float4x4 view = DirectX::SimpleMath::Matrix::CreateLookAt(
			float3(0.0f, 50.0f, -150.0f), // 카메라 위치
			float3(0.0f, 0.0f, 0.0f),     // 바라보는 지점
			float3(0.0f, 1.0f, 0.0f)      // Up 벡터
		);
		float4x4 proj = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(
			DirectX::XM_PIDIV4,
			static_cast<float>(wc.width) / static_cast<float>(wc.height),
			1.0f,
			1000.0f
		);
		float4x4 wvp = world * view * proj;

		ObjectConstants objConstants;
		objConstants.WorldViewProj = wvp.Transpose(); // HLSL은 Column-Major 행렬을 기대합니다.
		mObjectCB->CopyData(0, objConstants);

		DX12_CommandSystem::GetInstance().SetRootSignature(DX12_RootSignatureSystem::GetInstance().GetGraphicsSignature("test"));
		mCommandList->SetGraphicsRootConstantBufferView(0, mObjectCB->Resource()->GetGPUVirtualAddress());
		mCommandList->SetPipelineState(DX12_PSOSystem::GetInstance().Get(flag));

		DX12_MeshGeometry* geo = DX12_MeshSystem::GetInstance().GetGeometry(handle);
		DX12_CommandSystem::GetInstance().SetMesh(geo);
		mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		for (const auto& [key, ri] : geo->DrawArgs)
		{
			mCommandList->DrawIndexedInstanced(ri.IndexCount, ri.InstanceCount, ri.StartIndexLocation, ri.BaseVertexLocation, 0);
		}
	}

	inline void DrawSprites()
	{
		mCommandList->SetPipelineState(DX12_PSOSystem::GetInstance().Get(eRenderLayer::Sprite));
		DX12_CommandSystem::GetInstance().SetRootSignature(DX12_RootSignatureSystem::GetInstance().GetGraphicsSignature("sprite"));

		mCommandList->SetGraphicsRootConstantBufferView(0, mObjectCB->Resource()->GetGPUVirtualAddress()); // cbPass (gViewProj)
		mCommandList->SetGraphicsRootShaderResourceView(1, DX12_FrameResourceSystem::GetInstance().GetInstanceDataGPUVirtualAddress()); // gInstanceData

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