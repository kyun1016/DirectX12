#pragma once
#include "DX12_Config.h"
#include "ECSSystem.h"
#include "DX12_RootSignatureSystem.h"
#include "DX12_ShaderCompileSystem.h"
#include "DX12_InputLayoutSystem.h"
#include "DX12_SwapChainSystem.h"
#include "LogCore.h"
#include <d3d12shader.h>  // ID3D12ShaderReflection을 위해 추가

// 파일: DX12_PipelineManager.h
class DX12_PipelineManager {
	DEFAULT_SINGLETON(DX12_PipelineManager)
public:
	struct PipelineDefinition {
		// 식별 정보
		std::string name;
		eRenderLayer layer;

		// 셰이더 정의
		std::vector<D3D_SHADER_MACRO> defines;
		std::string vertexShader;
		std::string pixelShader;
		std::string geometryShader;
		std::string hullShader;
		std::string domainShader;

		// Input Layout (자동 생성 또는 명시적 정의)
		std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;

		// Root Parameters (자동 생성 또는 명시적 정의)
		std::vector<CD3DX12_ROOT_PARAMETER> rootParameters;
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		// 렌더 상태
		D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		// 출력 형식
		std::vector<DXGI_FORMAT> rtvFormats = { DXGI_FORMAT_R8G8B8A8_UNORM };
		DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN;
	};
	struct RootParameterBinding {
		UINT rootParameterIndex;
		D3D12_ROOT_PARAMETER_TYPE type;
		UINT registerSlot;
		UINT registerSpace;
		std::string resourceName;
	};
	struct CompiledPipeline {
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
		PipelineDefinition definition;
		std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> compiledShaders;
		std::vector<RootParameterBinding> rootParameterBindings;
	};
private:
	ID3D12Device* mDevice = nullptr;
	std::unordered_map<std::string, CompiledPipeline> mPipelines;
	std::unordered_map<eRenderLayer, std::string> mLayerToPipeline;
	D3D12_DESCRIPTOR_RANGE mSpriteTextureRange;

public:
	void Initialize(ID3D12Device* device) {
		mDevice = device;

		// Descriptor ranges 초기화
		mSpriteTextureRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		mSpriteTextureRange.NumDescriptors = 16;
		mSpriteTextureRange.BaseShaderRegister = 0;
		mSpriteTextureRange.RegisterSpace = 1;
		mSpriteTextureRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// 모든 파이프라인 정의 등록
		RegisterAllPipelines();
	}

	// 통합된 파이프라인 등록
	void RegisterPipeline(const PipelineDefinition& definition) {
		CompiledPipeline pipeline;
		pipeline.definition = definition;

		CompileShaders(definition, pipeline);
		CreateRootSignature(definition, pipeline);
		CreatePipelineState(definition, pipeline);

		mPipelines[definition.name] = std::move(pipeline);
		mLayerToPipeline[definition.layer] = definition.name;
	}

	ID3D12PipelineState* GetPipelineState(const std::string& name) const {
		auto it = mPipelines.find(name);
		return (it != mPipelines.end()) ? it->second.pipelineState.Get() : nullptr;
	}
	ID3D12PipelineState* GetPipelineState(eRenderLayer layer) const {
		auto it = mLayerToPipeline.find(layer);
		if (it != mLayerToPipeline.end()) {
			return GetPipelineState(it->second);
		}
		return nullptr;
	}

	ID3D12RootSignature* GetRootSignature(const std::string& name) const {
		auto it = mPipelines.find(name);
		return (it != mPipelines.end()) ? it->second.rootSignature.Get() : nullptr;
	}

	ID3D12RootSignature* GetRootSignature(eRenderLayer layer) const {
		auto it = mLayerToPipeline.find(layer);
		if (it != mLayerToPipeline.end()) {
			return GetRootSignature(it->second);
		}
		return nullptr;
	}

	// 특정 리소스의 Root Parameter 인덱스 조회
	std::optional<UINT> GetRootParameterIndex(const std::string& pipelineName, const std::string& resourceName) const {
		auto it = mPipelines.find(pipelineName);
		if (it == mPipelines.end()) return std::nullopt;

		const auto& bindings = it->second.rootParameterBindings;
		for (const auto& binding : bindings) {
			if (binding.resourceName == resourceName) {
				return binding.rootParameterIndex;
			}
		}
		return std::nullopt;
	}

