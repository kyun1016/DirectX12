#pragma once
#include "ECSCoordinator.h"
#include "GravityComponent.h"

class PhysicsSystem : public ECS::ISystem {
public:
    void Update(float dt) override {
        auto& coordinator = ECS::Coordinator::GetInstance();
        for (ECS::Entity entity : mEntities) {
            auto& rb = coordinator.GetComponent<RigidBody>(entity);
            auto& tf = coordinator.GetComponent<Transform>(entity);
            const auto& gravity = coordinator.GetComponent<Gravity>(entity);

            rb.acceleration = gravity.force;
            rb.velocity.x += rb.acceleration.x * dt;
            rb.velocity.y += rb.acceleration.y * dt;
            rb.velocity.z += rb.acceleration.z * dt;

            tf.position.x += rb.velocity.x * dt;
            tf.position.y += rb.velocity.y * dt;
            tf.position.z += rb.velocity.z * dt;
        }
    }
};