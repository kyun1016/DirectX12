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
		mAllRenderItems.emplace_back(std::make_unique<RenderItem>());
		mAllRenderItems.back()->Option = eCFGRenderItem::None;
		mAllRenderItems.back()->TargetLayer = eRenderLayer::Test;
		mAllRenderItems.back()->Push({ 2, 0 });
		mAllRenderItems.emplace_back(std::make_unique<RenderItem>());
		mAllRenderItems.back()->Option = eCFGRenderItem::None;
		mAllRenderItems.back()->TargetLayer = eRenderLayer::Sprite;
		mAllRenderItems.back()->Push({ 1, 0 });
		mAllRenderItems.back()->Push({ 1, 0 });
		mAllRenderItems.back()->Push({ 1, 0 });
	}

	const std::vector<std::unique_ptr<RenderItem>>& GetRenderItems() const
	{
		return mAllRenderItems;
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
					DX12_MeshHandle meshHandle = {static_cast<ECS::RepoHandle>(geoIdx), meshIdx};
					auto* meshComponent = DX12_MeshSystem::GetInstance().GetMeshComponent(meshHandle);
					meshComponent->StartInstanceLocation = visibleInstanceCount;
					instanceID.BaseInstanceIndex = visibleInstanceCount;
					currInstanceCB->CopyData(static_cast<int>(meshIdx), instanceID);	// 해당 구문의 이유는 Draw Call 간 Base InstanceIndex 전달에 버그가 존재하기 때문
																						 // Geometry의 Mesh 별로 관리 적용
					for(const auto& instanceIdx : ri->MeshIndex[geoIdx][meshIdx])
					{
						auto& instance = ri->Instances[instanceIdx];
						if (!(ri->Option & eCFGRenderItem::FrustumCullingEnabled)
						|| !(instance.Option & eCFGInstanceComponent::UseCulling)
						|| (CameraSystem::GetInstance().GetCamera(0)->Frustum.Contains(instance.BoundingSphere) != DirectX::DISJOINT))
						{
							currInstanceBuffer->CopyData(visibleInstanceCount++, instance.InstanceData);
						}
					}
					meshComponent->InstanceCount = visibleInstanceCount - meshComponent->StartInstanceLocation;
				}
			}
		}
    }

	void UpdateInstance()
	{
		for (auto& ri : mAllRenderItems)
			for (auto& instance : ri->Instances)
				if(instance.UpdateTransform())
					ri->NumFramesDirty = APP_NUM_BACK_BUFFERS; // Update dirty flag for the render item
	}
private:
	std::vector<std::unique_ptr<RenderItem>> mAllRenderItems;
};