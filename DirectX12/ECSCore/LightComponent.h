//#pragma once
//
//#include "../EngineCore/SimpleMath.h"
//struct LightComponent
//{
//	DirectX::SimpleMath::Vector3 Strength = { 0.5f, 0.5f, 0.5f };
//	float FalloffStart = 1.0f;                          // point/spot light only
//	DirectX::SimpleMath::Vector3 Direction = { 0.0f, -1.0f, 0.0f };// directional/spot light only
//	float FalloffEnd = 10.0f;                           // point/spot light only
//	DirectX::SimpleMath::Vector3 Position = { 0.0f, 0.0f, 0.0f };  // point/spot light only
//	float SpotPower = 64.0f;                            // spot light only
//
//	uint32_t type = 1;
//	float radius = 1.0f;
//	float haloRadius = 1.0f;
//	float haloStrength = 1.0f;
//
//	DirectX::SimpleMath::Matrix viewProj = MathHelper::Identity4x4();
//	DirectX::SimpleMath::Matrix invProj = MathHelper::Identity4x4();
//	DirectX::SimpleMath::Matrix shadowTransform = MathHelper::Identity4x4();
//};