#pragma once
#include "DX12_Config.h"
#include "ECSSystem.h"

class DX12_RootSignatureSystem : public ECS::ISystem {
public:
	static DX12_RootSignatureSystem& GetInstance() {
		static DX12_RootSignatureSystem instance;
		return instance;
	}

	// Initialize DirectX 12 resources
	inline void Initialize(ID3D12Device* device) {
		mDevice = device;
		BuildRootSignature();
	}
	
private:
	ID3D12Device* mDevice;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12RootSignature>> mGraphicsSignatures;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12RootSignature>> mComputeSignatures;

	void BuildRootSignature()
	{
		//// Create root CBVs.
		//D3D12_DESCRIPTOR_RANGE TexDiffTable // register t0[16] (Space1)
		//{
		//	/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		//	/* UINT NumDescriptors						*/.NumDescriptors = (UINT)mDiffuseTex.size() + SRV_USER_SIZE,
		//	/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		//	/* UINT RegisterSpace						*/.RegisterSpace = 1,
		//	/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
		//};
		//D3D12_DESCRIPTOR_RANGE DisplacementMapTable // register t0 (Space2)
		//{
		//	/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		//	/* UINT NumDescriptors						*/.NumDescriptors = 1,
		//	/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		//	/* UINT RegisterSpace						*/.RegisterSpace = 2,
		//	/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
		//};
		//D3D12_DESCRIPTOR_RANGE DisplacementMapTable2;
		//DisplacementMapTable2.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		//DisplacementMapTable2.NumDescriptors = 1;
		//DisplacementMapTable2.BaseShaderRegister = 0;
		//DisplacementMapTable2.RegisterSpace = 2;
		//DisplacementMapTable2.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		//D3D12_DESCRIPTOR_RANGE DisplacementMapTable3;
		//DisplacementMapTable3 = DisplacementMapTable2;

		//D3D12_DESCRIPTOR_RANGE TexNormTable // register t0[16] (Space3)
		//{
		//	/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		//	/* UINT NumDescriptors						*/.NumDescriptors = (UINT)mNormalTex.size(),
		//	/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		//	/* UINT RegisterSpace						*/.RegisterSpace = 3,
		//	/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
		//};
		//D3D12_DESCRIPTOR_RANGE TexAOTable // register t0 (Space4)
		//{
		//	/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		//	/* UINT NumDescriptors						*/.NumDescriptors = 1,
		//	/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		//	/* UINT RegisterSpace						*/.RegisterSpace = 4,
		//	/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
		//};
		//D3D12_DESCRIPTOR_RANGE TexMetallicTable // register t0 (Space5)
		//{
		//	/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		//	/* UINT NumDescriptors						*/.NumDescriptors = 1,
		//	/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		//	/* UINT RegisterSpace						*/.RegisterSpace = 5,
		//	/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
		//};
		//D3D12_DESCRIPTOR_RANGE TexRoughnessTable // register t0 (Space6)
		//{
		//	/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		//	/* UINT NumDescriptors						*/.NumDescriptors = 1,
		//	/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		//	/* UINT RegisterSpace						*/.RegisterSpace = 6,
		//	/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
		//};
		//D3D12_DESCRIPTOR_RANGE TexEmissiveTable // register t0 (Space7)
		//{
		//	/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		//	/* UINT NumDescriptors						*/.NumDescriptors = 1,
		//	/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		//	/* UINT RegisterSpace						*/.RegisterSpace = 7,
		//	/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
		//};
		//D3D12_DESCRIPTOR_RANGE ShadowMapTable // register t0[0] (Space8)
		//{
		//	/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		//	/* UINT NumDescriptors						*/.NumDescriptors = (UINT)MAX_LIGHTS,
		//	/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		//	/* UINT RegisterSpace						*/.RegisterSpace = 8,
		//	/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
		//};
		//D3D12_DESCRIPTOR_RANGE SsaoMapTable // register t0[0] (Space9)
		//{
		//	/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		//	/* UINT NumDescriptors						*/.NumDescriptors = (UINT)MAX_LIGHTS,
		//	/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		//	/* UINT RegisterSpace						*/.RegisterSpace = 9,
		//	/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
		//};
		//D3D12_DESCRIPTOR_RANGE TexArrayTable // register t0[0] (Space10)
		//{
		//	/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		//	/* UINT NumDescriptors						*/.NumDescriptors = (UINT)mTreeMapTex.size(),
		//	/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		//	/* UINT RegisterSpace						*/.RegisterSpace = 10,
		//	/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
		//};
		//D3D12_DESCRIPTOR_RANGE TexCubeTable // register t0[0] (Space11)
		//{
		//	/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		//	/* UINT NumDescriptors						*/.NumDescriptors = (UINT)mCubeMapTex.size(),
		//	/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		//	/* UINT RegisterSpace						*/.RegisterSpace = 11,
		//	/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
		//};
		//D3D12_DESCRIPTOR_RANGE TexStreamlineTable // register t0[0] (Space12)
		//{
		//	/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		//	/* UINT NumDescriptors						*/.NumDescriptors = (UINT)mStreamlineTex.size(),
		//	/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		//	/* UINT RegisterSpace						*/.RegisterSpace = 12,
		//	/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
		//};

		/*D3D12_SHADER_VISIBILITY
		{
			D3D12_SHADER_VISIBILITY_ALL = 0,
			D3D12_SHADER_VISIBILITY_VERTEX = 1,
			D3D12_SHADER_VISIBILITY_HULL = 2,
			D3D12_SHADER_VISIBILITY_DOMAIN = 3,
			D3D12_SHADER_VISIBILITY_GEOMETRY = 4,
			D3D12_SHADER_VISIBILITY_PIXEL = 5,
			D3D12_SHADER_VISIBILITY_AMPLIFICATION = 6,
			D3D12_SHADER_VISIBILITY_MESH = 7
		} 	D3D12_SHADER_VISIBILITY;*/

		// Root parameter can be a table, root descriptor or root constants.
		CD3DX12_ROOT_PARAMETER slotRootParameter[6];
		// Perfomance TIP: Order from most frequent to least frequent.
		slotRootParameter[0].InitAsConstantBufferView(0);		// gBaseInstanceIndex b0
		slotRootParameter[1].InitAsConstantBufferView(1);		// cbPass b1
		slotRootParameter[2].InitAsConstantBufferView(2);		// cbSkinned b2
		slotRootParameter[3].InitAsConstantBufferView(3);		// cbShaderToy b3
		slotRootParameter[4].InitAsShaderResourceView(0, 0);	// InstanceData t0 (Space0)
		slotRootParameter[5].InitAsShaderResourceView(1, 0);	// MaterialData t1 (Space0)
		//slotRootParameter[6].InitAsDescriptorTable(1, &TexDiffTable, D3D12_SHADER_VISIBILITY_PIXEL);
		//slotRootParameter[7].InitAsDescriptorTable(1, &DisplacementMapTable, D3D12_SHADER_VISIBILITY_VERTEX);
		//slotRootParameter[8].InitAsDescriptorTable(1, &TexNormTable, D3D12_SHADER_VISIBILITY_PIXEL);
		//slotRootParameter[9].InitAsDescriptorTable(1, &TexAOTable, D3D12_SHADER_VISIBILITY_PIXEL);
		//slotRootParameter[10].InitAsDescriptorTable(1, &TexMetallicTable, D3D12_SHADER_VISIBILITY_PIXEL);
		//slotRootParameter[11].InitAsDescriptorTable(1, &TexRoughnessTable, D3D12_SHADER_VISIBILITY_PIXEL);
		//slotRootParameter[12].InitAsDescriptorTable(1, &TexEmissiveTable, D3D12_SHADER_VISIBILITY_PIXEL);
		//slotRootParameter[13].InitAsDescriptorTable(1, &ShadowMapTable, D3D12_SHADER_VISIBILITY_PIXEL);
		//slotRootParameter[14].InitAsDescriptorTable(1, &SsaoMapTable, D3D12_SHADER_VISIBILITY_PIXEL);
		//slotRootParameter[15].InitAsDescriptorTable(1, &TexArrayTable, D3D12_SHADER_VISIBILITY_PIXEL);
		//slotRootParameter[16].InitAsDescriptorTable(1, &TexCubeTable, D3D12_SHADER_VISIBILITY_PIXEL);
		// slotRootParameter[17].InitAsDescriptorTable(1, &TexStreamlineTable, D3D12_SHADER_VISIBILITY_PIXEL);

		auto staticSamplers = GetStaticSamplers();

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(std::size(slotRootParameter), slotRootParameter, (UINT)staticSamplers.size(),
			staticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
		Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(
			&rootSigDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(),
			errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		ThrowIfFailed(hr);

		ThrowIfFailed(mDevice->CreateRootSignature(0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(mRootSignature.GetAddressOf())));
	}

	static std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers()
	{
		// Applications usually only need a handful of samplers.  So just define them all up front
		// and keep them available as part of the root signature.  

		const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
			0, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

		const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
			1, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

		const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
			2, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

		const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
			3, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

		const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
			4, // shaderRegister
			D3D12_FILTER_ANISOTROPIC, // filter
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
			0.0f,                             // mipLODBias
			8);                               // maxAnisotropy

		const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
			5, // shaderRegister
			D3D12_FILTER_ANISOTROPIC, // filter
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
			0.0f,                              // mipLODBias
			8);                                // maxAnisotropy

		const CD3DX12_STATIC_SAMPLER_DESC shadow(
			6, // shaderRegister
			D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
			0.0f,                               // mipLODBias
			16,                                 // maxAnisotropy
			D3D12_COMPARISON_FUNC_LESS_EQUAL,
			D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

		return {
			pointWrap, pointClamp,
			linearWrap, linearClamp,
			anisotropicWrap, anisotropicClamp,
			shadow
		};
	}
};