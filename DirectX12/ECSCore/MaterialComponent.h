#pragma once
#include "../EngineCore/SimpleMath.h"

struct MaterialPropertyComponent {
    DirectX::SimpleMath::Vector4 DiffuseAlbedo;
    DirectX::SimpleMath::Vector3 FresnelR0;
    float Roughness;

    float Metalic;
};

enum eMaterialFlags : uint32_t {
    USE_DIFFUSE_MAP = 1 << 0,
    USE_NORMAL_MAP = 1 << 1,
    USE_AO_MAP = 1 << 3,
    USE_METALLIC_MAP = 1 << 4,
    USE_ROUGHNESS_MAP = 1 << 5,
    USE_EMISSIVE_MAP = 1 << 6,
    ALPHA_TEST = 1 << 7,
    INVERT_NORMAL_MAP = 1 << 2,
};

struct MaterialIndexComponent {
    int DiffMapIndex;
    int NormMapIndex;
    int AOMapIndex;
    int MetalicMapIndex;

    int RoughnessMapIndex;
    int EmissiveMapIndex;
    uint32_t Flags; // bitmask로 useXxxMap & invertNormalMap & alphaTest 관리
};

struct MaterialUVTransformComponent {
    DirectX::SimpleMath::Matrix MatTransform;
};

struct MaterialUpdateComponent {
    float time = 0.0f;
    float speed = 1.0f;
    float minBrightness = 0.5f;
    float maxBrightness = 1.5f;
};

struct MaterialData
{
	MaterialPropertyComponent Property;
    MaterialIndexComponent Index;
	MaterialUVTransformComponent UVTransform;
};