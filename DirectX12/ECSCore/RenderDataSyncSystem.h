#pragma once
#include "ECSCoordinator.h"
#include "TransformComponent.h"
#include "RigidBodyComponent.h"
#include "GravityComponent.h"

class RenderDataSyncSystem : public ECS::ISystem {
public:
	void Update() override {
		auto& coordinator = ECS::Coordinator::GetInstance();
		const auto& time = coordinator.GetSingletonComponent<TimeComponent>();
		for (ECS::Entity entity : mEntities) {
			auto& rigidBody = coordinator.GetComponent<RigidBodyComponent>(entity);
			if (rigidBody.Acceleration.x == 0.0f && rigidBody.Acceleration.y == 0.0f && rigidBody.Acceleration.z &&
				rigidBody.AngularAcceleration.x == 0.0f && rigidBody.AngularAcceleration.y == 0.0f && rigidBody.AngularAcceleration.z)
				continue;

			rigidBody.Velocity.x += rigidBody.Acceleration.x * time.deltaTime;
			rigidBody.Velocity.y += rigidBody.Acceleration.y * time.deltaTime;
			rigidBody.Velocity.z += rigidBody.Acceleration.z * time.deltaTime;

			rigidBody.AngularVelocity.x += rigidBody.AngularAcceleration.x * time.deltaTime;
			rigidBody.AngularVelocity.y += rigidBody.AngularAcceleration.y * time.deltaTime;
			rigidBody.AngularVelocity.z += rigidBody.AngularAcceleration.z * time.deltaTime;

			if (!rigidBody.UseGravity)
				continue;
			const auto& gravity = coordinator.GetComponent<GravityComponent>(entity);
			rigidBody.Velocity += gravity.Force * time.deltaTime;
		}
	}

	void FinalUpdate() override {
		auto& coordinator = ECS::Coordinator::GetInstance();
		const auto& time = coordinator.GetSingletonComponent<TimeComponent>();
		for (ECS::Entity entity : mEntities) {
			auto& rigidBody = coordinator.GetComponent<RigidBodyComponent>(entity);
			if (rigidBody.Velocity.x == 0.0f && rigidBody.Velocity.y == 0.0f && rigidBody.Velocity.z == 0.0f &&
				rigidBody.AngularVelocity.x == 0.0f && rigidBody.AngularVelocity.y == 0.0f && rigidBody.AngularVelocity.z == 0.0f)
				continue;

			auto& transform = coordinator.GetComponent<TransformComponent>(entity);

			transform.Position.x += rigidBody.Velocity.x * time.deltaTime;
			transform.Position.y += rigidBody.Velocity.y * time.deltaTime;
			transform.Position.z += rigidBody.Velocity.z * time.deltaTime;

			transform.Rotation.x += rigidBody.AngularVelocity.x * time.deltaTime;
			transform.Rotation.y += rigidBody.AngularVelocity.y * time.deltaTime;
			transform.Rotation.z += rigidBody.AngularVelocity.z * time.deltaTime;

			transform.Dirty = true;
		}
	}
private:
};