#pragma once
#include "ECSCoordinator.h"
#include "DX12_PSOSystem.h"
#include "DX12_TransformSystem.h"
#include "DX12_InstanceSystem.h"
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
		auto currInstanceBuffer = DX12_FrameResourceSystem::GetInstance().GetCurrentFrameResource().InstanceBuffer.get();
		// auto currInstanceCB = mCurrFrameResource->InstanceCB.get();
		uint32_t visibleInstanceCount = 0;
		for (auto& [_, ri]: mAllRenderItems)
		{
			if (ri->FrustumCullingEnabled)
			{
				// InstanceConstants insCB;
				// insCB.BaseInstanceIndex = visibleInstanceCount;
				ri->StartInstanceLocation = visibleInstanceCount;
				// currInstanceCB->CopyData(i, insCB);

				// 컬링을 활용하며, 매 Frame 간 데이터를 복사하는 로직으로 구현
				for (const auto& d : ri->Datas)
				{
					// Cam Frustum에 포함되는 Instance만 복사
					// if ((mCamFrustum.Contains(d.BoundingBox) != DirectX::DISJOINT) || (d.FrustumCullingEnabled == false))
					if ((mCamFrustum.Contains(d.BoundingSphere) != DirectX::DISJOINT) || (d.FrustumCullingEnabled == false))
					{
						// 추후 Frustum을 관리하는 Camera System 구현 필요
						currInstanceBuffer->CopyData(visibleInstanceCount++, d.InstanceData);
					}
				}

				// Instance 개수 업데이트
				ri->InstanceCount = visibleInstanceCount - ri->StartInstanceLocation;
			}
			else
			{
				// Dirty Flag가 존재하는 경우 업데이트 (이동 등 instance 변경 발생 시 NumFramesDirty = APP_NUM_FRAME_RESOURCES)
				if (ri->NumFramesDirty > 0)
				{
					// InstanceConstants insCB;
					// insCB.BaseInstanceIndex = visibleInstanceCount;
					ri->StartInstanceLocation = visibleInstanceCount;
					// currInstanceCB->CopyData(i, insCB);

					// Instance 전체 복사
					for (const auto& d : ri->Datas)
					{
						currInstanceBuffer->CopyData(visibleInstanceCount++, d.InstanceData);
					}
					ri->InstanceCount = visibleInstanceCount - ri->StartInstanceLocation;

					ri->NumFramesDirty--;
				}
			}
			
		}
    }

	DirectX::BoundingFrustum mCamFrustum;
	std::unordered_map<std::string, std::unique_ptr<RenderItem>> mAllRenderItems;
    std::vector<RenderItem*> mRenderItemLayers[static_cast<int>(eRenderLayer::Count)];
};