#pragma once
#include "ECSCoordinator.h"
#include "MeshComponent.h"
#include "MeshRepository.h"

class MeshSystem : public ECS::ISystem {
public:
    void Update(float dt) override {
        auto& coordinator = ECS::Coordinator::GetInstance();
        for (ECS::Entity entity : mEntities) {
            auto& component = coordinator.GetComponent<MeshComponent>(entity);
            MeshRepository::GetMesh(component.handle);
        }
    }
};