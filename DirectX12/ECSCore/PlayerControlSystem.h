#pragma once
#include "ECSCoordinator.h"
#include "InputSystem.h"
#include "TimeComponent.h"
#include "TransformComponent.h"
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
            auto& control = coordinator.GetComponent<PlayerControlComponent>(entity);

            float3 moveDir = { 0.0f, 0.0f, 0.0f };

            if (input.IsKeyDown('W')) {
                moveDir.z += 1.0f;
            }
            if (input.IsKeyDown('S')) {
                moveDir.z -= 1.0f;
            }
            if (input.IsKeyDown('A')) {
                moveDir.x -= 1.0f;
            }
            if (input.IsKeyDown('D')) {
                moveDir.x += 1.0f;
            }

            transform.w_Position += moveDir * control.moveSpeed * time.deltaTime;
        }
    }
};