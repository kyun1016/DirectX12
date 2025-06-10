#pragma once
#include "ECSCoordinator.h"
#include "TimeComponent.h"
#include <chrono>

class Timer {
public:
    void Start() {
        mLast = std::chrono::high_resolution_clock::now();
    }

    float Tick() {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> delta = now - mLast;
        mLast = now;
        return delta.count();
    }

private:
    std::chrono::high_resolution_clock::time_point mLast;
};

class TimeSystem : public ECS::ISystem {
public:
    TimeSystem()
    {
        auto& coordinator = ECS::Coordinator::GetInstance();
        coordinator.RegisterSingletonComponent<TimeComponent>();
        coordinator.GetSingletonComponent<TimeComponent>().deltaTime = 0.0f;
        coordinator.GetSingletonComponent<TimeComponent>().totalTime = 0.0f;
        mTimer.Start();
        mTimer.Tick(); // Initialize the timer
    }

    void Update() override {

    }

    void LateUpdate() override {

    }

    void Sync() override {
        auto& coordinator = ECS::Coordinator::GetInstance();
        const float dt = mTimer.Tick();

        auto& time = coordinator.GetSingletonComponent<TimeComponent>();
        time.deltaTime = dt;
        time.totalTime += dt;

        LOG_VERBOSE("time at step: ({}, {})", time.deltaTime, time.totalTime);
    }

private:
    Timer mTimer;
};

class AnimationTimeSystem : public ECS::ISystem {
public:
    void Update() override {
        auto& coordinator = ECS::Coordinator::GetInstance();
        const auto& time = coordinator.GetSingletonComponent<TimeComponent>();

        for (auto entity : mEntities) {
            auto& anim = coordinator.GetComponent<AnimationTimeComponent>(entity);
            anim.localTime += time.deltaTime * anim.speed;
        }
    }
};