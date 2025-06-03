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

using MeshHandle = std::string;
struct MeshComponent
{
	MeshHandle handle;
};

struct Vertex
{
	Vertex()
		: Position(0.0f, 0.0f, 0.0f)
		, Normal(0.0f, 0.0f, 0.0f)
		, TangentU(0.0f, 0.0f, 0.0f)
		, TexC(0.0f, 0.0f) {
	}
	Vertex(
		const DirectX::SimpleMath::Vector3& p,
		const DirectX::SimpleMath::Vector3& n,
		const DirectX::SimpleMath::Vector3& t,
		const DirectX::SimpleMath::Vector2& uv) :
		Position(p),
		Normal(n),
		TangentU(t),
		TexC(uv) {
	}
	Vertex(
		float px, float py, float pz,
		float nx, float ny, float nz,
		float tx, float ty, float tz,
		float u, float v) :
		Position(px, py, pz),
		Normal(nx, ny, nz),
		TangentU(tx, ty, tz),
		TexC(u, v) {
	}

	DirectX::SimpleMath::Vector3 Position;
	DirectX::SimpleMath::Vector3 Normal;
	DirectX::SimpleMath::Vector2 TexC;
	DirectX::SimpleMath::Vector3 TangentU;
};

struct SkinnedVertex
{
	DirectX::SimpleMath::Vector3 Position;
	DirectX::SimpleMath::Vector3 Normal;
	DirectX::SimpleMath::Vector2 TexC;
	DirectX::SimpleMath::Vector3 TangentU;
	DirectX::SimpleMath::Vector3 BoneWeights;
	uint8_t BoneIndices[4];
};