#pragma once

#include "AppBase.h"
#include <dxgi1_5.h>
#include "d3dx12.h"
#include <directxtk/SimpleMath.h>
#include "MathHelper.h"
#include "UploadBuffer.h"

namespace kyun
{
	using namespace DirectX;
	using Microsoft::WRL::ComPtr;

	struct Vertex
	{
		XMFLOAT3 Pos;
		XMFLOAT4 Color;
	};

	struct ObjectConstants
	{
		XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4();
	};

	class AppSimple : public AppBase
	{
		using Super = typename AppBase;

	public:
		AppSimple();
		AppSimple(uint32_t width, uint32_t height, std::wstring name);
		virtual ~AppSimple();

		virtual bool OnInit() override;

	private:
		virtual void OnResize()override;
		virtual void OnUpdate(const StepTimer& dt)override;
		virtual void OnDraw(const StepTimer& dt)override;

		virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
		virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
		virtual void OnMouseMove(WPARAM btnState, int x, int y)override;
		void BuildDescriptorHeaps();
		void BuildConstantBuffers();
		void BuildRootSignature();
		void BuildShadersAndInputLayout();
		void BuildBoxGeometry();
		void BuildPSO();

	private:		
		ComPtr<ID3D12RootSignature> m_rootSignature = nullptr;
		ComPtr<ID3D12DescriptorHeap> m_cbvHeap = nullptr;

		std::unique_ptr<UploadBuffer<ObjectConstants>> m_objectCB = nullptr;

		ComPtr<ID3D12PipelineState> mPSO = nullptr;

		XMFLOAT4X4 m_world = MathHelper::Identity4x4();
		XMFLOAT4X4 m_view = MathHelper::Identity4x4();
		XMFLOAT4X4 m_proj = MathHelper::Identity4x4();
	};
}

