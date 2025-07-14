#pragma once
#include "ECSCoordinator.h"
#include "DX12_PSOSystem.h"
#include "DX12_TransformSystem.h"
#include "DX12_MeshSystem.h"
#include "DX12_SceneComponent.h"
#include "DX12_FrameResourceSystem.h"
#include "CameraSystem.h"

class DX12_SceneSystem {
	DEFAULT_SINGLETON(DX12_SceneSystem)
public:
    void Update()
    {
		UpdateInstance();
        SyncData();	// View
    }

	void Initialize()
	{
		const auto& meshCount = DX12_MeshSystem::GetInstance().GetMeshCount();
		MeshIndexMap.resize(meshCount.size());
		for (size_t i = 0; i < MeshIndexMap.size(); ++i)
			MeshIndexMap[i].resize(meshCount[i]);
		RenderItem ri;
		ri.Instance.MeshHandle = { 2, 0 };
		ri.TargetLayer = eRenderLayer::Opaque;
		Push(ri);

		ri.TargetLayer = eRenderLayer::Sprite;
		ri.Instance.MeshHandle = { 1, 2 };
		Push(ri);
		ri.Instance.MeshHandle = { 1, 1 };
		Push(ri);
		Push(ri);
		ri.Instance.MeshHandle = { 1, 0 };
		Push(ri);
		Push(ri);
		Push(ri);
	}

	void Push(const RenderItem& renderItem)
	{
		mAllRenderItems.emplace_back(renderItem);
		MeshIndexMap[renderItem.Instance.MeshHandle.GeometryHandle][renderItem.Instance.MeshHandle.MeshHandle].push_back(static_cast<uint32_t>(mAllRenderItems.size() - 1));
	}

	const std::vector<RenderItem>& GetRenderItems() const
	{
		return mAllRenderItems;
	}
	const std::vector<std::vector<std::vector<uint32_t>>>& GetMeshIndexMap() const
	{
		return MeshIndexMap;
	}

private:
    void SyncData()
    {
		auto currInstanceBuffer = DX12_FrameResourceSystem::GetInstance().GetCurrentFrameResource().InstanceDataBuffer.get();
		auto currInstanceCB = DX12_FrameResourceSystem::GetInstance().GetCurrentFrameResource().InstanceIDCB.get();
		InstanceIDData instanceID;
		uint32_t visibleInstanceCount = 0;
		size_t totalMeshIdx = 0;
		for (size_t geoIdx = 1; geoIdx < MeshIndexMap.size(); ++geoIdx)
		{
			if (MeshIndexMap[geoIdx].empty())
				continue;
			for (size_t meshIdx = 0; meshIdx < MeshIndexMap[geoIdx].size(); ++meshIdx)
			{
				DX12_MeshHandle meshHandle = { static_cast<ECS::RepoHandle>(geoIdx), meshIdx };
				auto* meshComponent = DX12_MeshSystem::GetInstance().GetMeshComponent(meshHandle);
				meshComponent->StartInstanceLocation = visibleInstanceCount;
				instanceID.BaseInstanceIndex = visibleInstanceCount;
				currInstanceCB->CopyData(static_cast<int>(totalMeshIdx++), instanceID);	// 해당 구문의 이유는 Draw Call 간 Base InstanceIndex 전달에 버그가 존재하기 때문
				for (const auto& instanceIdx : MeshIndexMap[geoIdx][meshIdx])
				{
					auto& ri = mAllRenderItems[instanceIdx];
					if(--ri.NumFramesDirty == 0)
						continue;
					if (!(ri.Option & eCFGRenderItem::FrustumCullingEnabled)
						|| !(ri.Instance.Option & eCFGInstanceComponent::UseCulling)
						|| (CameraSystem::GetInstance().GetCamera(0)->Frustum.Contains(ri.Instance.BoundingSphere) != DirectX::DISJOINT))
					{
						currInstanceBuffer->CopyData(visibleInstanceCount++, ri.Instance.InstanceData);
					}
				}
				meshComponent->InstanceCount = visibleInstanceCount - meshComponent->StartInstanceLocation;
			}
		}
    }

	void UpdateInstance()
	{
		for (auto& ri : mAllRenderItems)
			if (ri.Instance.UpdateTransform())
				ri.NumFramesDirty = APP_NUM_BACK_BUFFERS;
					
	}
private:
	std::vector<RenderItem> mAllRenderItems;
	std::vector<std::vector<std::vector<uint32_t>>> MeshIndexMap;	// [GeometryHandle][MeshHandle] = [InstanceIndex]
};