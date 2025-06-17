#pragma once
#include "DX12_Config.h"

struct DX12_BoundingComponent {
	DirectX::BoundingBox Box;
	DirectX::BoundingSphere Sphere;
	bool FrustumCullingEnabled;
	bool ShowBoundingBox;
	bool ShowBoundingSphere;
};