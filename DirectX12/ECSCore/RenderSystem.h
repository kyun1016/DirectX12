#pragma once
#include "ECSCoordinator.h"
#include "RenderComponent.h"

namespace ECS {
    class RenderSystem : public ECS::ISystem {
    public:
        void Update(float dt) override {
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