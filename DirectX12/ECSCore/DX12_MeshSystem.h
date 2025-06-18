#pragma once
#include "ECSCoordinator.h"
#include "DX12_MeshComponent.h"
#include "DX12_TransformComponent.h"

class DX12_MeshSystem : public ECS::ISystem {
public:
    void Update() override {
        auto& coordinator = ECS::Coordinator::GetInstance();
        for (ECS::Entity entity : mEntities) {
            auto& transform = coordinator.GetComponent<DX12_TransformComponent>(entity);
            if (!transform.Dirty) continue;
            auto& instance = coordinator.GetComponent<DX12_MeshComponent>(entity);

            DirectX::XMMATRIX S = DirectX::XMMatrixScaling(transform.r_Scale.x, transform.r_Scale.y, transform.r_Scale.z);
            DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
                DirectX::XMConvertToRadians(transform.r_RotationEuler.x),
                DirectX::XMConvertToRadians(transform.r_RotationEuler.y),
                DirectX::XMConvertToRadians(transform.r_RotationEuler.z));
            DirectX::XMMATRIX Tm = DirectX::XMMatrixTranslation(transform.r_Position.x, transform.r_Position.y, transform.r_Position.z);

            instance.World = S * R * Tm;
            DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(instance.World);
            instance.WorldInvTranspose = DirectX::XMMatrixInverse(&det, instance.World);
            // instance.TexTransform;
            LOG_INFO("Instance World[0]: {}, {}, {}, {}", instance.World._11, instance.World._12, instance.World._13, instance.World._14);
            LOG_INFO("Instance World[1]: {}, {}, {}, {}", instance.World._21, instance.World._22, instance.World._23, instance.World._24);
            LOG_INFO("Instance World[2]: {}, {}, {}, {}", instance.World._31, instance.World._32, instance.World._33, instance.World._34);
            LOG_INFO("Instance World[3]: {}, {}, {}, {}", instance.World._41, instance.World._42, instance.World._43, instance.World._44);
        }
    }
private:
    // std::unique_ptr<UploadBuffer<InstanceData>> InstanceBuffer = nullptr;
};