#pragma once
#include "ECSConfig.h"

struct GravityComponent {
	static const char* GetName() { return "GravityComponent"; }
	float3 Force = { 0.0f, -9.81f, 0.0f };
};

inline void to_json(json& j, const GravityComponent& p) {
	j = json{
		{"Force", {p.Force.x, p.Force.y, p.Force.z}}
	};
}
inline void from_json(const json& j, GravityComponent& p) {
	j.at("Force").get_to(p.Force.x);
}