#pragma once
#include "ECSCoordinator.h"
#include "TransformComponent.h"
#include "RigidBodyComponent.h"
#include "GravityComponent.h"


class PhysicsSystem : public ECS::ISystem {
public:
	void Update() override {
		auto& coordinator = ECS::Coordinator::GetInstance();
		for (ECS::Entity entity : mEntities) {

		}
	}

	void FinalUpdate() override {
		auto& coordinator = ECS::Coordinator::GetInstance();
		for (ECS::Entity entity : mEntities) {
			auto& rb = coordinator.GetComponent<RigidBodyComponent>(entity);
			auto& tf = coordinator.GetComponent<TransformComponent>(entity);
			const auto& gravity = coordinator.GetComponent<GravityComponent>(entity);
			const auto& time = coordinator.GetSingletonComponent<TimeComponent>();

			rb.Acceleration = gravity.force;
			rb.Velocity.x += rb.Acceleration.x * time.deltaTime;
			rb.Velocity.y += rb.Acceleration.y * time.deltaTime;
			rb.Velocity.z += rb.Acceleration.z * time.deltaTime;

			tf.Position.x += rb.Velocity.x * time.deltaTime;
			tf.Position.y += rb.Velocity.y * time.deltaTime;
			tf.Position.z += rb.Velocity.z * time.deltaTime;
		}
	}

private:
};