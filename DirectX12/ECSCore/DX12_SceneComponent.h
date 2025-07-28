#pragma once
#include "DX12_Config.h"
#include "InstanceComponent.h"
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
	std::vector<InstanceComponent> Instances;
	ECS::RepoHandle GeometryHandle;
	ECS::RepoHandle MeshHandle;
	uint32_t NumFramesDirty = APP_NUM_BACK_BUFFERS;
	eCFGRenderItem Option = eCFGRenderItem::None;
	eRenderLayer TargetLayer = eRenderLayer::None;
};

struct InstanceKey
{
	int32_t RenderItemIndex;
	int32_t InstanceIndex;
};