	std::optional<UINT> GetRootParameterIndex(eRenderLayer layer, const std::string& resourceName) const {
		auto it = mLayerToPipeline.find(layer);
		if (it != mLayerToPipeline.end()) {
			return GetRootParameterIndex(it->second, resourceName);
		}
		return std::nullopt;
	}

	// 리소스 이름으로 Root Parameter 인덱스 조회 (가장 정확한 방법)
	std::optional<UINT> GetRootParameterIndexByResourceName(eRenderLayer layer, const std::string& resourceName) const {
		auto it = mLayerToPipeline.find(layer);
		if (it == mLayerToPipeline.end()) return std::nullopt;

		auto pipelineIt = mPipelines.find(it->second);
		if (pipelineIt == mPipelines.end()) return std::nullopt;

		const auto& bindings = pipelineIt->second.rootParameterBindings;
		for (const auto& binding : bindings) {
			if (binding.resourceName == resourceName) {
				return binding.rootParameterIndex;
			}
		}
		return std::nullopt;
	}

	// 모든 바인딩 정보 조회 (디버깅용)
	std::vector<RootParameterBinding> GetAllBindings(eRenderLayer layer) const {
		auto it = mLayerToPipeline.find(layer);
		if (it == mLayerToPipeline.end()) return {};

		auto pipelineIt = mPipelines.find(it->second);
		if (pipelineIt == mPipelines.end()) return {};

		return pipelineIt->second.rootParameterBindings;
	}

private:
	void RegisterAllPipelines() {
		RegisterStandardMeshPipeline();
		RegisterSpritePipeline();
		RegisterTestPipeline();
	}

	void RegisterStandardMeshPipeline() {
		PipelineDefinition pipeline;
		// Step 1. Compile Shaders 설정
		pipeline.name = "StandardMesh";
		pipeline.layer = eRenderLayer::Opaque;
		pipeline.vertexShader = "../Data/Shaders/Main.hlsl";
		pipeline.pixelShader = "../Data/Shaders/Main.hlsl";

		// Step 2. Input Layout 정의
		// 미정의 시 자동 생성을 의도하고 있으며, Reflection 기능을 응용하였지만, 버그 발생
		// -> 따라서 Json 방식으로 코드 자동 생성을 고민하고 있음
		// --> 우선은 기존 방식 그대로 직접 정의 하는 방식으로 유지 결정
		pipeline.inputLayout = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Step 3. Root Parameters 정의
		auto& param = pipeline.rootParameters;
		CD3DX12_ROOT_PARAMETER tmp;
		tmp.InitAsConstantBufferView(0); param.push_back(tmp);      // CBV, gBaseInstanceIndex b0
		tmp.InitAsConstantBufferView(1); param.push_back(tmp);      // CBV, cbPass b1
		tmp.InitAsConstantBufferView(2); param.push_back(tmp);      // CBV, cbSkinned b2
		tmp.InitAsConstantBufferView(3); param.push_back(tmp);      // CBV, cbShaderToy b3
		tmp.InitAsShaderResourceView(0, 0); param.push_back(tmp);   // SRV, InstanceData t0 (Space0)
		tmp.InitAsShaderResourceView(1, 0); param.push_back(tmp);   // SRV, MaterialData t1 (Space0)

		RegisterPipeline(pipeline);
	}

	void RegisterSpritePipeline() {
		PipelineDefinition pipeline;
		pipeline.name = "Sprite";
		pipeline.layer = eRenderLayer::Sprite;
		pipeline.vertexShader = "../Data/Shaders/Sprite.hlsl";
		pipeline.geometryShader = "../Data/Shaders/Sprite.hlsl";
		pipeline.pixelShader = "../Data/Shaders/Sprite.hlsl";
		pipeline.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

		// Input Layout
		pipeline.inputLayout = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		auto& param = pipeline.rootParameters;
		CD3DX12_ROOT_PARAMETER tmp;
		tmp.InitAsConstantBufferView(0); param.push_back(tmp);       // CBV, cbInstanceID b1
		tmp.InitAsShaderResourceView(0, 0); param.push_back(tmp);    // SRV, InstanceData t0 (Space0)
		tmp.InitAsShaderResourceView(1, 0); param.push_back(tmp);    // SRV, CameraData t1 (Space0)
		tmp.InitAsShaderResourceView(2, 0); param.push_back(tmp);    // SRV, LightData t2 (Space0)
		tmp.InitAsDescriptorTable(1, &mSpriteTextureRange, D3D12_SHADER_VISIBILITY_PIXEL); param.push_back(tmp);

		// 알파 블렌딩 설정
		pipeline.blendDesc.RenderTarget[0].BlendEnable = true;
		pipeline.blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		pipeline.blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		pipeline.blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

		pipeline.rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
		pipeline.depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

		RegisterPipeline(pipeline);
	}

