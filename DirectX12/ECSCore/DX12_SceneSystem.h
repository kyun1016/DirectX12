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
    void Update(UploadBuffer<InstanceData>* instanceDataBuffer, UploadBuffer<InstanceIDData>* instanceIDBuffer)
    {
		UpdateInstance();
		SyncInstanceIDData(instanceIDBuffer);
        SyncData(instanceDataBuffer);	// View
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

	InstanceComponent* GetInstance(const InstanceKey& key)
	{
		if (key.RenderItemIndex >= mAllRenderItems.size())
			return nullptr;
		if (key.InstanceIndex >= mAllRenderItems[key.RenderItemIndex].Instances.size())
			return nullptr;
		
		return &mAllRenderItems[key.RenderItemIndex].Instances[key.InstanceIndex];
	}

	void UpdateInstance(const InstanceKey& key, InputSystem& input)
	{
		if (key.RenderItemIndex >= mAllRenderItems.size())
			return;
		if (key.InstanceIndex >= mAllRenderItems[key.RenderItemIndex].Instances.size())
			return;
		auto& transform = mAllRenderItems[key.RenderItemIndex].Instances[key.InstanceIndex].Transform;

		float3 moveDir = { 0.0f, 0.0f, 0.0f };

		if (input.IsKeyDown('A')) moveDir.x -= 1.0f;
		if (input.IsKeyDown('a')) moveDir.x -= 1.0f;
		if (input.IsKeyDown('D')) moveDir.x += 1.0f;
		if (input.IsKeyDown('d')) moveDir.x += 1.0f;
		if (input.IsKeyDown('Q')) moveDir.y -= 1.0f;
		if (input.IsKeyDown('q')) moveDir.y -= 1.0f;
		if (input.IsKeyDown('E')) moveDir.y += 1.0f;
		if (input.IsKeyDown('e')) moveDir.y += 1.0f;
		// if (input.IsKeyDown('W')) moveDir.z += 1.0f;
		// if (input.IsKeyDown('w')) moveDir.z += 1.0f;
		// if (input.IsKeyDown('S')) moveDir.z -= 1.0f;
		// if (input.IsKeyDown('s')) moveDir.z -= 1.0f;
		if (input.IsKeyDown('W')) moveDir.y += 1.0f;
		if (input.IsKeyDown('w')) moveDir.y += 1.0f;
		if (input.IsKeyDown('S')) moveDir.y -= 1.0f;
		if (input.IsKeyDown('s')) moveDir.y -= 1.0f;

		if(moveDir.x != 0.0f || moveDir.y != 0.0f || moveDir.z != 0.0f){
			transform.Dirty = true;
			transform.w_Position += moveDir * 0.01f;
			LOG_INFO("{}, {}, {}", transform.w_Position.x, transform.w_Position.y, transform.w_Position.z);
		}
			
		
	}

private:
    void SyncData(UploadBuffer<InstanceData>* instanceDataBuffer)
    {
		uint32_t visibleInstanceCount = 0;
		for (auto& ri : mAllRenderItems)
		{
			if (!(ri.Option & eCFGRenderItem::FrustumCullingEnabled) && ri.NumFramesDirty == 0)	// 현재 for문 내부에서 Frame Dirty를 바탕으로 Render Item 별로 업데이트를 관리하는데, 어떤 방식이 CPU 성능에 적합할지는 고민해보고 개선할 여지가 있다.
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
					instanceDataBuffer->CopyData(visibleInstanceCount++, instance.InstanceData);
				}
			}
			meshComponent->InstanceCount = visibleInstanceCount - meshComponent->StartInstanceLocation;
		}
    }

	void SyncInstanceIDData(UploadBuffer<InstanceIDData>* instanceIDBuffer)
	{
		if (mNumFramesDirty == 0)
			return;

		--mNumFramesDirty;
		InstanceIDData instanceID;
		instanceID.BaseInstanceIndex = 0;
		for (int i = 0; i < mAllRenderItems.size(); ++i)
		{
			auto& ri = mAllRenderItems[i];
			DX12_MeshSystem::GetInstance().GetMeshComponent(ri.MeshHandle)->StartInstanceLocation = instanceID.BaseInstanceIndex;
			instanceIDBuffer->CopyData(i, instanceID);	// 해당 구문의 이유는 Draw Call 간 Base InstanceIndex 전달에 버그가 존재하기 때문
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