#pragma once
#include "ECSCoordinator.h"
#include "LightComponent.h"

class LightSystem : public ECS::ISystem {
public:
    void Update() override {
        auto& coordinator = ECS::Coordinator::GetInstance();
        for (ECS::Entity entity : mEntities) {
            auto& light = coordinator.GetComponent<LightComponent>(entity);

            // 1. Light의 위치나 방향 변화 반영
            // 2. ShadowComponent 생성 or 제거
            // 3. GPU 버퍼로 빌드
        }
    }
};