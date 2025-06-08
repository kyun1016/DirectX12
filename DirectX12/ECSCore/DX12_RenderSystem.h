#pragma once
#include "DX12_Config.h"
#include "RenderComponent.h"
#include "ECSCoordinator.h"

namespace ECS {
    class DX12_RenderSystem : public ECS::ISystem {
    public:
        DX12_RenderSystem()
        {

        }
        void Update() override {
            auto& coordinator = ECS::Coordinator::GetInstance();
            for (ECS::Entity entity : mEntities) {
                //auto& rb = coordinator.GetComponent<RigidBodyComponent>(entity);
                //auto& tf = coordinator.GetComponent<TransformComponent>(entity);
                //const auto& gravity = coordinator.GetComponent<GravityComponent>(entity);

                //rb.acceleration = gravity.force;
                //rb.velocity.x += rb.acceleration.x * dt;
                //rb.velocity.y += rb.acceleration.y * dt;
                //rb.velocity.z += rb.acceleration.z * dt;

                //tf.position.x += rb.velocity.x * dt;
                //tf.position.y += rb.velocity.y * dt;
                //tf.position.z += rb.velocity.z * dt;
            }
        }
    };
}