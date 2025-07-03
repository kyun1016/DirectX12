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
		for (size_t i = 0; i < mAllRenderItems.size(); ++i)
		{
			auto& ri = mAllRenderItems[i];
			if (ri->Option & eCFGRenderItem::FrustumCullingEnabled)
			{
				instanceID.BaseInstanceIndex = visibleInstanceCount;
				currInstanceCB->CopyData(i, instanceID);
				for (auto& instance : ri->Instances) {
					auto& meshComponent = DX12_MeshSystem::GetInstance().GetMeshComponent(instance.MeshHandle);
					meshComponent.StartInstanceLocation = visibleInstanceCount;
					if (!(instance.Option & eCFGInstanceComponent::UseCulling) || (mCamFrustum.Contains(instance.BoundingSphere) != DirectX::DISJOINT))
					{
						currInstanceBuffer->CopyData(visibleInstanceCount++, instance.InstanceData);
					}
					meshComponent.InstanceCount = visibleInstanceCount - meshComponent.StartInstanceLocation;
				}
			}
			else
			{
				//// Dirty Flag가 존재하는 경우 업데이트 (이동 등 instance 변경 발생 시 NumFramesDirty = APP_NUM_FRAME_RESOURCES)
				//if (ri->NumFramesDirty > 0)
				//{
				//	instanceID.BaseInstanceIndex = visibleInstanceCount;
				//	currInstanceCB->CopyData(i, instanceID);
				//	// Instance 전체 복사
				//	for (const auto& instance : ri->Instances)
				//	{
				//		currInstanceBuffer->CopyData(visibleInstanceCount++, instance.InstanceData);
				//	}
				//	ri->InstanceCount = visibleInstanceCount - ri->StartInstanceLocation;

				//	ri->NumFramesDirty--;
				//}
			}
			
		}
    }

	DirectX::BoundingFrustum mCamFrustum;
	std::vector<std::unique_ptr<RenderItem>> mAllRenderItems;
    std::vector<RenderItem*> mRenderItemLayers[static_cast<int>(eRenderLayer::Count)];
};