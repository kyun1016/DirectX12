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
		SyncInstanceIDData();
        SyncData();	// View
    }

	void Initialize()
	{
		const auto& meshCount = DX12_MeshSystem::GetInstance().GetMeshCount();

		RenderItem ri;
		ri.TargetLayer = eRenderLayer::Opaque;
		ri.MeshHandle = { 2, 0 };
		Push(ri);
		Push(mAllRenderItems.size() - 1, InstanceComponent());

		ri.TargetLayer = eRenderLayer::Sprite;
		ri.MeshHandle = { 1, 0 };
		Push(ri);
		Push(mAllRenderItems.size() - 1, InstanceComponent());
		Push(mAllRenderItems.size() - 1, InstanceComponent());
		Push(mAllRenderItems.size() - 1, InstanceComponent());

		ri.MeshHandle = { 1, 1 };
		Push(ri);
		Push(mAllRenderItems.size() - 1, InstanceComponent());
		Push(mAllRenderItems.size() - 1, InstanceComponent());

		ri.MeshHandle = { 1, 2 };
		Push(ri);
		Push(mAllRenderItems.size() - 1, InstanceComponent());
		Push(mAllRenderItems.size() - 1, InstanceComponent());
		Push(mAllRenderItems.size() - 1, InstanceComponent());

		ri.MeshHandle = { 1, 3 };
		Push(ri);
		Push(mAllRenderItems.size() - 1, InstanceComponent());
		Push(mAllRenderItems.size() - 1, InstanceComponent());

		ri.MeshHandle = { 1, 4 };
		Push(ri);
		Push(mAllRenderItems.size() - 1, InstanceComponent());
		Push(mAllRenderItems.size() - 1, InstanceComponent());
		Push(mAllRenderItems.size() - 1, InstanceComponent());

		ri.MeshHandle = { 1, 5 };
		Push(ri);
		Push(mAllRenderItems.size() - 1, InstanceComponent());
		Push(mAllRenderItems.size() - 1, InstanceComponent());
	}

	void Push(const RenderItem& renderItem)
	{
		mAllRenderItems.emplace_back(renderItem);
	}

	void Push(const size_t& idx, const InstanceComponent& data)
	{
		mNumFramesDirty = APP_NUM_BACK_BUFFERS;
		auto& ri = mAllRenderItems[idx];
		ri.Instances.emplace_back(data);
		ri.Instances.back().MeshHandle = ri.MeshHandle;
	}

	const std::vector<RenderItem>& GetRenderItems() const
	{
		return mAllRenderItems;
	}

private:
    void SyncData()
    {
		auto currInstanceBuffer = DX12_FrameResourceSystem::GetInstance().GetCurrentFrameResource().InstanceDataBuffer.get();
		uint32_t visibleInstanceCount = 0;
		for (auto& ri : mAllRenderItems)
		{
			if (!(ri.Option & eCFGRenderItem::FrustumCullingEnabled) && ri.NumFramesDirty == 0)
				continue;
			--ri.NumFramesDirty;
			
			auto* meshComponent = DX12_MeshSystem::GetInstance().GetMeshComponent(ri.MeshHandle);
			visibleInstanceCount = meshComponent->StartInstanceLocation;
			for (const auto& instance : ri.Instances)
			{
				if (!(ri.Option & eCFGRenderItem::FrustumCullingEnabled)
					|| !(instance.Option & eCFGInstanceComponent::UseCulling)
					|| (CameraSystem::GetInstance().GetCamera(0)->Frustum.Contains(instance.BoundingSphere) != DirectX::DISJOINT))
				{
					currInstanceBuffer->CopyData(visibleInstanceCount++, instance.InstanceData);
				}
			}
			meshComponent->InstanceCount = visibleInstanceCount - meshComponent->StartInstanceLocation;
		}
    }

	void SyncInstanceIDData()
	{
		if (mNumFramesDirty == 0)
			return;

		--mNumFramesDirty;
		auto currInstanceCB = DX12_FrameResourceSystem::GetInstance().GetCurrentFrameResource().InstanceIDCB.get();
		InstanceIDData instanceID;
		instanceID.BaseInstanceIndex = 0;
		for (int i = 0; i < mAllRenderItems.size(); ++i)
		{
			auto& ri = mAllRenderItems[i];
			DX12_MeshSystem::GetInstance().GetMeshComponent(ri.MeshHandle)->StartInstanceLocation = instanceID.BaseInstanceIndex;
			currInstanceCB->CopyData(i, instanceID);	// 해당 구문의 이유는 Draw Call 간 Base InstanceIndex 전달에 버그가 존재하기 때문
			instanceID.BaseInstanceIndex += ri.Instances.size();
		}
	}

	void UpdateInstance()
	{
		for (auto& ri : mAllRenderItems)
			for (auto& instance : ri.Instances)
				if(instance.UpdateTransform())
					ri.NumFramesDirty = APP_NUM_BACK_BUFFERS;
	}
private:
	uint32_t mNumFramesDirty = APP_NUM_BACK_BUFFERS;
	std::vector<RenderItem> mAllRenderItems;
};