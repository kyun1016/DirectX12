#pragma once
#include "DX12_Config.h"
#include "ECSSystem.h"

class InputLayoutSystem : public ECS::ISystem
{
public:
	void Initialize() 
	{
		std::vector<D3D12_INPUT_ELEMENT_DESC> layout;

		D3D12_INPUT_ELEMENT_DESC
		{
			/* LPCSTR						*/ .SemanticName = "POSITION",
			/* UINT							*/ .SemanticIndex = 0,
			/* DXGI_FORMAT					*/ .Format = DXGI_FORMAT_R32G32B32_FLOAT,
			/* UINT							*/ .InputSlot = 0,
			/* UINT							*/ .AlignedByteOffset = 0,
			/* D3D12_INPUT_CLASSIFICATION	*/ .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			/* UINT							*/ .InstanceDataStepRate = 0
		}

		RegisterLayout("mainInput", )
	}

	void RegisterLayout(const std::string& name, const std::vector<D3D12_INPUT_ELEMENT_DESC>& layout)
	{
		mLayouts[name] = layout;
	}

	const std::vector<D3D12_INPUT_ELEMENT_DESC>& GetLayout(const std::string& name) const
	{
		auto it = mLayouts.find(name);
		if (it != mLayouts.end())
			return it->second;
		static std::vector<D3D12_INPUT_ELEMENT_DESC> empty;
		return empty;
	}

private:
	std::unordered_map<std::string, std::vector<D3D12_INPUT_ELEMENT_DESC>> mLayouts;
};