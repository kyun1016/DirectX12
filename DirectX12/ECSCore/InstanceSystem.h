#pragma once
#include "ECSCoordinator.h"
#include "InstanceComponent.h"

class InstanceSystem : public ECS::ISystem {
public:
	void Update() override {
		auto& coordinator = ECS::Coordinator::GetInstance();
		for (ECS::Entity entity : mEntities) {
			auto& rb = coordinator.GetComponent<InstanceComponent>(entity);
		}
	}

	void FinalUpdate() override {
		auto& coordinator = ECS::Coordinator::GetInstance();
		for (ECS::Entity entity : mEntities) {

		}
	}

private:
};