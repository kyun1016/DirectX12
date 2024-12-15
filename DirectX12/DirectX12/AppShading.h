#pragma once

#include "AppBase.h"
#include "StepTimer.h"
#include <dxgi1_5.h>

namespace kyun
{
	class AppShading : public AppBase
	{
	public:
		AppShading();
		AppShading(uint32_t width, uint32_t height, std::wstring name);
		virtual ~AppShading();

		static const UINT FrameCount = 3;

	protected:
		virtual void OnInit();
		virtual void OnUpdate() = 0;
		virtual void OnRender() = 0;
		virtual void OnSizeChanged(uint32_t width, uint32_t height, bool minimized) = 0;
		virtual void OnDestroy() = 0;
		virtual IDXGISwapChain* GetSwapchain() { return m_swapChain.Get(); }

	private:
		bool InitMainWindow();
		void LoadPipeline();
		void LoadAssets();
		void LoadSizeDependentResources();
	private:
		// D3D objects.
		ComPtr<ID3D12Device> m_device;
		ComPtr<ID3D12CommandQueue> m_commandQueue;
		ComPtr<IDXGISwapChain4> m_swapChain;
		ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
		ComPtr<ID3D12Fence> m_fence;

		// Scene rendering resources.
		// std::unique_ptr<VariableRateShadingScene> m_scene;

		StepTimer m_timer;


		// Frame synchronization objects.
		UINT   m_frameIndex;
		HANDLE m_fenceEvent;
		UINT64 m_fenceValues[FrameCount];

	};
}

