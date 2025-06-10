#pragma once
#include "DX12_Config.h"
#include "ECSSystem.h"

enum class RenderLayer : std::uint32_t
{
	None = 0,
	Opaque,
	SkinnedOpaque,
	Mirror,
	Reflected,
	AlphaTested,
	Transparent,
	Subdivision,
	Normal,
	SkinnedNormal,
	TreeSprites,
	Tessellation,
	BoundingBox,
	BoundingSphere,
	CubeMap,
	DebugShadowMap,
	OpaqueWireframe,
	MirrorWireframe,
	ReflectedWireframe,
	AlphaTestedWireframe,
	TransparentWireframe,
	SubdivisionWireframe,
	NormalWireframe,
	TreeSpritesWireframe,
	TessellationWireframe,
	ShadowMap,
	SkinnedShadowMap,
	AddCS,
	BlurCS,
	WaveCS,
	ShaderToy,
	Count
};

struct PSODescriptor {
    std::string name;
    std::string vsName, psName, gsName, hsName, dsName;
    RenderLayer layer;
    D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    D3D12_INPUT_LAYOUT_DESC inputLayout;
    D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    DXGI_FORMAT rtvFormats[8] = {};
    UINT numRenderTargets = 1;
    DXGI_FORMAT dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    bool wireframe = false;
};

