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
	InstanceComponent Instance;
	uint32_t NumFramesDirty = APP_NUM_BACK_BUFFERS;
	eCFGRenderItem Option = eCFGRenderItem::None;
	eRenderLayer TargetLayer = eRenderLayer::None;
};