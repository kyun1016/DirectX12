#pragma once

#include "../EngineCore/SimpleMath.h"

enum class LightType : uint32_t {
	Directional = 0,
	Point = 1,
	Spot = 2
};

struct LightComponent
{
	DirectX::SimpleMath::Vector3 Strength = { 0.5f, 0.5f, 0.5f };
	float FalloffStart = 1.0f;										// point/spot light only
	DirectX::SimpleMath::Vector3 Direction = { 0.0f, -1.0f, 0.0f };	// directional/spot light only
	float FalloffEnd = 10.0f;										// point/spot light only
	DirectX::SimpleMath::Vector3 Position = { 0.0f, 0.0f, 0.0f };	// point/spot light only
	float SpotPower = 64.0f;										// spot light only

	LightType type = LightType::Point;
	float radius = 1.0f;
	float haloRadius = 1.0f;
	float haloStrength = 1.0f;
};

struct LightShadowComponent {
	DirectX::SimpleMath::Matrix viewProj;
	DirectX::SimpleMath::Matrix invProj;
	DirectX::SimpleMath::Matrix shadowTransform;
};