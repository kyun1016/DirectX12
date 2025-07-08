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
	~RenderItem() = default;
	RenderItem& operator=(const RenderItem&) = delete;
	RenderItem(RenderItem&&) = delete;
	RenderItem& operator=(RenderItem&&) = delete;

	uint32_t NumFramesDirty = APP_NUM_BACK_BUFFERS;
	std::vector<InstanceComponent> Instances;
	std::vector<std::vector<std::vector<uint32_t>>> MeshIndex;
	eCFGRenderItem Option = eCFGRenderItem::None;
	eRenderLayer TargetLayer = eRenderLayer::None;
	
	// // TODO: 
	// // Skinned Mesh
	// UINT SkinnedCBIndex = -1;
	// SkinnedModelInstance* SkinnedModelInst = nullptr;

	void Push(const DX12_MeshHandle& meshHandle, const float3& translation = {}, const float3& scale = { 1.0f, 1.0f, 1.0f }, const DirectX::SimpleMath::Quaternion& rot = { 0.0f, 0.0f, 0.0f, 1.0f }, const float3& texScale = { 1.0f, 1.0f, 1.0f }, UINT matIdx = 0)
	{
		// 1. Mesh 정합성 체크
		if (DX12_MeshSystem::GetInstance().GetMeshComponent(meshHandle) == nullptr)
			return;

		// 2. Update 편의성을 위해 미리 탐색 행렬 제작
		if (MeshIndex.size() <= meshHandle.GeometryHandle)
			MeshIndex.resize(meshHandle.GeometryHandle + 1);	// 이때, 효율성을 위해 미리 
		if (MeshIndex[meshHandle.GeometryHandle].size() <= meshHandle.MeshHandle)
			MeshIndex[meshHandle.GeometryHandle].resize(meshHandle.MeshHandle + 1);
		MeshIndex[meshHandle.GeometryHandle][meshHandle.MeshHandle].emplace_back(Instances.size());

		// 3. Instance 배열 추가
		Instances.push_back(InstanceComponent(meshHandle, translation, scale, rot, texScale, matIdx));
	}

	void Push(InstanceComponent data)
	{
		Instances.push_back(data);
	}
};