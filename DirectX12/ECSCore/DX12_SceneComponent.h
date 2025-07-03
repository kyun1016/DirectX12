#pragma once
#include "DX12_Config.h"
#include "DX12_InstanceComponent.h"
#include "DX12_MeshSystem.h"
#include "DX12_PSOSystem.h"


enum class eCFGRenderItem : std::uint32_t
{
	None = 0,
	FrustumCullingEnabled = 1 << 0
};
ENUM_OPERATORS_32(eCFGRenderItem)
struct RenderItem
{
	RenderItem() = default;
	RenderItem(const RenderItem& rhs) = delete;
	RenderItem(eCFGRenderItem option)
		: Option(option)
	{
	}

	void Push(const DX12_MeshHandle& meshHandle, const float3& translation = {}, const float3& scale = { 1.0f, 1.0f, 1.0f }, const DirectX::SimpleMath::Quaternion& rot = { 0.0f, 0.0f, 0.0f, 1.0f }, const float3& texScale = { 1.0f, 1.0f, 1.0f }, UINT matIdx = 0)
	{
		Instances.push_back(InstanceComponent(meshHandle, translation, scale, rot, texScale, matIdx));
	}

	void Push(InstanceComponent data)
	{
		Instances.push_back(data);
	}

	int NumFramesDirty = APP_NUM_BACK_BUFFERS;
	std::vector<InstanceComponent> Instances;
	eCFGRenderItem Option = eCFGRenderItem::FrustumCullingEnabled;
	
	// // TODO: 
	// // Skinned Mesh
	// UINT SkinnedCBIndex = -1;
	// SkinnedModelInstance* SkinnedModelInst = nullptr;
};