#pragma once
#include "DX12_Config.h"
#include "ECSSystem.h"
#include "DX12_RootSignatureSystem.h"
#include "DX12_ShaderCompileSystem.h"
#include "DX12_InputLayoutSystem.h"
#include "DX12_SwapChainSystem.h"

struct PSODescriptor {
	std::string sigName;
	std::string vsName, psName, gsName, hsName, dsName;
	std::string inputName;
	eRenderLayer layer = eRenderLayer::Opaque;

	D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	UINT numRenderTargets = 1;
	DXGI_FORMAT rtvFormats[8] = { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN };
	DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN; // DXGI_FORMAT_D24_UNORM_S8_UINT;
	bool wireframe = false;
};

class DX12_PSOSystem {
public:
	inline static DX12_PSOSystem& GetInstance() {
		static DX12_PSOSystem instance;
		return instance;
	}
	void Initialize(ID3D12Device* device)
	{
		mDevice = device;

		BuildExamplePSO();
		BuildSpritePSO();
		// BuildSpritePSO();
	}
	void RegisterDescriptor(const PSODescriptor& desc)
	{
		mDescriptors.push_back(desc);
		CreatePSO(desc);
	}
	ID3D12PipelineState* Get(eRenderLayer layer) const
	{
		auto it = mPSOs.find(layer);
		if (it != mPSOs.end()) return it->second.Get();
		return nullptr;
	}
private:
	DX12_PSOSystem()
		: mRootSystem(DX12_RootSignatureSystem::GetInstance())
		, mShaderSystem(DX12_ShaderCompileSystem::GetInstance())
		, mInputSystem(DX12_InputLayoutSystem::GetInstance())
		, mSwapChainSystem(DX12_SwapChainSystem::GetInstance())
	{
	};
	~DX12_PSOSystem() = default;
	DX12_PSOSystem(const DX12_PSOSystem&) = delete;
	DX12_PSOSystem& operator=(const DX12_PSOSystem&) = delete;
	DX12_PSOSystem(DX12_PSOSystem&&) = delete;
	DX12_PSOSystem& operator=(DX12_PSOSystem&&) = delete;
private:
	void BuildExamplePSO()
	{
		PSODescriptor desc;
		desc.layer = eRenderLayer::Opaque;
		desc.sigName = "test";
		desc.vsName = "vs_test";
		desc.psName = "ps_test";
		desc.inputName = "main";

		desc.blendDesc.IndependentBlendEnable = true;
		RegisterDescriptor(desc);
	}

	void BuildSpritePSO()
	{
		PSODescriptor desc;
		desc.layer = eRenderLayer::Sprite;
		desc.sigName = "sprite";
		desc.vsName = "vs_sprite";
		desc.gsName = "gs_sprite";
		desc.psName = "ps_sprite";
		desc.inputName = "sprite";
		desc.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

		// Alpha blending
		D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
		transparencyBlendDesc.BlendEnable = true;
		transparencyBlendDesc.LogicOpEnable = false;
		transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
		transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
		transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
		transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
		transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		desc.blendDesc.RenderTarget[0] = transparencyBlendDesc;

		desc.rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
		desc.depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // Don't write to depth buffer for transparent objects

		RegisterDescriptor(desc);
	}

private:
	void CreatePSO(const PSODescriptor& desc) {
		if (mPSOs.find(desc.layer) != mPSOs.end()) {
			LOG_ERROR("PSO for layer {} already exists.", static_cast<std::uint32_t>(desc.layer));
			return;
		}
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mRootSystem.GetGraphicsSignature(desc.layer);
		if (!desc.vsName.empty()) {
			auto shader = mShaderSystem.GetShader(desc.vsName);
			psoDesc.VS = { reinterpret_cast<BYTE*>(shader->GetBufferPointer()), shader->GetBufferSize() };
		}
		if (!desc.psName.empty()) {
			auto shader = mShaderSystem.GetShader(desc.psName);
			psoDesc.PS = { reinterpret_cast<BYTE*>(shader->GetBufferPointer()), shader->GetBufferSize() };
		}
		if (!desc.gsName.empty()) {
			auto shader = mShaderSystem.GetShader(desc.gsName);
			psoDesc.GS = { shader->GetBufferPointer(), shader->GetBufferSize() };
		}
		if (!desc.hsName.empty()) {
			auto shader = mShaderSystem.GetShader(desc.hsName);
			psoDesc.HS = { shader->GetBufferPointer(), shader->GetBufferSize() };
		}
		if (!desc.dsName.empty()) {
			auto shader = mShaderSystem.GetShader(desc.dsName);
			psoDesc.DS = { shader->GetBufferPointer(), shader->GetBufferSize() };
		}
		auto inputlayout = mInputSystem.GetLayout(desc.inputName);
		psoDesc.InputLayout = { inputlayout.data(), static_cast<UINT>(inputlayout.size()) };

		psoDesc.BlendState = desc.blendDesc;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.RasterizerState = desc.rasterizerDesc;
		psoDesc.DepthStencilState = desc.depthStencilDesc;
		psoDesc.IBStripCutValue = desc.IBStripCutValue;
		psoDesc.PrimitiveTopologyType = desc.topologyType;
		psoDesc.NumRenderTargets = desc.numRenderTargets;
		memcpy(psoDesc.RTVFormats, desc.rtvFormats, sizeof(DXGI_FORMAT) * 8);
		psoDesc.DSVFormat = desc.dsvFormat;
		psoDesc.SampleDesc.Count = mSwapChainSystem.GetMsaaState() ? 4u : 1u;
		psoDesc.SampleDesc.Quality = mSwapChainSystem.GetMsaaState() ? (mSwapChainSystem.GetMsaaQuality() - 1) : 0;
		psoDesc.NodeMask = 0;
		psoDesc.CachedPSO = { NULL, 0 };
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs[desc.layer])));
	}

	
private:
	DX12_RootSignatureSystem& mRootSystem;
	DX12_ShaderCompileSystem& mShaderSystem;
	DX12_InputLayoutSystem& mInputSystem;
	DX12_SwapChainSystem& mSwapChainSystem;
	std::unordered_map<eRenderLayer, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;
	std::vector<PSODescriptor> mDescriptors;
	ID3D12Device* mDevice = nullptr;
};