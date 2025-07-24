#pragma once
#include "ECSCoordinator.h"
#include "InstanceComponent.h"

class BoundingVolumeUpdateSystem : public ECS::ISystem {
public:
	void Update() override {
		auto& coordinator = ECS::Coordinator::GetInstance();
		for (ECS::Entity entity : mEntities) {
			auto& transform = coordinator.GetComponent<TransformComponent>(entity);
			if (!transform.Dirty)
				continue;

			auto& boundingVolumn = coordinator.GetComponent<BoundingVolumnComponent>(entity);
			auto& mesh = coordinator.GetComponent<DX12_MeshComponent>(entity);

			boundingVolumn.BoundingBox = mesh.BoundingBox;
			boundingVolumn.BoundingBox.Center.x *= transform.Scale.x;
			boundingVolumn.BoundingBox.Center.y *= transform.Scale.y;
			boundingVolumn.BoundingBox.Center.z *= transform.Scale.z;
			boundingVolumn.BoundingBox.Center.x += transform.Position.x;
			boundingVolumn.BoundingBox.Center.y += transform.Position.y;
			boundingVolumn.BoundingBox.Center.z += transform.Position.z;
			boundingVolumn.BoundingBox.Extents.x *= transform.Scale.x;
			boundingVolumn.BoundingBox.Extents.y *= transform.Scale.y;
			boundingVolumn.BoundingBox.Extents.z *= transform.Scale.z;

			boundingVolumn.BoundingSphere = mesh.BoundingSphere;
			boundingVolumn.BoundingSphere.Center.x *= transform.Scale.x;
			boundingVolumn.BoundingSphere.Center.y *= transform.Scale.y;
			boundingVolumn.BoundingSphere.Center.z *= transform.Scale.z;
			boundingVolumn.BoundingSphere.Center.x += transform.Position.x;
			boundingVolumn.BoundingSphere.Center.y += transform.Position.y;
			boundingVolumn.BoundingSphere.Center.z += transform.Position.z;
			boundingVolumn.BoundingSphere.Radius *= transform.Scale.Length();
		}
	}

private:
};