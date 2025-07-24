#pragma once
#include "ECSConfig.h"

struct GravityComponent {
	float3 force = { 0.0f, -9.81f, 0.0f };
};
