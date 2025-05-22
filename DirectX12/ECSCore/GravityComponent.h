#pragma once
#include "ECSConfig.h"

struct Gravity { DirectX::SimpleMath::Vector3 force = { 0.0f, -9.8f, 0.0f }; };
struct RigidBody { DirectX::SimpleMath::Vector3 velocity{}, acceleration{}; };
struct Transform { DirectX::SimpleMath::Vector3 position{}, rotation{}, scale{ 1.0f, 1.0f, 1.0f }; };