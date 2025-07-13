#pragma once
#include "ECSCoordinator.h"
#include "DX12_TransformComponent.h"


class DX12_TransformSystem : public ECS::ISystem {
public:
	void Update() override {
		auto& coordinator = ECS::Coordinator::GetInstance();
		for (ECS::Entity entity : mEntities) {
			auto& transform = coordinator.GetComponent<DX12_TransformComponent>(entity);
			transform.w_Position += {0.0f, 0.0f, 0.1f};
			transform.w_Scale += {0.0f, 0.0f, 0.1f};
			transform.w_RotationEuler += {0.0f, 0.0f, 0.1f};
			LOG_VERBOSE("transform {}, {}, {}", transform.w_Position.x, transform.w_Position.y, transform.w_Position.z);
		}
	}

	void FinalUpdate() override {
		auto& coordinator = ECS::Coordinator::GetInstance();
		for (ECS::Entity entity : mEntities) {
			auto& transform = coordinator.GetComponent<DX12_TransformComponent>(entity);
			if (transform.w_Position != transform.r_Position ||
				transform.w_Scale != transform.r_Scale ||
				transform.w_RotationEuler != transform.r_RotationEuler)
			{
				transform.Dirty = true;
				transform.r_Position = transform.w_Position;
				transform.r_Scale = transform.w_Scale;
				transform.r_RotationEuler = transform.w_RotationEuler;
			}
			else
			{
				transform.Dirty = false;
			}
		}
	}

private:
};