class DX12_PSOSystem : public ECS::ISystem {
public:
    void Initialize(ID3D12Device* device, ID3D12RootSignature* rootSig)
    {

    }
    void RegisterDescriptor(const PSODescriptor& desc)
    {

    }
    ID3D12PipelineState* Get(RenderLayer layer) const
    {

    }

private:
    std::unordered_map<RenderLayer, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;
    std::vector<PSODescriptor> mDescriptors;
    ID3D12Device* mDevice = nullptr;
    ID3D12RootSignature* mRootSig = nullptr;
	void CreatePSO(const PSODescriptor& desc)
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc
		{
			/* ID3D12RootSignature* pRootSignature								*/.pRootSignature = mRootSignature.Get(),
			/* D3D12_SHADER_BYTECODE VS											*/.VS = {reinterpret_cast<BYTE*>(mShaders["MainVS"]->GetBufferPointer()), mShaders["MainVS"]->GetBufferSize()},
			/* D3D12_SHADER_BYTECODE PS											*/.PS = {reinterpret_cast<BYTE*>(mShaders["MainPS"]->GetBufferPointer()), mShaders["MainPS"]->GetBufferSize()},
			/* D3D12_SHADER_BYTECODE DS											*/.DS = {NULL, 0},
			/* D3D12_SHADER_BYTECODE HS											*/.HS = {NULL, 0},
			/* D3D12_SHADER_BYTECODE GS											*/.GS = {NULL, 0},
			/* D3D12_STREAM_OUTPUT_DESC StreamOutput{							*/.StreamOutput = {
				/*		const D3D12_SO_DECLARATION_ENTRY* pSODeclaration{			*/	NULL,
				/*			UINT Stream;											*/
				/*			LPCSTR SemanticName;									*/
				/*			UINT SemanticIndex;										*/
				/*			BYTE StartComponent;									*/
				/*			BYTE ComponentCount;									*/
				/*			BYTE OutputSlot;										*/
				/*		}															*/
			/*		UINT NumEntries;											*/	0,
			/*		const UINT* pBufferStrides;									*/	0,
			/*		UINT NumStrides;											*/	0,
			/*		UINT RasterizedStream;										*/	0
			/* }																*/},
			/* D3D12_BLEND_DESC BlendState{										*/.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
			/*		BOOL AlphaToCoverageEnable									*/
			/*		BOOL IndependentBlendEnable									*/
			/*		D3D12_RENDER_TARGET_BLEND_DESC RenderTarget[8]				*/
			/* }																*/
			/* UINT SampleMask													*/.SampleMask = UINT_MAX,
			/* D3D12_RASTERIZER_DESC RasterizerState{							*/.RasterizerState = { // CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
				/*		D3D12_FILL_MODE FillMode									*/		D3D12_FILL_MODE_SOLID, // D3D12_FILL_MODE_WIREFRAME,
				/*		D3D12_CULL_MODE CullMode									*/		D3D12_CULL_MODE_BACK,
				/*		BOOL FrontCounterClockwise									*/		false,
				/*		INT DepthBias												*/		D3D12_DEFAULT_DEPTH_BIAS,
				/*		FLOAT DepthBiasClamp										*/		D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
				/*		FLOAT SlopeScaledDepthBias									*/		D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
				/*		BOOL DepthClipEnable										*/		true,
				/*		BOOL MultisampleEnable										*/		false,
				/*		BOOL AntialiasedLineEnable									*/		false,
				/*		UINT ForcedSampleCount										*/		0,
				/*		D3D12_CONSERVATIVE_RASTERIZATION_MODE ConservativeRaster	*/		D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
				/* }																*/},
				/* D3D12_DEPTH_STENCIL_DESC DepthStencilState {						*/.DepthStencilState = { // CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
				/*		BOOL DepthEnable											*/		.DepthEnable = true,
				/*		D3D12_DEPTH_WRITE_MASK DepthWriteMask						*/		.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
				/*		D3D12_COMPARISON_FUNC DepthFunc								*/		.DepthFunc = D3D12_COMPARISON_FUNC_LESS,
				/*		BOOL StencilEnable											*/		.StencilEnable = false,
				/*		UINT8 StencilReadMask										*/		.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
				/*		UINT8 StencilWriteMask										*/		.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
				/*		D3D12_DEPTH_STENCILOP_DESC FrontFace {						*/		.FrontFace = {
				/*			D3D12_STENCIL_OP StencilFailOp							*/			.StencilFailOp = D3D12_STENCIL_OP_KEEP,
				/*			D3D12_STENCIL_OP StencilDepthFailOp						*/			.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
				/*			D3D12_STENCIL_OP StencilPassOp							*/			.StencilPassOp = D3D12_STENCIL_OP_KEEP,
				/*			D3D12_COMPARISON_FUNC StencilFunc						*/			.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
				/*		}															*/		},
			/*		D3D12_DEPTH_STENCILOP_DESC BackFace							*/		.BackFace = {
				/*			D3D12_STENCIL_OP StencilFailOp							*/			.StencilFailOp = D3D12_STENCIL_OP_KEEP,
				/*			D3D12_STENCIL_OP StencilDepthFailOp						*/			.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
				/*			D3D12_STENCIL_OP StencilPassOp							*/			.StencilPassOp = D3D12_STENCIL_OP_KEEP,
				/*			D3D12_COMPARISON_FUNC StencilFunc						*/			.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
				/*		}															*/		},
			/* }																*/ },
			/* D3D12_INPUT_LAYOUT_DESC InputLayout{								*/.InputLayout = {
				/*		const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs			*/		.pInputElementDescs = mMainInputLayout.data(),
				/*		UINT NumElements											*/		.NumElements = (UINT)mMainInputLayout.size()
				/*	}																*/ },
			/* D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue				*/.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
			/* D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType				*/.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			/* UINT NumRenderTargets											*/.NumRenderTargets = 2,
			/* DXGI_FORMAT RTVFormats[8]										*/.RTVFormats = {mParam.swapChainFormat, mParam.swapChainFormat,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN},	// 0
			/* DXGI_FORMAT DSVFormat											*/.DSVFormat = mParam.depthStencilFormat,
			/* DXGI_SAMPLE_DESC SampleDesc{										*/.SampleDesc = {
				/*		UINT Count;													*/		.Count = m4xMsaaState ? 4u : 1u,
				/*		UINT Quality;												*/		.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0
				/*	}																*/},
			/* UINT NodeMask													*/.NodeMask = 0,
			/* D3D12_CACHED_PIPELINE_STATE CachedPSO							*/.CachedPSO = {NULL, 0},
			/* D3D12_PIPELINE_STATE_FLAGS Flags									*/.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
		};
		ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::Opaque])));
	}

};