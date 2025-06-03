#pragma once


#include "../EngineCore/D3DUtil.h"
#include "../EngineCore/MathHelper.h"

static constexpr int MAX_LIGHTS = 3;

struct LightComponent
{
	DirectX::SimpleMath::Vector3 Strength = { 0.5f, 0.5f, 0.5f };
	float FalloffStart = 1.0f;                          // point/spot light only
	DirectX::SimpleMath::Vector3 Direction = { 0.0f, -1.0f, 0.0f };// directional/spot light only
	float FalloffEnd = 10.0f;                           // point/spot light only
	DirectX::SimpleMath::Vector3 Position = { 0.0f, 0.0f, 0.0f };  // point/spot light only
	float SpotPower = 64.0f;                            // spot light only

	uint32_t type = 1;
	float radius = 1.0f;
	float haloRadius = 1.0f;
	float haloStrength = 1.0f;

	DirectX::SimpleMath::Matrix viewProj = MathHelper::Identity4x4();
	DirectX::SimpleMath::Matrix invProj = MathHelper::Identity4x4();
	DirectX::SimpleMath::Matrix shadowTransform = MathHelper::Identity4x4();
};

struct MaterialComponent
{
	DirectX::SimpleMath::Vector4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::SimpleMath::Vector3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 0.1f;

	// Used in texture mapping.
	DirectX::XMFLOAT4X4 MatTransform = MathHelper::Identity4x4();

	float Matalic = 1.0f;
	int DiffMapIndex = 0; // *Warn, Billboard에서 DiffMapIndex를 gTreeMapArray 배열에 적용하여 활용 중
	int NormMapIndex = 0;
	int AOMapIndex = 0;

	int MetalicMapIndex = 0;
	int RoughnessMapIndex = 0;
	int EmissiveMapIndex = 0;
	int useAlbedoMap = 0;

	int useNormalMap = 0;
	int invertNormalMap = 0;
	int useAOMap = 0;
	int useMetallicMap = 0;

	int useRoughnessMap = 0;
	int useEmissiveMap = 0;
	int useAlphaTest = 0;
	int dummy1 = 0;
};

struct InstanceComponent
{
	DirectX::SimpleMath::Matrix World = MathHelper::Identity4x4();
	DirectX::SimpleMath::Matrix TexTransform = MathHelper::Identity4x4();
	DirectX::SimpleMath::Matrix WorldInvTranspose = MathHelper::Identity4x4();
	DirectX::SimpleMath::Vector2 DisplacementMapTexelSize = { 1.0f, 1.0f };
	float GridSpatialStep = 1.0f;
	int useDisplacementMap = 0;

	int DisplacementIndex = 0;
	int MaterialIndex = 0;
	int dummy1 = 0;
	int dummy2 = 0;
};



struct SkinnedConstantComponent
{
	DirectX::SimpleMath::Matrix BoneTransforms[96];
};

struct ShaderToyConstantComponent
{
	DirectX::SimpleMath::Vector4 iMouse;

	float dx;
	float dy;
	float threshold;
	float strength;

	float iTime;
	DirectX::SimpleMath::Vector2 iResolution;
	float dummy;
};

struct SsaoConstantComponent
{
	DirectX::SimpleMath::Matrix Proj;
	DirectX::SimpleMath::Matrix InvProj;
	DirectX::SimpleMath::Matrix ProjTex;
	DirectX::SimpleMath::Vector4 OffsetVectors[14];

	// For SsaoBlur.hlsl
	DirectX::SimpleMath::Vector4 BlurWeights[3];

	DirectX::SimpleMath::Vector2 InvRenderTargetSize = { 0.0f, 0.0f };

	// Coordinates given in view space.
	float OcclusionRadius = 0.5f;
	float OcclusionFadeStart = 0.2f;
	float OcclusionFadeEnd = 2.0f;
	float SurfaceEpsilon = 0.05f;
};

struct PassConstantComponent
{
	DirectX::SimpleMath::Matrix View = MathHelper::Identity4x4();
	DirectX::SimpleMath::Matrix InvView = MathHelper::Identity4x4();
	DirectX::SimpleMath::Matrix Proj = MathHelper::Identity4x4();
	DirectX::SimpleMath::Matrix InvProj = MathHelper::Identity4x4();
	DirectX::SimpleMath::Matrix ViewProj = MathHelper::Identity4x4();
	DirectX::SimpleMath::Matrix InvViewProj = MathHelper::Identity4x4();

	DirectX::SimpleMath::Vector3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float cbPerObjectPad1 = 0.0f;

	DirectX::SimpleMath::Vector2 RenderTargetSize = { 0.0f, 0.0f };
	DirectX::SimpleMath::Vector2 InvRenderTargetSize = { 0.0f, 0.0f };

	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;

	DirectX::SimpleMath::Vector4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	DirectX::SimpleMath::Vector4 FogColor = { 0.7f, 0.7f, 0.7f, 1.0f };

	float gFogStart = 5.0f;
	float gFogRange = 150.0f;
	DirectX::SimpleMath::Vector2 cbPerObjectPad2 = { 0.0f, 0.0f };

	LightComponent Lights[MAX_LIGHTS];

	uint32_t gCubeMapIndex = 0;
};

struct InstanceConstantComponent
{
	uint32_t BaseInstanceIndex;
};