#pragma once
#include "ECSConfig.h"

struct TransformComponent {
	float3 Position;
	float3 Scale;
	float3 Rotation;
	float4 RotationQuat;

	bool Dirty = true;
};