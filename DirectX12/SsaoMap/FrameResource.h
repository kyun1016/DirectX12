#pragma once

#include "../EngineCore/D3DUtil.h"
#include "../EngineCore/MathHelper.h"
#include "../EngineCore/UploadBuffer.h"

static constexpr int MAX_LIGHTS = 3;

struct LightData
{
	DirectX::XMFLOAT3 Strength = { 0.5f, 0.5f, 0.5f };
	float FalloffStart = 1.0f;                          // point/spot light only
	DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };// directional/spot light only
	float FalloffEnd = 10.0f;                           // point/spot light only
	DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };  // point/spot light only
	float SpotPower = 64.0f;                            // spot light only

	UINT type = 1;
	float radius = 1.0f;
	float haloRadius = 1.0f;
	float haloStrength = 1.0f;

	DirectX::SimpleMath::Matrix viewProj = MathHelper::Identity4x4();
	DirectX::SimpleMath::Matrix invProj = MathHelper::Identity4x4();
	DirectX::SimpleMath::Matrix shadowTransform = MathHelper::Identity4x4();
};

struct MaterialData
{
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
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
	int dummy1 = 0;
	int dummy2 = 0;
};

struct InstanceData
{
	DirectX::SimpleMath::Matrix World = MathHelper::Identity4x4();
	DirectX::SimpleMath::Matrix TexTransform = MathHelper::Identity4x4();
	DirectX::SimpleMath::Matrix WorldInvTranspose = MathHelper::Identity4x4();
	DirectX::XMFLOAT2 DisplacementMapTexelSize = { 1.0f, 1.0f };
	float GridSpatialStep = 1.0f;
	int useDisplacementMap = 0;

	UINT DisplacementIndex = 0;
	UINT MaterialIndex = 0;
	UINT dummy1 = 0;
	UINT dummy2 = 0;
};

struct EXMaterialData
{
	EXMaterialData()
	{
		MaterialData.DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
		MaterialData.FresnelR0 = { 0.01f, 0.01f, 0.01f };
		MaterialData.Roughness = 0.1f;
	}
	MaterialData MaterialData;

	// std::string Name;
	int NumFramesDirty = 3;
};

struct EXInstanceData
{
	EXInstanceData() = delete;
	EXInstanceData(DirectX::SimpleMath::Vector3 translation, DirectX::SimpleMath::Vector3 scale, DirectX::SimpleMath::Quaternion rot, DirectX::SimpleMath::Vector3 texScale, UINT boundingCount = 0, UINT matIdx = 0, bool cull = true)
		: Translation(translation)
		, Scale(scale)
		, RotationQuat(rot)
		, TexScale(texScale)
		, BoundingCount(boundingCount)
		, FrustumCullingEnabled(cull)
		, ShowBoundingBox(false)
		, ShowBoundingSphere(false)
		, IsPickable(true)
	{
		DirectX::XMMATRIX world = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z) * DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z);
		DirectX::XMMATRIX texTransform = DirectX::XMMatrixScaling(texScale.x, texScale.y, texScale.z);
		DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(world);

		InstanceData.World = DirectX::XMMatrixTranspose(world);
		InstanceData.TexTransform = DirectX::XMMatrixTranspose(texTransform);
		InstanceData.WorldInvTranspose = DirectX::XMMatrixInverse(&det, world);
		InstanceData.MaterialIndex = matIdx;
	}
	EXInstanceData(DirectX::BoundingBox boundingBox, DirectX::BoundingSphere boundingSphere, DirectX::SimpleMath::Vector3 translation, DirectX::SimpleMath::Vector3 scale, DirectX::SimpleMath::Quaternion rot, DirectX::SimpleMath::Vector3 texScale, UINT boundingCount = 0, UINT matIdx = 0, bool cull = true)
		: BoundingBox(boundingBox)
		, BoundingSphere(boundingSphere)
		, Translation(translation)
		, Scale(scale)
		, TexScale(texScale)
		, RotationQuat(rot)
		, BoundingCount(boundingCount)
		, FrustumCullingEnabled(cull)
		, ShowBoundingBox(false)
		, ShowBoundingSphere(false)
		, IsPickable(true)
	{
		BoundingBox.Center.x += translation.x;
		BoundingBox.Center.y += translation.y;
		BoundingBox.Center.z += translation.z;

		BoundingBox.Extents.x *= scale.x;
		BoundingBox.Extents.y *= scale.y;
		BoundingBox.Extents.z *= scale.z;

		BoundingSphere.Center.x += translation.x;
		BoundingSphere.Center.y += translation.y;
		BoundingSphere.Center.z += translation.z;
		BoundingSphere.Radius *= scale.Length();

		DirectX::XMMATRIX world
			= DirectX::XMMatrixScaling(scale.x, scale.y, scale.z)
			* DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z);
		DirectX::XMMATRIX texTransform = DirectX::XMMatrixScaling(texScale.x, texScale.y, texScale.z);
		DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(world);

		InstanceData.World = DirectX::XMMatrixTranspose(world);
		InstanceData.TexTransform = DirectX::XMMatrixTranspose(texTransform);
		InstanceData.WorldInvTranspose = DirectX::XMMatrixInverse(&det, world);
		InstanceData.MaterialIndex = matIdx;
	}

	InstanceData InstanceData; // GPU 전송 전용 데이터

	DirectX::BoundingBox BoundingBox;
	DirectX::BoundingSphere BoundingSphere;

	DirectX::SimpleMath::Vector3 Translation;
	DirectX::SimpleMath::Vector3 Scale;
	DirectX::SimpleMath::Quaternion RotationQuat;
	DirectX::SimpleMath::Vector3 TexScale;
	UINT BoundingCount;	// 추후 BoundingBox, BoundingSphere 표현을 위한 구조에서 연동하여 활용
	bool FrustumCullingEnabled;
	bool ShowBoundingBox;
	bool ShowBoundingSphere;
	bool IsPickable;
};