	void RegisterTestPipeline() {
		PipelineDefinition pipeline;
		pipeline.name = "Test";
		pipeline.layer = eRenderLayer::Test;
		pipeline.vertexShader = "../Data/Shaders/test.hlsl";
		pipeline.pixelShader = "../Data/Shaders/test.hlsl";

		// Input Layout
		pipeline.inputLayout = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		RegisterPipeline(pipeline);
	}

	void CompileShaders(const PipelineDefinition& definition, CompiledPipeline& pipeline) {
		if (!definition.vertexShader.empty()) {
			pipeline.compiledShaders["VS"] = DX12_ShaderCompileSystem::CompileShader(definition.vertexShader, definition.defines.data(), "VS", "vs_5_1");
		}
		if (!definition.pixelShader.empty()) {
			pipeline.compiledShaders["PS"] = DX12_ShaderCompileSystem::CompileShader(definition.pixelShader, definition.defines.data(), "PS", "ps_5_1");
		}
		if (!definition.geometryShader.empty()) {
			pipeline.compiledShaders["GS"] = DX12_ShaderCompileSystem::CompileShader(definition.geometryShader, definition.defines.data(), "GS", "gs_5_1");
		}
		if (!definition.hullShader.empty()) {
			pipeline.compiledShaders["HS"] = DX12_ShaderCompileSystem::CompileShader(definition.hullShader, definition.defines.data(), "HS", "hs_5_1");
		}
		if (!definition.domainShader.empty()) {
			pipeline.compiledShaders["DS"] = DX12_ShaderCompileSystem::CompileShader(definition.domainShader, definition.defines.data(), "DS", "ds_5_1");
		}
	}

	void CreateRootSignature(const PipelineDefinition& definition, CompiledPipeline& pipeline) {
		std::vector<CD3DX12_ROOT_PARAMETER> rootParams;

		if (definition.rootParameters.empty()) {
			// 셰이더 reflection을 통한 자동 생성
			rootParams = GenerateRootParametersFromShader(pipeline.compiledShaders, pipeline);
		}
		else {
			// 명시적 정의 사용
			rootParams = definition.rootParameters;
		}

		// Input Layout 자동 생성 (필요시)
		if (definition.inputLayout.empty()) {
			auto generatedLayout = GenerateInputLayoutFromShader(pipeline.compiledShaders);
			// PipelineDefinition이 const이므로 pipeline.definition에 직접 저장
			const_cast<PipelineDefinition&>(pipeline.definition).inputLayout = generatedLayout;
		}

		auto samplers = GetStaticSamplers();
		CD3DX12_ROOT_SIGNATURE_DESC desc;
		if (rootParams.empty()) {
			// 완전히 빈 Root Signature
			desc.Init(0, nullptr, (UINT)samplers.size(), samplers.data(), definition.rootSignatureFlags);
		} else {
			// 일반적인 Root Signature
			desc.Init((UINT)rootParams.size(), rootParams.data(),
					(UINT)samplers.size(), samplers.data(), definition.rootSignatureFlags);
		}

		// Root Signature 생성 (기존 RootSignatureSystem 로직 통합)
		CreateRootSignatureFromDesc(desc, pipeline.rootSignature);
	}

