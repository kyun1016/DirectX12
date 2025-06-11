#pragma once
#include "DX12_Config.h"
#include "DX12_DeviceSystem.h"
#include "DX12_CommandSystem.h"
#include "DX12_RTVHeapRepository.h"
#include "DX12_DSVHeapRepository.h"
#include "DX12_SwapChainSystem.h"
#include "DX12_RootSignatureSystem.h"
#include "DX12_InputLayoutSystem.h"
#include "DX12_ShaderCompileSystem.h"
#include "WindowSystem.h"

class DX12_Core {
DEFAULT_SINGLETON(DX12_RootSignatureSystem)
public:
	inline static DX12_Core& GetInstance() {
		static DX12_Core instance;
		return instance;
	}

	// Initialize DirectX 12 resources
	inline void Initialize() {
		// auto& coordinator = ECS::Coordinator::GetInstance();
		// auto deviceSystem = coordinator.RegisterSystem<DX12_DeviceSystem>();
		// mDevice = deviceSystem->GetDevice();
		DX12_DeviceSystem& deviceSystem = DX12_DeviceSystem::GetInstance();
		DX12_CommandSystem& commandSystem = DX12_CommandSystem::GetInstance();
		DX12_SwapChainSystem& swapChainSystem = DX12_SwapChainSystem::GetInstance();
		WindowComponent& wc = ECS::Coordinator::GetInstance().GetSingletonComponent<WindowComponent>();
		
		deviceSystem.Initialize();
		commandSystem.Initialize(deviceSystem.GetDevice()); // 시스템 간 공유가 필요 없는 로직은 DX12_Core에서 처리하는 방식으로 구현
		DX12_RTVHeapRepository::GetInstance().Initialize(deviceSystem.GetDevice());
		DX12_DSVHeapRepository::GetInstance().Initialize(deviceSystem.GetDevice());
		swapChainSystem.Initialize(deviceSystem.GetDevice(), deviceSystem.GetFactory(), commandSystem.GetCommandQueue(), wc.hwnd, wc.width, wc.height);
		DX12_RootSignatureSystem::GetInstance().Initialize(deviceSystem.GetDevice());

		DX12_InputLayoutSystem::GetInstance().Initialize();
		DX12_ShaderCompileSystem::GetInstance().Initialize();
		// Heap에 Texture 관련 데이터 업로드 공간 초기화
		// Frame 관련 데이터 데이터 업로드 공간 초기화
		// PSO 설정 초기화
	}

	ID3D12Device* GetDevice() const {
		return DX12_DeviceSystem::GetInstance().GetDevice();
	}

private:
	DX12_Core() = default;
	virtual ~DX12_Core() = default;
	DX12_Core(const DX12_Core&) = delete;
	DX12_Core& operator=(const DX12_Core&) = delete;
	DX12_Core(DX12_Core&&) = delete;
	DX12_Core& operator=(DX12_Core&&) = delete; 

	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mGraphicsQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mComputeQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCopyQueue;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPipelineState;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mImGuiHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTexture;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontUploadBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontReadbackBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureUploadHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureReadbackHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureSRV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureUAV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureRTV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDSV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencil;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilUploadHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilReadbackHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilSRV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilUAV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilRTV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilDSV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilResourceSRV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilResourceUAV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilResourceRTV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilResourceDSV;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilResourceReadbackHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> mImGuiFontTextureDepthStencilResourceUploadHeap;
	// Root signature
	
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mImGuiRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mImGuiPipelineState;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mGraphicsPipelineState;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mComputePipelineState;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mCopyPipelineState;
};
	