#pragma once
#include "DX12_Config.h"

struct DX12_TransformComponent {
	float3 w_Position;
	float3 w_Scale;
	float3 w_RotationEuler;
	DirectX::SimpleMath::Quaternion w_RotationQuat;

	float3 r_Position;
	float3 r_Scale;
	float3 r_RotationEuler;
	DirectX::SimpleMath::Quaternion r_RotationQuat;

	bool Dirty = true;
};