	void CreatePipelineState(const PipelineDefinition& definition, CompiledPipeline& pipeline) {
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

		// Root Signature
		psoDesc.pRootSignature = pipeline.rootSignature.Get();

		// 셰이더들
		if (auto it = pipeline.compiledShaders.find("VS"); it != pipeline.compiledShaders.end()) {
			psoDesc.VS = { it->second->GetBufferPointer(), it->second->GetBufferSize() };
		}
		if (auto it = pipeline.compiledShaders.find("PS"); it != pipeline.compiledShaders.end()) {
			psoDesc.PS = { it->second->GetBufferPointer(), it->second->GetBufferSize() };
		}
		if (auto it = pipeline.compiledShaders.find("GS"); it != pipeline.compiledShaders.end()) {
			psoDesc.GS = { it->second->GetBufferPointer(), it->second->GetBufferSize() };
		}
		if (auto it = pipeline.compiledShaders.find("HS"); it != pipeline.compiledShaders.end()) {
			psoDesc.HS = { it->second->GetBufferPointer(), it->second->GetBufferSize() };
		}
		if (auto it = pipeline.compiledShaders.find("DS"); it != pipeline.compiledShaders.end()) {
			psoDesc.DS = { it->second->GetBufferPointer(), it->second->GetBufferSize() };
		}

		// Input Layout
		psoDesc.InputLayout = {
			definition.inputLayout.data(),
			static_cast<UINT>(definition.inputLayout.size())
		};

		// 렌더 상태들
		psoDesc.BlendState = definition.blendDesc;
		psoDesc.RasterizerState = definition.rasterizerDesc;
		psoDesc.DepthStencilState = definition.depthStencilDesc;
		psoDesc.PrimitiveTopologyType = definition.topologyType;

		// 출력 형식
		psoDesc.NumRenderTargets = static_cast<UINT>(definition.rtvFormats.size());
		for (size_t i = 0; i < definition.rtvFormats.size(); ++i) {
			psoDesc.RTVFormats[i] = definition.rtvFormats[i];
		}
		psoDesc.DSVFormat = definition.dsvFormat;

		// MSAA 설정
		psoDesc.SampleDesc.Count = 1;  // SwapChain에서 가져오도록 개선 가능
		psoDesc.SampleDesc.Quality = 0;

		psoDesc.SampleMask = UINT_MAX;
		psoDesc.NodeMask = 0;
		psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		// PSO 생성
		ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeline.pipelineState)));
	}

	// 셰이더 리플렉션을 통한 자동 Root Parameter 생성
	std::vector<CD3DX12_ROOT_PARAMETER> GenerateRootParametersFromShader(
		const std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>>& compiledShaders,
		CompiledPipeline& pipeline) {
		
		std::vector<CD3DX12_ROOT_PARAMETER> rootParams;
		std::map<UINT, D3D12_SHADER_INPUT_BIND_DESC> cbvBindings;
		std::map<UINT, D3D12_SHADER_INPUT_BIND_DESC> srvBindings;
		std::map<UINT, D3D12_SHADER_INPUT_BIND_DESC> uavBindings;
		std::vector<D3D12_DESCRIPTOR_RANGE> descriptorRanges;
		
		// 바인딩 정보 초기화
		pipeline.rootParameterBindings.clear();

		// 모든 셰이더 스테이지에서 리소스 바인딩 정보 수집
		for (const auto& [stageName, shaderBlob] : compiledShaders) {
			if (!shaderBlob) continue;

			Microsoft::WRL::ComPtr<ID3D12ShaderReflection> reflection;
			HRESULT hr = D3DReflect(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), 
								   IID_PPV_ARGS(&reflection));
			if (FAILED(hr)) continue;

			D3D12_SHADER_DESC shaderDesc;
			reflection->GetDesc(&shaderDesc);
			if (shaderDesc.BoundResources == 0) {
				// 디버그 메시지
				LOG_INFO("{}", "Shader stage " + stageName + " has no bound resources");
				continue;
			}
			// 각 리소스 바인딩 정보 분석
			for (UINT i = 0; i < shaderDesc.BoundResources; ++i) {
				D3D12_SHADER_INPUT_BIND_DESC bindDesc;
				reflection->GetResourceBindingDesc(i, &bindDesc);

				switch (bindDesc.Type) {
				case D3D_SIT_CBUFFER:
					if (cbvBindings.find(bindDesc.BindPoint) == cbvBindings.end()) {
						cbvBindings[bindDesc.BindPoint] = bindDesc;
					}
					break;

				case D3D_SIT_TEXTURE:
				case D3D_SIT_STRUCTURED:
				case D3D_SIT_BYTEADDRESS:
					if (srvBindings.find(bindDesc.BindPoint) == srvBindings.end()) {
						srvBindings[bindDesc.BindPoint] = bindDesc;
					}
					break;

				case D3D_SIT_UAV_RWTYPED:
				case D3D_SIT_UAV_RWSTRUCTURED:
				case D3D_SIT_UAV_RWBYTEADDRESS:
				case D3D_SIT_UAV_APPEND_STRUCTURED:
				case D3D_SIT_UAV_CONSUME_STRUCTURED:
				case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
					if (uavBindings.find(bindDesc.BindPoint) == uavBindings.end()) {
						uavBindings[bindDesc.BindPoint] = bindDesc;
					}
					break;
				}
			}
		}

		UINT currentRootParameterIndex = 0;

		// CBV들을 Root Parameter로 추가 (개별 CBV는 Root Descriptor로)
		for (const auto& [bindPoint, bindDesc] : cbvBindings) {
			CD3DX12_ROOT_PARAMETER param;
			param.InitAsConstantBufferView(bindPoint, bindDesc.Space);
			rootParams.push_back(param);
			
			// 매핑 정보 저장
			RootParameterBinding binding;
			binding.rootParameterIndex = currentRootParameterIndex++;
			binding.type = D3D12_ROOT_PARAMETER_TYPE_CBV;
			binding.registerSlot = bindPoint;
			binding.registerSpace = bindDesc.Space;
			binding.resourceName = bindDesc.Name; // 실제 셰이더 리소스 이름 사용
			pipeline.rootParameterBindings.push_back(binding);
		}

		// SRV들 처리 - 개별 SRV는 Root Descriptor로, 배열은 Descriptor Table로
		for (const auto& [bindPoint, bindDesc] : srvBindings) {
			RootParameterBinding binding;
			binding.rootParameterIndex = currentRootParameterIndex++;
			binding.registerSlot = bindPoint;
			binding.registerSpace = bindDesc.Space;
			binding.resourceName = bindDesc.Name; // 실제 셰이더 리소스 이름 사용
			
			if (bindDesc.BindCount == 1) {
				// 단일 SRV - Root Descriptor 사용
				CD3DX12_ROOT_PARAMETER param;
				param.InitAsShaderResourceView(bindPoint, bindDesc.Space);
				rootParams.push_back(param);
				
				binding.type = D3D12_ROOT_PARAMETER_TYPE_SRV;
			} else {
				// SRV 배열 - Descriptor Table 사용
				D3D12_DESCRIPTOR_RANGE range;
				range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
				range.NumDescriptors = bindDesc.BindCount;
				range.BaseShaderRegister = bindPoint;
				range.RegisterSpace = bindDesc.Space;
				range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
				
				descriptorRanges.push_back(range);
				
				CD3DX12_ROOT_PARAMETER param;
				param.InitAsDescriptorTable(1, &descriptorRanges.back());
				rootParams.push_back(param);
				
				binding.type = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			}
			pipeline.rootParameterBindings.push_back(binding);
		}

		// UAV들 처리
		for (const auto& [bindPoint, bindDesc] : uavBindings) {
			RootParameterBinding binding;
			binding.rootParameterIndex = currentRootParameterIndex++;
			binding.registerSlot = bindPoint;
			binding.registerSpace = bindDesc.Space;
			binding.resourceName = bindDesc.Name; // 실제 셰이더 리소스 이름 사용
			
			if (bindDesc.BindCount == 1) {
				CD3DX12_ROOT_PARAMETER param;
				param.InitAsUnorderedAccessView(bindPoint, bindDesc.Space);
				rootParams.push_back(param);
				
				binding.type = D3D12_ROOT_PARAMETER_TYPE_UAV;
			} else {
				// UAV 배열 - Descriptor Table 사용
				D3D12_DESCRIPTOR_RANGE range;
				range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
				range.NumDescriptors = bindDesc.BindCount;
				range.BaseShaderRegister = bindPoint;
				range.RegisterSpace = bindDesc.Space;
				range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
				
				descriptorRanges.push_back(range);
				
				CD3DX12_ROOT_PARAMETER param;
				param.InitAsDescriptorTable(1, &descriptorRanges.back());
				rootParams.push_back(param);
				
				binding.type = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			}
			pipeline.rootParameterBindings.push_back(binding);
		}

		return rootParams;
	}

	// Input Layout 자동 생성 (제한적)
	std::vector<D3D12_INPUT_ELEMENT_DESC> GenerateInputLayoutFromShader(
		const std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>>& compiledShaders) {
		
		std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
		
		// Vertex Shader에서만 Input Signature 분석
		auto vsIt = compiledShaders.find("VS");
		if (vsIt == compiledShaders.end() || !vsIt->second) {
			return inputLayout; // VS가 없으면 빈 레이아웃 반환
		}

		Microsoft::WRL::ComPtr<ID3D12ShaderReflection> reflection;
		HRESULT hr = D3DReflect(vsIt->second->GetBufferPointer(), vsIt->second->GetBufferSize(), 
							   IID_PPV_ARGS(&reflection));
		if (FAILED(hr)) return inputLayout;

		D3D12_SHADER_DESC shaderDesc;
		reflection->GetDesc(&shaderDesc);

		UINT currentOffset = 0;
		
		// Input Parameters 분석
		for (UINT i = 0; i < shaderDesc.InputParameters; ++i) {
			D3D12_SIGNATURE_PARAMETER_DESC inputDesc;
			reflection->GetInputParameterDesc(i, &inputDesc);
			
			D3D12_INPUT_ELEMENT_DESC element = {};
			element.SemanticName = inputDesc.SemanticName;
			element.SemanticIndex = inputDesc.SemanticIndex;
			element.InputSlot = 0; // 기본값: 모든 데이터가 하나의 버퍼에서
			element.AlignedByteOffset = currentOffset;
			element.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			element.InstanceDataStepRate = 0;
			
			// ComponentType과 Mask를 기반으로 Format 결정
			element.Format = DetermineInputFormat(inputDesc.ComponentType, inputDesc.Mask);
			
			// 다음 요소의 오프셋 계산
			currentOffset += GetFormatSize(element.Format);
			
			inputLayout.push_back(element);
		}
		
		return inputLayout;
	}

	// 헬퍼 함수들
	DXGI_FORMAT DetermineInputFormat(D3D_REGISTER_COMPONENT_TYPE componentType, BYTE mask) const {
		// Mask: 1=x, 3=xy, 7=xyz, 15=xyzw
		UINT componentCount = 0;
		if (mask & 1) componentCount++;
		if (mask & 2) componentCount++;
		if (mask & 4) componentCount++;
		if (mask & 8) componentCount++;
		
		switch (componentType) {
		case D3D_REGISTER_COMPONENT_FLOAT32:
			switch (componentCount) {
			case 1: return DXGI_FORMAT_R32_FLOAT;
			case 2: return DXGI_FORMAT_R32G32_FLOAT;
			case 3: return DXGI_FORMAT_R32G32B32_FLOAT;
			case 4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			break;
		case D3D_REGISTER_COMPONENT_UINT32:
			switch (componentCount) {
			case 1: return DXGI_FORMAT_R32_UINT;
			case 2: return DXGI_FORMAT_R32G32_UINT;
			case 3: return DXGI_FORMAT_R32G32B32_UINT;
			case 4: return DXGI_FORMAT_R32G32B32A32_UINT;
			}
			break;
		case D3D_REGISTER_COMPONENT_SINT32:
			switch (componentCount) {
			case 1: return DXGI_FORMAT_R32_SINT;
			case 2: return DXGI_FORMAT_R32G32_SINT;
			case 3: return DXGI_FORMAT_R32G32B32_SINT;
			case 4: return DXGI_FORMAT_R32G32B32A32_SINT;
			}
			break;
		}
		
		// 기본값: float3
		return DXGI_FORMAT_R32G32B32_FLOAT;
	}

	UINT GetFormatSize(DXGI_FORMAT format) const {
		switch (format) {
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
			return 4;
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
			return 8;
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			return 12;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return 16;
		default:
			return 16; // 기본값
		}
	}

	inline void CreateRootSignatureFromDesc(const CD3DX12_ROOT_SIGNATURE_DESC& desc, Microsoft::WRL::ComPtr<ID3D12RootSignature>& outSig) {
		Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
		if (errorBlob) {
			// 에러 출력 (간단화)
		}
		ThrowIfFailed(hr);
		ThrowIfFailed(mDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(outSig.GetAddressOf())));
	}

	static std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers() {
		return {
			CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP),
			CD3DX12_STATIC_SAMPLER_DESC(1, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP),
			CD3DX12_STATIC_SAMPLER_DESC(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP),
			CD3DX12_STATIC_SAMPLER_DESC(3, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP),
			CD3DX12_STATIC_SAMPLER_DESC(4, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0.0f, 8),
			CD3DX12_STATIC_SAMPLER_DESC(5, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 0.0f, 8),
			CD3DX12_STATIC_SAMPLER_DESC(6, D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER, 0.0f, 16, D3D12_COMPARISON_FUNC_LESS_EQUAL, D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK)
		};
	}
};