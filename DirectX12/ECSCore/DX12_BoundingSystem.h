#pragma once
#include "ECSCoordinator.h"
#include "DX12_BoundingComponent.h"
#include "DX12_TransformComponent.h"
#include "DX12_MeshComponent.h"

class DX12_BoundingSystem : public ECS::ISystem {
public:
	void Update() override {
		auto& coordinator = ECS::Coordinator::GetInstance();
		for (ECS::Entity entity : mEntities) {
			auto& transform = coordinator.GetComponent<DX12_TransformComponent>(entity);
			if (!transform.Dirty) continue;
			auto& bounding = coordinator.GetComponent<DX12_BoundingComponent>(entity);
			auto& mesh = coordinator.GetComponent<DX12_MeshComponent>(entity);

			DirectX::XMMATRIX S = DirectX::XMMatrixScaling(transform.r_Scale.x, transform.r_Scale.y, transform.r_Scale.z);
			DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
				DirectX::XMConvertToRadians(transform.r_RotationEuler.x),
				DirectX::XMConvertToRadians(transform.r_RotationEuler.y),
				DirectX::XMConvertToRadians(transform.r_RotationEuler.z));
			DirectX::XMMATRIX Tm = DirectX::XMMatrixTranslation(transform.r_Position.x, transform.r_Position.y, transform.r_Position.z);

			DirectX::XMMATRIX world = S * R * Tm;

			mesh.BoundingBox.Transform(bounding.Box, world);
			mesh.BoundingSphere.Transform(bounding.Sphere, world);
			LOG_VERBOSE("box {}, {}, {}", bounding.Box.Center.x, bounding.Box.Center.y, bounding.Box.Center.z);
		}
	}

private:
};