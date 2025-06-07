#pragma once
#include "ECSCoordinator.h"
#include "MeshComponent.h"
#include "MeshRepository.h"

class MeshSystem : public ECS::ISystem {
public:
    void Update() override {
        auto& coordinator = ECS::Coordinator::GetInstance();
        for (ECS::Entity entity : mEntities) {
            auto& component = coordinator.GetComponent<MeshComponent>(entity);
			auto mesh = MeshRepository::GetInstance().Get(component.handle);
            LOG_INFO("Mesh System {}", mesh->id);
        }
    }
};