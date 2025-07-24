#pragma once
#include "ECSCoordinator.h"
#include "InputSystem.h"
#include "TimeComponent.h"
#include "TransformComponent.h"
#include "RigidBodyComponent.h"
#include "PlayerControlComponent.h"
#include <WinUser.h> // For VK_... virtual key codes

class PlayerControlSystem : public ECS::ISystem {
public:
    void Update() override {
        auto& coordinator = ECS::Coordinator::GetInstance();
        auto& input = InputSystem::GetInstance();
        const auto& time = coordinator.GetSingletonComponent<TimeComponent>();

        for (auto const& entity : mEntities) {
            auto& transform = coordinator.GetComponent<TransformComponent>(entity);
            auto& rigidBody = coordinator.GetComponent<RigidBodyComponent>(entity);
            auto& control = coordinator.GetComponent<PlayerControlComponent>(entity);

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

			rigidBody.Velocity = moveDir * control.moveSpeed * time.deltaTime;
        }
    }
};