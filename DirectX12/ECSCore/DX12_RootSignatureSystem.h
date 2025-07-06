#pragma once
#include "DX12_Config.h"

class DX12_RootSignatureSystem {
	DEFAULT_SINGLETON(DX12_RootSignatureSystem)
public:
	void Initialize(ID3D12Device* device) {
		mDevice = device;
		BuildExampleRootSignature();	// Example
		BuildSpriteRootSignature();
		BuildTestRootSignature();
	}

	inline ID3D12RootSignature* GetGraphicsSignature(const std::string& name) {
		auto it = mGraphicsSignatures.find(name);
		return (it != mGraphicsSignatures.end()) ? it->second.Get() : nullptr;
	}

	inline ID3D12RootSignature* GetComputeSignature(const std::string& name) {
		auto it = mComputeSignatures.find(name);
		return (it != mComputeSignatures.end()) ? it->second.Get() : nullptr;
	}

	inline void RegisterGraphicsSignature(const std::string& name, const std::vector<CD3DX12_ROOT_PARAMETER>& params, D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT) {
		auto samplers = GetStaticSamplers();
		CD3DX12_ROOT_SIGNATURE_DESC desc((UINT)params.size(), params.data(), (UINT)samplers.size(), samplers.data(), flags);
		CreateSignature(desc, mGraphicsSignatures[name]);
	}

	inline void RegisterComputeSignature(const std::string& name, const std::vector<CD3DX12_ROOT_PARAMETER>& params, D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE) {
		auto samplers = GetStaticSamplers();
		CD3DX12_ROOT_SIGNATURE_DESC desc((UINT)params.size(), params.data(), (UINT)samplers.size(), samplers.data(), flags);
		CreateSignature(desc, mComputeSignatures[name]);
	}

private:
	ID3D12Device* mDevice = nullptr;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12RootSignature>> mGraphicsSignatures;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12RootSignature>> mComputeSignatures;

	void BuildExampleRootSignature()
	{
		std::vector<CD3DX12_ROOT_PARAMETER> param;
		CD3DX12_ROOT_PARAMETER tmp;
		// Perfomance TIP: Order from most frequent to least frequent.
		tmp.InitAsConstantBufferView(0); param.push_back(tmp);		// CBV, gBaseInstanceIndex b0
		tmp.InitAsConstantBufferView(1); param.push_back(tmp);		// CBV, cbPass b1
		tmp.InitAsConstantBufferView(2); param.push_back(tmp);		// CBV, cbSkinned b2
		tmp.InitAsConstantBufferView(3); param.push_back(tmp);		// CBV, cbShaderToy b3
		tmp.InitAsShaderResourceView(0, 0); param.push_back(tmp);	// SRV, InstanceData t0 (Space0)
		tmp.InitAsShaderResourceView(1, 0); param.push_back(tmp);	// SRV, MaterialData t1 (Space0)

		RegisterGraphicsSignature("main", param);
	}

	void BuildSpriteRootSignature()
	{
		std::vector<CD3DX12_ROOT_PARAMETER> param;
		CD3DX12_ROOT_PARAMETER tmp;
		tmp.InitAsConstantBufferView(0); param.push_back(tmp);		// CBV, cbInstanceID b1
		tmp.InitAsShaderResourceView(0, 0); param.push_back(tmp);	// SRV, InstanceData t0 (Space0)
		tmp.InitAsShaderResourceView(1, 0); param.push_back(tmp);	// SRV, CameraData t1 (Space0)
		tmp.InitAsShaderResourceView(2, 0); param.push_back(tmp);	// SRV, LightData t2 (Space0)

		RegisterGraphicsSignature("sprite", param);
	}

	void BuildTestRootSignature()
	{
		std::vector<CD3DX12_ROOT_PARAMETER> param(1);
		// Per-object constants (WVP matrix)
		param[0].InitAsConstantBufferView(0); // b0
		RegisterGraphicsSignature("test", param);
	}
	inline void CreateSignature(const CD3DX12_ROOT_SIGNATURE_DESC& desc, Microsoft::WRL::ComPtr<ID3D12RootSignature>& outSig) {
		Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
		if (errorBlob)
			LOG_ERROR("{}", (char*)errorBlob->GetBufferPointer());
		ThrowIfFailed(hr);
		ThrowIfFailed(mDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(outSig.GetAddressOf())));
	}

	static std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers() {
		/*D3D12_STATIC_SAMPLER_DESC
			{
				D3D12_FILTER Filter;
				D3D12_TEXTURE_ADDRESS_MODE AddressU;
				D3D12_TEXTURE_ADDRESS_MODE AddressV;
				D3D12_TEXTURE_ADDRESS_MODE AddressW;
				FLOAT MipLODBias;
				UINT MaxAnisotropy;
				D3D12_COMPARISON_FUNC ComparisonFunc;
				D3D12_STATIC_BORDER_COLOR BorderColor;
				FLOAT MinLOD;
				FLOAT MaxLOD;
				UINT ShaderRegister;
				UINT RegisterSpace;
				D3D12_SHADER_VISIBILITY ShaderVisibility;
		}*/
		// shaderRegister, filter, addressU, addressV, addressW
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