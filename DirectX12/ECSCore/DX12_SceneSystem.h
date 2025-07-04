#pragma once
#include "ECSCoordinator.h"
#include "DX12_PSOSystem.h"
#include "DX12_TransformSystem.h"
#include "DX12_InstanceSystem.h"
#include "DX12_MeshSystem.h"
#include "DX12_SceneComponent.h"
#include "DX12_FrameResourceSystem.h"

class DX12_SceneSystem {
	DEFAULT_SINGLETON(DX12_SceneSystem)
public:
    void Update()
    {
        SyncData();	// View 
    }

    // DX12_RenderSystem이 렌더링할 아이템 목록을 가져갈 수 있도록 getter 제공
    const std::vector<RenderItem*>& GetRenderItems(eRenderLayer layer) const
    {
        return mRenderItemLayers[static_cast<int>(layer)];
    }

private:
    void SyncData()
    {
		auto currInstanceBuffer = DX12_FrameResourceSystem::GetInstance().GetCurrentFrameResource().InstanceDataBuffer.get();
		auto currInstanceCB = DX12_FrameResourceSystem::GetInstance().GetCurrentFrameResource().InstanceIDCB.get();
		uint32_t visibleInstanceCount = 0;
		InstanceIDData instanceID;
		for (auto& ri : mAllRenderItems)
		{
			if (!(ri->Option & eCFGRenderItem::FrustumCullingEnabled) && ri->NumFramesDirty == 0)
				continue;
			ri->NumFramesDirty--;
			for (size_t geoIdx = 1; geoIdx < ri->MeshIndex.size(); ++geoIdx)
			{
				for (size_t meshIdx = 0; meshIdx < ri->MeshIndex[geoIdx].size(); ++meshIdx)
				{
					DX12_MeshHandle meshHandle = {geoIdx, meshIdx};
					auto* meshComponent = DX12_MeshSystem::GetInstance().GetMeshComponent(meshHandle);
					meshComponent->StartInstanceLocation = visibleInstanceCount;
					instanceID.BaseInstanceIndex = visibleInstanceCount;
					currInstanceCB->CopyData(meshIdx, instanceID);	// 해당 구문의 이유는 Draw Call 간 Base InstanceIndex 전달에 버그가 존재하기 때문
																	// Geometry의 Mesh 별로 관리 적용
					for(const auto& instanceIdx : ri->MeshIndex[geoIdx][meshIdx])
					{
						auto& instance = ri->Instances[instanceIdx];
						if (!(ri->Option & eCFGRenderItem::FrustumCullingEnabled)
						|| !(instance.Option & eCFGInstanceComponent::UseCulling)
						|| (mCamFrustum.Contains(instance.BoundingSphere) != DirectX::DISJOINT))
						{
							currInstanceBuffer->CopyData(visibleInstanceCount++, instance.InstanceData);
						}
					}
					meshComponent->InstanceCount = visibleInstanceCount - meshComponent->StartInstanceLocation;
				}
			}
		}
    }

	DirectX::BoundingFrustum mCamFrustum;
	std::vector<std::unique_ptr<RenderItem>> mAllRenderItems;
    std::vector<RenderItem*> mRenderItemLayers[static_cast<int>(eRenderLayer::Count)];
};