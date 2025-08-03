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
#include "InstanceComponent.h" // Required for InstanceData access
#include "DX12_SceneSystem.h"
#include "DX12_HeapRepository.h"
#include "CameraSystem.h"
#include "TextureSystem.h"
#include "DX12_PipelineManager.h"

class DX12_RenderSystem : public ECS::ISystem {
public:
	DX12_RenderSystem()
	{
		Initialize();
	}

	virtual void Sync() override
	{
		CameraSystem::GetInstance().Sync();
		DX12_SceneSystem::GetInstance().UpdateInstance(ImGuiSystem::GetInstance().GetSelectInstance(), InputSystem::GetInstance());
		DX12_SceneSystem::GetInstance().Update(DX12_FrameResourceSystem::GetInstance().GetCurrentFrameResource().InstanceDataBuffer.get(), DX12_FrameResourceSystem::GetInstance().GetCurrentFrameResource().InstanceIDCB.get());
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
	std::unique_ptr<TextureSystem> mTextureSystem;
	std::unique_ptr<DX12_HeapRepository> mSRVHeapRepository;
	std::unique_ptr<DX12_HeapRepository> mRTVHeapRepository;
	std::unique_ptr<DX12_HeapRepository> mDSVHeapRepository;

	inline void Initialize() {
		WindowSystem::GetInstance().Initialize();
		WindowComponent& wc = WindowSystem::GetInstance().GetWindowComponent();
		DX12_DeviceSystem::GetInstance().Initialize();
		mDevice = DX12_DeviceSystem::GetInstance().GetDevice();
		DX12_CommandSystem::GetInstance().Initialize(mDevice);
		mCommandList = DX12_CommandSystem::GetInstance().GetCommandList();
		DX12_CommandSystem::GetInstance().BeginCommandList();
		mTextureSystem = std::make_unique<TextureSystem>(mDevice, mCommandList);
		mTextureSystem->Initialize();
		mRTVHeapRepository = std::make_unique<DX12_HeapRepository>(mDevice, APP_NUM_BACK_BUFFERS, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
		mDSVHeapRepository = std::make_unique<DX12_HeapRepository>(mDevice, 3, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
		mSRVHeapRepository = std::make_unique<DX12_HeapRepository>(mDevice, mTextureSystem->Size(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
		
		DX12_SwapChainSystem::GetInstance().Initialize(mDevice, DX12_CommandSystem::GetInstance().GetCommandQueue(), DX12_DeviceSystem::GetInstance().GetFactory(), wc.hwnd, wc.width, wc.height, mRTVHeapRepository.get());
		DX12_RootSignatureSystem::GetInstance().Initialize(mDevice, mTextureSystem->Size());
		DX12_InputLayoutSystem::GetInstance().Initialize();
		DX12_ShaderCompileSystem::GetInstance().Initialize(mTextureSystem->Size());
		DX12_PSOSystem::GetInstance().Initialize(mDevice);
		DX12_PipelineManager::GetInstance().Initialize(mDevice);
		DX12_FrameResourceSystem::GetInstance().Initialize(mDevice);

		DX12_MeshSystem::GetInstance().Initialize();
		DX12_SceneSystem::GetInstance().Initialize();
		CameraSystem::GetInstance().Initialize();
		ImGuiSystem::GetInstance().Initialize(wc.hwnd, mDevice, DX12_CommandSystem::GetInstance().GetCommandQueue(), mCommandList);

		{
			auto& textures = mTextureSystem->GetTextures();
			for (auto& texture : textures)
			{
				texture->Handle = mSRVHeapRepository->LoadTexture(texture.get());
			}
		}

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
		float r = std::fmod(time.totalTime * 0.1f, 1.0f); // Example: Use time to create a dynamic color
		float g = std::fmod(time.totalTime * 0.2f, 1.0f); // Example: Use time to create a dynamic color
		float b = std::fmod(time.totalTime * 0.05f, 1.0f); // Example: Use time to create a dynamic color
		float4 FogColor = { r, g, b, 1.0f };
		mCommandList->ClearRenderTargetView(DX12_SwapChainSystem::GetInstance().GetBackBufferDescriptorHandle(), (float*)&FogColor, 0, nullptr);
		mCommandList->OMSetRenderTargets(1, &DX12_SwapChainSystem::GetInstance().GetBackBufferDescriptorHandle(), false, nullptr);
	}

	inline void DrawRenderItems(const eRenderLayer flag)
	{
		// 자동화된 파이프라인 바인딩
		ID3D12PipelineState* pso = DX12_PipelineManager::GetInstance().GetPipelineState(flag);
		ID3D12RootSignature* rootSig = DX12_PipelineManager::GetInstance().GetRootSignature(flag);
		
		if (!pso || !rootSig)
		{
			LOG_WARN("Pipeline or Root Signature not found for layer: {}", static_cast<int>(flag));
			// return;
		}

		const D3D12_GPU_VIRTUAL_ADDRESS baseInstanceIDAddress = DX12_FrameResourceSystem::GetInstance().GetInstanceIDDataGPUVirtualAddress();
		const UINT objCBByteSize = CalcConstantBufferByteSize(sizeof(InstanceIDData));
		
		ID3D12DescriptorHeap* descriptorHeaps[] = { mSRVHeapRepository->GetHeap()};
		mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

		mCommandList->SetPipelineState(pso);
		// TODO: 자동으로 생성된 RootSignature 기반으로 자동 바인딩
		mCommandList->SetGraphicsRootSignature(rootSig);
		mCommandList->SetGraphicsRootShaderResourceView(1, DX12_FrameResourceSystem::GetInstance().GetInstanceDataGPUVirtualAddress());
		mCommandList->SetGraphicsRootShaderResourceView(2, DX12_FrameResourceSystem::GetInstance().GetCameraDataGPUVirtualAddress());
		mCommandList->SetGraphicsRootDescriptorTable(4, mSRVHeapRepository->GetGPUHandle(0));

		auto& allRenderItems = DX12_SceneSystem::GetInstance().GetRenderItems();
		size_t totalMeshIdx = 0;
		for (size_t i = 0; i < allRenderItems.size(); ++i)
		{
			auto& ri = allRenderItems[i];
			if (!(ri.TargetLayer & flag))
				continue;
					
			DX12_CommandSystem::GetInstance().SetMesh(DX12_MeshSystem::GetInstance().GetGeometry(ri.GeometryHandle));
			auto* meshComponent = DX12_MeshSystem::GetInstance().GetMeshComponent(ri.GeometryHandle, ri.MeshHandle);
			
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