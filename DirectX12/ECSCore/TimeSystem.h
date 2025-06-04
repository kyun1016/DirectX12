#pragma once
#include "ECSCoordinator.h"
#include "TimeComponent.h"

class TimeSystem : public ECS::ISystem {
public:
    void Update(float dt) override {
        // auto& time = ECS::Coordinator::GetInstance().GetSingletonComponent<TimeComponent>();
        // time.deltaTime = dt;
        // time.totalTime += dt;
    }
};