struct PassConstants
{
	DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
	DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float cbPerObjectPad1 = 0.0f;
	DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;

	DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	DirectX::XMFLOAT4 FogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
	float gFogStart = 5.0f;
	float gFogRange = 150.0f;
	DirectX::XMFLOAT2 cbPerObjectPad2 = { 0.0f, 0.0f };

	LightData Lights[MAX_LIGHTS];

	UINT gCubeMapIndex = 0;
};

struct InstanceConstants
{
	UINT BaseInstanceIndex;
};

struct Vertex
{
	Vertex()
		: Position(0.0f, 0.0f, 0.0f)
		, Normal(0.0f, 0.0f, 0.0f)
		, TangentU(0.0f, 0.0f, 0.0f)
		, TexC(0.0f, 0.0f) {}
	Vertex(
		const DirectX::XMFLOAT3& p,
		const DirectX::XMFLOAT3& n,
		const DirectX::XMFLOAT3& t,
		const DirectX::XMFLOAT2& uv) :
		Position(p),
		Normal(n),
		TangentU(t),
		TexC(uv) {}
	Vertex(
		float px, float py, float pz,
		float nx, float ny, float nz,
		float tx, float ty, float tz,
		float u, float v) :
		Position(px, py, pz),
		Normal(nx, ny, nz),
		TangentU(tx, ty, tz),
		TexC(u, v) {}

	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 TexC;
	DirectX::XMFLOAT3 TangentU;
};

struct FrameResource
{
public:
	FrameResource(ID3D12Device* device, UINT passCount, UINT baseInstanceCount, UINT maxInstanceCount, UINT materialCount)
	{
		ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

		InstanceBuffer = std::make_unique<UploadBuffer<InstanceData>>(device, maxInstanceCount, false);
		MaterialBuffer = std::make_unique<UploadBuffer<MaterialData>>(device, materialCount, false);
		PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
		InstanceCB = std::make_unique<UploadBuffer<InstanceConstants>>(device, baseInstanceCount, true);
	}
	FrameResource(const FrameResource& rhs) = delete;
	FrameResource& operator=(const FrameResource& rhs) = delete;
	~FrameResource() {}

public:
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

	std::unique_ptr<UploadBuffer<InstanceData>> InstanceBuffer = nullptr;
	std::unique_ptr<UploadBuffer<MaterialData>> MaterialBuffer = nullptr;

	std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
	std::unique_ptr<UploadBuffer<InstanceConstants>> InstanceCB = nullptr;

	UINT64 Fence = 0;
};

