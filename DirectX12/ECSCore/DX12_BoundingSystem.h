#pragma once
#include "ECSCoordinator.h"
#include "DX12_BoundingComponent.h"
#include "TransformComponent.h"
#include "DX12_MeshComponent.h"

class DX12_BoundingSystem : public ECS::ISystem {
public:
	void Update() override {
		auto& coordinator = ECS::Coordinator::GetInstance();
		for (ECS::Entity entity : mEntities) {
			auto& transform = coordinator.GetComponent<TransformComponent>(entity);
			if (!transform.Dirty) continue;
			auto& bounding = coordinator.GetComponent<DX12_BoundingComponent>(entity);
			auto& mesh = coordinator.GetComponent<DX12_MeshComponent>(entity);

			DirectX::XMMATRIX S = DirectX::XMMatrixScaling(transform.Scale.x, transform.Scale.y, transform.Scale.z);
			DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
				DirectX::XMConvertToRadians(transform.Rotation.x),
				DirectX::XMConvertToRadians(transform.Rotation.y),
				DirectX::XMConvertToRadians(transform.Rotation.z));
			DirectX::XMMATRIX Tm = DirectX::XMMatrixTranslation(transform.Position.x, transform.Position.y, transform.Position.z);

			DirectX::XMMATRIX world = S * R * Tm;

			mesh.BoundingBox.Transform(bounding.Box, world);
			mesh.BoundingSphere.Transform(bounding.Sphere, world);
			LOG_VERBOSE("box {}, {}, {}", bounding.Box.Center.x, bounding.Box.Center.y, bounding.Box.Center.z);
		}
	}

private:
};