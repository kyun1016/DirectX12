#pragma once
#include "DX12_Config.h"
#include "ECSSystem.h"

class DX12_InputLayoutSystem
{
DEFAULT_SINGLETON(DX12_InputLayoutSystem)
public:

	inline void Initialize()
	{
		//D3D12_INPUT_ELEMENT_DESC desc
		//{
		//	/* LPCSTR						*/ .SemanticName = "POSITION",
		//	/* UINT							*/ .SemanticIndex = 0,
		//	/* DXGI_FORMAT					*/ .Format = DXGI_FORMAT_R32G32B32_FLOAT,
		//	/* UINT							*/ .InputSlot = 0,
		//	/* UINT							*/ .AlignedByteOffset = 0,
		//	/* D3D12_INPUT_CLASSIFICATION	*/ .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		//	/* UINT							*/ .InstanceDataStepRate = 0
		//};

		std::vector<D3D12_INPUT_ELEMENT_DESC> mainInputLayout =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
		std::vector<D3D12_INPUT_ELEMENT_DESC> skinnedInputLayout =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "WEIGHTS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BONEINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, 56, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		std::vector<D3D12_INPUT_ELEMENT_DESC> treeSpriteInputLayout =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		RegisterLayout("main", mainInputLayout);
		RegisterLayout("skinned", skinnedInputLayout);
		RegisterLayout("treeSprite", treeSpriteInputLayout);
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