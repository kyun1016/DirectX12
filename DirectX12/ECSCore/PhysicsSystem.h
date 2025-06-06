#pragma once
#include "ECSCoordinator.h"
#include "PhysicsComponent.h"
#include "TimeComponent.h"

class PhysicsSystem : public ECS::ISystem {
public:
    void Update() override {
        auto& coordinator = ECS::Coordinator::GetInstance();
        for (ECS::Entity entity : mEntities) {
            auto& rb = coordinator.GetComponent<RigidBodyComponent>(entity);
            auto& tf = coordinator.GetComponent<TransformComponent>(entity);
            const auto& gravity = coordinator.GetComponent<GravityComponent>(entity);
            const auto& time = coordinator.GetSingletonComponent<TimeComponent>();

            rb.acceleration = gravity.force;
            rb.velocity.x += rb.acceleration.x * time.deltaTime;
            rb.velocity.y += rb.acceleration.y * time.deltaTime;
            rb.velocity.z += rb.acceleration.z * time.deltaTime;

            tf.position.x += rb.velocity.x * time.deltaTime;
            tf.position.y += rb.velocity.y * time.deltaTime;
            tf.position.z += rb.velocity.z * time.deltaTime;

            //std::cout << "velocity at step: ("
            //    << rb.velocity.x << ", "
            //    << rb.velocity.y << ", "
            //    << rb.velocity.z << ")\n";

            //std::cout << "Position at step: ("
            //    << tf.position.x << ", "
            //    << tf.position.y << ", "
            //    << tf.position.z << ")\n";
        }
    }
};