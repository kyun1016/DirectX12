#pragma once

#include "AppBase.h"
#include <dxgi1_5.h>
#include "d3dx12.h"
#include <directxtk/SimpleMath.h>


namespace kyun
{
	const wchar_t* c_meshFilename = L"..\\Assets\\Dragon_LOD0.bin";
	const wchar_t* c_meshShaderFilename = L"MeshletMS.cso";
	const wchar_t* c_pixelShaderFilename = L"MeshletPS.cso";

	class StepTimer;
	class Camera;
	class Model;

	using namespace DirectX;
	using Microsoft::WRL::ComPtr;

	class AppSimple : public AppBase
	{
		using Super = typename AppBase;

	public:
		AppSimple();
		AppSimple(uint32_t width, uint32_t height, std::wstring name);
		virtual ~AppSimple();


	protected:
		virtual void OnInit();
		virtual void OnUpdate() = 0;
		virtual void OnRender() = 0;
		virtual void OnSizeChanged(uint32_t width, uint32_t height, bool minimized) = 0;
		virtual void OnDestroy() = 0;
		virtual IDXGISwapChain* GetSwapchain() { return m_swapChain.Get(); }

	private:
		bool InitMainWindow();
		bool InitCamera();
		void LoadPipeline();
		void LoadAssets();
		void LoadSizeDependentResources();

	private:
		static const UINT FrameCount = 2;
		
		_declspec(align(256u)) struct SceneConstantBuffer
		{
			XMFLOAT4X4 World;
			XMFLOAT4X4 WorldView;
			XMFLOAT4X4 WorldViewProj;
			uint32_t   DrawMeshlets;
		};

		// Pipeline objects.
		CD3DX12_VIEWPORT m_viewport;
		CD3DX12_RECT m_scissorRect;
		ComPtr<IDXGISwapChain3> m_swapChain;
		ComPtr<ID3D12Device2> m_device;
		ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
		ComPtr<ID3D12Resource> m_depthStencil;
		ComPtr<ID3D12CommandAllocator> m_commandAllocators[FrameCount];
		ComPtr<ID3D12CommandQueue> m_commandQueue;
		ComPtr<ID3D12RootSignature> m_rootSignature;
		ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
		ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
		ComPtr<ID3D12PipelineState> m_pipelineState;
		ComPtr<ID3D12Resource> m_constantBuffer;
		UINT m_rtvDescriptorSize;
		UINT m_dsvDescriptorSize;

		ComPtr<ID3D12GraphicsCommandList6> m_commandList;
		SceneConstantBuffer m_constantBufferData;
		UINT8* m_cbvDataBegin;

		unique_ptr<StepTimer> m_timer;
		unique_ptr<Camera> m_camera;
		unique_ptr<Model> m_model;

		UINT m_rtvDescriptorSize;
		UINT m_dsvDescriptorSize;



		// Synchronization objects.
		UINT m_frameIndex;
		UINT m_frameCounter;
		HANDLE m_fenceEvent;
		ComPtr<ID3D12Fence> m_fence;
		UINT64 m_fenceValues[FrameCount];

	};
}

