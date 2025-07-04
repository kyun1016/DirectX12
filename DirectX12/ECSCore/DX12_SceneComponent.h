#pragma once
#include "DX12_Config.h"
#include "DX12_InstanceComponent.h"
#include "DX12_MeshSystem.h"
#include "DX12_PSOSystem.h"


enum class eCFGSceneComponent : std::uint32_t
{
	None = 0,
	FrustumCullingEnabled = 1 << 0
};
ENUM_OPERATORS_32(eCFGSceneComponent)
struct SceneComponent
{
	SceneComponent() = default;
	SceneComponent(const SceneComponent& rhs) = delete;
	SceneComponent(eCFGSceneComponent option)
		: Option(option)
	{
	}

	void Push(const DX12_MeshHandle& meshHandle, const float3& translation = {}, const float3& scale = { 1.0f, 1.0f, 1.0f }, const DirectX::SimpleMath::Quaternion& rot = { 0.0f, 0.0f, 0.0f, 1.0f }, const float3& texScale = { 1.0f, 1.0f, 1.0f }, UINT matIdx = 0)
	{
		// Mesh 정합성 체크
		if (DX12_MeshSystem::GetInstance().GetMeshComponent(meshHandle) == nullptr)
			return;

		// 1. Instance 배열 추가
		Instances.push_back(InstanceComponent(meshHandle, translation, scale, rot, texScale, matIdx));

		// 2. Update 편의성을 위해 미리 탐색 행렬 제작
		if (MeshHandles.size() <= meshHandle.GeometryHandle)
			MeshHandles.resize(meshHandle.GeometryHandle + 1);	// 이때, 효율성을 위해 미리 
		if (MeshHandles[meshHandle.GeometryHandle].size() <= meshHandle.MeshHandle)
			MeshHandles[meshHandle.GeometryHandle].resize(meshHandle.MeshHandle + 1, 0);
		MeshHandles[meshHandle.GeometryHandle][meshHandle.MeshHandle]++;
	}

	void Push(InstanceComponent data)
	{
		Instances.push_back(data);
	}

	int NumFramesDirty = APP_NUM_BACK_BUFFERS;
	std::vector<InstanceComponent> Instances;
	std::vector<std::vector<uint32_t>> MeshHandles;
	eCFGSceneComponent Option = eCFGSceneComponent::FrustumCullingEnabled;
	
	// // TODO: 
	// // Skinned Mesh
	// UINT SkinnedCBIndex = -1;
	// SkinnedModelInstance* SkinnedModelInst = nullptr;
};