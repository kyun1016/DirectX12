#pragma once
#include "ECSConfig.h"

struct RigidBodyComponent {
	float Mass;
	float3 DiffPosition;
	float3 Velocity;
	float3 AngularVelocity;
	float3 Force;
	float3 Torque;
	float3 Acceleration;
	float3 AngularAcceleration;
	bool UseGravity = true;
	bool IsKinematic = false;
};