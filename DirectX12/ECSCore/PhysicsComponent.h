#pragma once
#include "ECSConfig.h"

struct GravityComponent { DirectX::SimpleMath::Vector3 force = { 0.0f, -9.8f, 0.0f }; };
struct RigidBodyComponent { DirectX::SimpleMath::Vector3 velocity{}, acceleration{}; };
struct TransformComponent { DirectX::SimpleMath::Vector3 position{}, rotation{}, scale{ 1.0f, 1.0f, 1.0f }; };