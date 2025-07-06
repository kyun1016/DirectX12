#pragma once
#include "ECSCoordinator.h"
#include "CameraComponent.h"
#include "InputSystem.h"

class CameraSystem : public ECS::ISystem {
public:
	void Sync() override {
		auto& coordinator = ECS::Coordinator::GetInstance();
		for (ECS::Entity entity : mEntities) {
			auto& cameraComp = coordinator.GetComponent<CameraComponent>(entity);
			cameraComp.SyncData();
		}
	}
	void Update() override {
		auto& input = ECS::InputSystem::GetInstance();
		auto& coordinator = ECS::Coordinator::GetInstance();
		for (ECS::Entity entity : mEntities) {
			auto& cameraComp = coordinator.GetComponent<CameraComponent>(entity);
			if (input.IsKeyDown('W') || input.IsKeyDown('w')) {
				// cameraComp.component.MovementDelta.z += cameraComp.component.MovementSpeed * input.GetDeltaTime();
			}
		}
	}
private:
};