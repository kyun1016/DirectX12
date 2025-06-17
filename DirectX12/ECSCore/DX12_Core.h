//#pragma once
//#include "DX12_Config.h"
//#include "DX12_DeviceSystem.h"
//#include "DX12_CommandSystem.h"
//#include "DX12_RTVHeapRepository.h"
//#include "DX12_DSVHeapRepository.h"
//#include "DX12_SwapChainSystem.h"
//#include "DX12_RootSignatureSystem.h"
//#include "DX12_InputLayoutSystem.h"
//#include "DX12_ShaderCompileSystem.h"
//#include "DX12_PSOSystem.h"
//#include "WindowSystem.h"
//
//class DX12_Core {
//DEFAULT_SINGLETON(DX12_Core)
//public:
//	// Initialize DirectX 12 resources
//	inline void Initialize() {
//		// auto& coordinator = ECS::Coordinator::GetInstance();
//		// auto deviceSystem = coordinator.RegisterSystem<DX12_DeviceSystem>();
//		// mDevice = deviceSystem->GetDevice();
//		DX12_DeviceSystem& deviceSystem = DX12_DeviceSystem::GetInstance();
//		DX12_CommandSystem& commandSystem = DX12_CommandSystem::GetInstance();
//		DX12_SwapChainSystem& swapChainSystem = DX12_SwapChainSystem::GetInstance();
//		WindowComponent& wc = ECS::Coordinator::GetInstance().GetSingletonComponent<WindowComponent>();
//		
//		deviceSystem.Initialize();
//		commandSystem.Initialize(deviceSystem.GetDevice()); // 시스템 간 공유가 필요 없는 로직은 DX12_Core에서 처리하는 방식으로 구현
//		DX12_RTVHeapRepository::GetInstance().Initialize(deviceSystem.GetDevice());
//		DX12_DSVHeapRepository::GetInstance().Initialize(deviceSystem.GetDevice());
//		swapChainSystem.Initialize(deviceSystem.GetDevice(), deviceSystem.GetFactory(), commandSystem.GetCommandQueue(), wc.hwnd, wc.width, wc.height);
//		DX12_RootSignatureSystem::GetInstance().Initialize(deviceSystem.GetDevice());
//
//		DX12_InputLayoutSystem::GetInstance().Initialize();
//		DX12_ShaderCompileSystem::GetInstance().Initialize();
//
//		DX12_PSOSystem::GetInstance().Initialize(deviceSystem.GetDevice());
//		// Heap에 Texture 관련 데이터 업로드 공간 초기화
//		// Frame 관련 데이터 데이터 업로드 공간 초기화
//		// PSO 설정 초기화
//	}
//
//	ID3D12Device* GetDevice() const {
//		return DX12_DeviceSystem::GetInstance().GetDevice();
//	}
//
//private:
//	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;
//	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
//	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mGraphicsQueue;
//	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mComputeQueue;
//	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCopyQueue;
//	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPipelineState;
//};
//	