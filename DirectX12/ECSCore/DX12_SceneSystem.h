#pragma once
#include "ECSCoordinator.h"
#include "DX12_PSOSystem.h"
#include "DX12_TransformSystem.h"
#include "DX12_InstanceSystem.h"

class SceneSystem : public ECS::ISystem
{
public:
    void Update() override
    {
        // 이 함수에서 ECS 컴포넌트 -> RenderItem 데이터 동기화를 수행합니다.
        SyncData();
    }

    // DX12_RenderSystem이 렌더링할 아이템 목록을 가져갈 수 있도록 getter 제공
    const std::vector<RenderItem*>& GetRenderItems(eRenderLayer layer) const
    {
        return mRenderItemLayers[static_cast<int>(layer)];
    }

private:
    void SyncData()
    {
		if (BaseBoundingBox)
		{
			BoundingBox.Center.x = BaseBoundingBox->Center.x * Scale.x + Translation.x;
			BoundingBox.Center.y = BaseBoundingBox->Center.y * Scale.y + Translation.y;
			BoundingBox.Center.z = BaseBoundingBox->Center.z * Scale.z + Translation.z;

			BoundingBox.Extents.x = BaseBoundingBox->Extents.x * Scale.x;
			BoundingBox.Extents.y = BaseBoundingBox->Extents.y * Scale.y;
			BoundingBox.Extents.z = BaseBoundingBox->Extents.z * Scale.z;
		}
		if (BaseBoundingSphere)
		{
			BoundingSphere.Center.x = BaseBoundingSphere->Center.x * Scale.x + Translation.x;
			BoundingSphere.Center.y = BaseBoundingSphere->Center.y * Scale.y + Translation.y;
			BoundingSphere.Center.z = BaseBoundingSphere->Center.z * Scale.z + Translation.z;
			BoundingSphere.Radius = BaseBoundingSphere->Radius * Scale.Length();
		}

		float rx = DirectX::XMConvertToRadians(Rotate.x);
		float ry = DirectX::XMConvertToRadians(Rotate.y);
		float rz = DirectX::XMConvertToRadians(Rotate.z);
		RotationQuat = DirectX::XMQuaternionRotationRollPitchYaw(rx, ry, rz);

		DirectX::XMMATRIX rotX = DirectX::XMMatrixRotationX(rx);
		DirectX::XMMATRIX rotY = DirectX::XMMatrixRotationY(ry);
		DirectX::XMMATRIX rotZ = DirectX::XMMatrixRotationZ(rz);

		DirectX::XMMATRIX rot = rotX * rotY * rotZ;
		if (useQuat)
			rot = DirectX::XMMatrixRotationQuaternion(RotationQuat);

		DirectX::XMMATRIX world
			= DirectX::XMMatrixScaling(Scale.x, Scale.y, Scale.z)
			* rot
			* DirectX::XMMatrixTranslation(Translation.x, Translation.y, Translation.z);
		DirectX::XMMATRIX texTransform = DirectX::XMMatrixScaling(TexScale.x, TexScale.y, TexScale.z);
		DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(world);

		InstanceData.World = DirectX::XMMatrixTranspose(world);
		InstanceData.TexTransform = DirectX::XMMatrixTranspose(texTransform);
		InstanceData.WorldInvTranspose = DirectX::XMMatrixInverse(&det, world);
    }

    // RenderLayer별로 RenderItem 포인터 목록을 관리
    // 예: mRenderItemLayers[0]는 Opaque 레이어의 아이템 목록
    std::vector<RenderItem*> mRenderItemLayers[static_cast<int>(eRenderLayer::Count)];

    // 모든 RenderItem의 실제 소유권을 관리
    std::unordered_map<std::string, std::unique_ptr<RenderItem>> mAllRenderItems;
};