#pragma once
#include "DX12_Config.h"

struct DX12_BoundingComponent {
	static const char* GetName() { return "DX12_BoundingComponent"; }
	DirectX::BoundingBox Box;
	DirectX::BoundingSphere Sphere;
	bool FrustumCullingEnabled;
	bool ShowBoundingBox;
	bool ShowBoundingSphere;
};

inline void to_json(json& j, const DX12_BoundingComponent& p) {
	j = json{
		{"BoundingBox",             {p.Box.Extents.x, p.Box.Extents.y, p.Box.Extents.z}},
		{"BoundingSphere",          {p.Sphere.Radius}},
		{"FrustumCullingEnable",    {p.FrustumCullingEnabled}},
		{"ShowBoundingBox",         {p.ShowBoundingBox}},
		{"ShowBoundingSphere",      {p.ShowBoundingSphere}}
	};
}

// JSON -> TransformComponent
inline void from_json(const json& j, DX12_BoundingComponent& p) {
	j.at("BoundingBox").get_to(p.Box.Extents.x);
	j.at("BoundingSphere").get_to(p.Sphere.Radius);
	j.at("FrustumCullingEnable").get_to(p.FrustumCullingEnabled);
	j.at("ShowBoundingBox").get_to(p.ShowBoundingBox);
	j.at("ShowBoundingSphere").get_to(p.ShowBoundingSphere);
}