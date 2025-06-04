#pragma once

struct TimeComponent {
    float deltaTime = 0.0f;
    float totalTime = 0.0f;
};

struct AnimationTimeComponent {
    float localTime = 0.0f;
    float speed = 1.0f;
};