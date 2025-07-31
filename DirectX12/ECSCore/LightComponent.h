#pragma once

#include "../EngineCore/SimpleMath.h"

enum class eLightType : uint32_t {
	Directional = 0,
	Point = 1,
	Spot = 2
};

struct LightComponent
{
	static const char* GetName() { return "LightComponent"; }
	DirectX::SimpleMath::Vector3 Strength = { 0.5f, 0.5f, 0.5f };
	float FalloffStart = 1.0f;										// point/spot light only
	DirectX::SimpleMath::Vector3 Direction = { 0.0f, -1.0f, 0.0f };	// directional/spot light only
	float FalloffEnd = 10.0f;										// point/spot light only
	DirectX::SimpleMath::Vector3 Position = { 0.0f, 0.0f, 0.0f };	// point/spot light only
	float SpotPower = 64.0f;										// spot light only

	eLightType type = eLightType::Point;
	float radius = 1.0f;
	float haloRadius = 1.0f;
	float haloStrength = 1.0f;
};

struct LightShadowComponent {
	static const char* GetName() { return "LightShadowComponent"; }
	DirectX::SimpleMath::Matrix viewProj;
	DirectX::SimpleMath::Matrix invProj;
	DirectX::SimpleMath::Matrix shadowTransform;
};

inline void to_json(json& j, const LightComponent& p) {
	j = json{
		{ "Strength", { p.Strength.x, p.Strength.y, p.Strength.z } },
		{ "FalloffStart", p.FalloffStart },
		{ "Direction", { p.Direction.x, p.Direction.y, p.Direction.z } },
		{ "FalloffEnd", p.FalloffEnd },
		{ "Position", { p.Position.x, p.Position.y, p.Position.z } },
		{ "SpotPower", p.SpotPower },
		{ "type", static_cast<uint32_t>(p.type) },
		{ "radius", p.radius },
		{ "haloRadius", p.haloRadius },
		{ "haloStrength", p.haloStrength }
	};
}

inline void from_json(const json& j, LightComponent& p) {
	j.at("Strength").get_to(p.Strength.x);
	j.at("FalloffStart").get_to(p.FalloffStart);
	j.at("Direction").get_to(p.Direction.x);
	j.at("FalloffEnd").get_to(p.FalloffEnd);
	j.at("Position").get_to(p.Position.x);
	j.at("SpotPower").get_to(p.SpotPower);
	p.type = static_cast<eLightType>(j.at("type").get<uint32_t>());
	p.radius = j.at("radius").get<float>();
	p.haloRadius = j.at("haloRadius").get<float>();
	p.haloStrength = j.at("haloStrength").get<float>();
}