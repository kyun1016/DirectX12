#pragma once
#include "ECSConfig.h"

struct TransformComponent {
	float3 Position;
	float3 Scale;
	float3 Rotation;
	float4 RotationQuat;

	bool Dirty = true;
};

inline void to_json(json& j, const TransformComponent& p) {
    j = json{
        {"Position", {p.Position.x, p.Position.y, p.Position.z}},
        {"Rotation", {p.RotationQuat.x, p.RotationQuat.y, p.RotationQuat.z, p.RotationQuat.w}},
        {"Scale",    {p.Scale.x, p.Scale.y, p.Scale.z}}
    };
}

// JSON -> TransformComponent
inline void from_json(const json& j, TransformComponent& p) {
    j.at("Position").get_to(p.Position.x); // float3에 대한 변환도 필요할 수 있음
    j.at("Rotation").get_to(p.RotationQuat.x);
    j.at("Scale").get_to(p.Scale.x);
}