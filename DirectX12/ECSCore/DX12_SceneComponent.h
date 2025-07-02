#pragma once
#include "DX12_Config.h"
#include "DX12_InstanceData.h"
#include "DX12_MeshComponent.h"
#include "DX12_PSOSystem.h"

// #ifndef SKINNEDDATA_H
// #define SKINNEDDATA_H
// struct Keyframe
// {
// 	Keyframe();
// 	~Keyframe();

// 	float TimePos;
// 	DirectX::XMFLOAT3 Translation;
// 	DirectX::XMFLOAT3 Scale;
// 	DirectX::XMFLOAT4 RotationQuat;
// };

// struct BoneAnimation
// {
// 	float GetStartTime()const;
// 	float GetEndTime()const;

// 	void Interpolate(float t, DirectX::XMFLOAT4X4& M)const;

// 	std::vector<Keyframe> Keyframes;
// };

// struct AnimationClip
// {
// 	float GetClipStartTime()const;
// 	float GetClipEndTime()const;

// 	void Interpolate(float t, std::vector<DirectX::XMFLOAT4X4>& boneTransforms)const;

// 	std::vector<BoneAnimation> BoneAnimations;
// };

// class SkinnedData
// {
// public:

// 	UINT BoneCount()const;

// 	float GetClipStartTime(const std::string& clipName)const;
// 	float GetClipEndTime(const std::string& clipName)const;

// 	void Set(
// 		std::vector<int>& boneHierarchy,
// 		std::vector<DirectX::XMFLOAT4X4>& boneOffsets,
// 		std::unordered_map<std::string, AnimationClip>& animations);

// 	void GetFinalTransforms(const std::string& clipName, float timePos,
// 		std::vector<DirectX::XMFLOAT4X4>& finalTransforms)const;

// private:
// 	std::vector<int> mBoneHierarchy;
// 	std::vector<DirectX::XMFLOAT4X4> mBoneOffsets;
// 	std::unordered_map<std::string, AnimationClip> mAnimations;
// };

// #endif // SKINNEDDATA_H


// struct SkinnedModelInstance
// {
// 	SkinnedData SkinnedInfo;
// 	std::vector<DirectX::XMFLOAT4X4> FinalTransforms;
// 	std::string ClipName;
// 	float TimePos = 0.0f;

// 	// Called every frame and increments the time position, interpolates the 
// 	// animations for each bone based on the current animation clip, and 
// 	// generates the final transforms which are ultimately set to the effect
// 	// for processing in the vertex shader.
// 	void UpdateSkinnedAnimation(float dt)
// 	{
// 		TimePos += dt;

// 		// Loop animation
// 		if (TimePos > SkinnedInfo.GetClipEndTime(ClipName))
// 			TimePos = 0.0f;

// 		// Compute the final transforms for this time position.
// 		SkinnedInfo.GetFinalTransforms(ClipName, TimePos, FinalTransforms);
// 	}
// };



// enum class eRenderLayer : std::uint32_t
// {
// 	None = 0,
// 	Opaque = 1 << 0,
// 	Sprite = 1 << 1,
// 	SkinnedOpaque = 1 << 2,
// 	Mirror = 1 << 3,
// 	Reflected = 1 << 4,
// 	AlphaTested = 1 << 5,
// 	Transparent = 1 << 6,
// 	Subdivision = 1 << 7,
// 	Normal = 1 << 8,
// 	SkinnedNormal = 1 << 9,
// 	TreeSprites = 1 << 10,
// 	Tessellation = 1 << 11,
// 	BoundingBox = 1 << 12,
// 	BoundingSphere = 1 << 13,
// 	CubeMap = 1 << 14,
// 	DebugShadowMap = 1 << 15,
// 	OpaqueWireframe = 1 << 16,
// 	MirrorWireframe = 1 << 17,
// 	ReflectedWireframe = 1 << 18,
// 	AlphaTestedWireframe = 1 << 19,
// 	TransparentWireframe = 1 << 20,
// 	SubdivisionWireframe = 1 << 21,
// 	NormalWireframe = 1 << 22,
// 	TreeSpritesWireframe = 1 << 23,
// 	TessellationWireframe = 1 << 24,
// 	ShadowMap = 1 << 25,
// 	SkinnedShadowMap = 1 << 26,
// 	AddCS = 1 << 27,
// 	BlurCS = 1 << 28,
// 	WaveCS = 1 << 29,
// 	ShaderToy = 1 << 30
// };
// ENUM_OPERATORS_32(eRenderLayer)

enum class eConfigInstanceComponent : std::uint32_t
{
	None = 0,
	UseCulling = 1 << 0,
	ShowBoundingBox = 1 << 1,
	ShowBoundingSphere = 1 << 2,
	Pickable = 1 << 2,
	UseQuat = 1 << 3
};
ENUM_OPERATORS_32(eConfigInstanceComponent)

struct InstanceComponent
{
	InstanceComponent() = delete;
	InstanceComponent(float3 translation, float3 scale, DirectX::SimpleMath::Quaternion rot, float3 texScale, UINT boundingCount = 0, UINT matIdx = 0)
		: Translation(translation)
		, Scale(scale)
		, RotationQuat(rot)
		, TexScale(texScale)
		, BoundingCount(boundingCount)
	{
		Update();
	}
	InstanceComponent(DirectX::BoundingBox* baseBoundingBox, DirectX::BoundingSphere* baseBoundingSphere, float3 translation, float3 scale, DirectX::SimpleMath::Quaternion rot, float3 texScale, UINT boundingCount = 0, UINT matIdx = 0)
		: BaseBoundingBox(baseBoundingBox)
		, BaseBoundingSphere(baseBoundingSphere)
		, Translation(translation)
		, Scale(scale)
		, TexScale(texScale)
		, RotationQuat(rot)
		, BoundingCount(boundingCount)
	{
		InstanceData.MaterialIndex = matIdx;
		Update();
	}
	void UpdateTranslation(float3 translation)
	{
		Translation = translation;
		Update();
	}
	void UpdateScale(float3 scale)
	{
		Scale = scale;
		Update();
	}
	void UpdateRotate(float3 rotate)
	{
		Rotate = rotate;
		Update();
	}
	void UpdateTexScale(float3 scale)
	{
		TexScale = scale;
		Update();
	}

	InstanceData InstanceData; // GPU 전송 전용 데이터

	DirectX::BoundingBox* BaseBoundingBox;
	DirectX::BoundingSphere* BaseBoundingSphere;
	DirectX::BoundingBox BoundingBox;
	DirectX::BoundingSphere BoundingSphere;

	DX12_TransformComponent Transform;
	float3 TexScale;
	eConfigInstanceComponent Config = eConfigInstanceComponent::UseCulling;

private:
	void Update()
	{
		if (BaseBoundingBox)
		{
			BoundingBox.Center.x = BaseBoundingBox->Center.x * Scale.x + Translation.x;
			BoundingBox.Center.y = BaseBoundingBox->Center.y * Scale.y + Translation.y;
			BoundingBox.Center.z = BaseBoundingBox->Center.z * Scale.z + Translation.z;

			BoundingBox.Extents.x = BaseBoundingBox->Extents.x * Scale.x;
			BoundingBox.Extents.y = BaseBoundingBox->Extents.y * Scale.y;
			BoundingBox.Extents.z = BaseBoundingBox->Extents.z * Scale.z;
		}
		if (BaseBoundingSphere)
		{
			BoundingSphere.Center.x = BaseBoundingSphere->Center.x * Scale.x + Translation.x;
			BoundingSphere.Center.y = BaseBoundingSphere->Center.y * Scale.y + Translation.y;
			BoundingSphere.Center.z = BaseBoundingSphere->Center.z * Scale.z + Translation.z;
			BoundingSphere.Radius = BaseBoundingSphere->Radius * Scale.Length();
		}

		float rx = DirectX::XMConvertToRadians(Rotate.x);
		float ry = DirectX::XMConvertToRadians(Rotate.y);
		float rz = DirectX::XMConvertToRadians(Rotate.z);
		RotationQuat = DirectX::XMQuaternionRotationRollPitchYaw(rx, ry, rz);

		DirectX::XMMATRIX rotX = DirectX::XMMatrixRotationX(rx);
		DirectX::XMMATRIX rotY = DirectX::XMMatrixRotationY(ry);
		DirectX::XMMATRIX rotZ = DirectX::XMMatrixRotationZ(rz);

		DirectX::XMMATRIX rot = rotX * rotY * rotZ;
		if (useQuat)
			rot = DirectX::XMMatrixRotationQuaternion(RotationQuat);

		DirectX::XMMATRIX world
			= DirectX::XMMatrixScaling(Scale.x, Scale.y, Scale.z)
			* rot
			* DirectX::XMMatrixTranslation(Translation.x, Translation.y, Translation.z);
		DirectX::XMMATRIX texTransform = DirectX::XMMatrixScaling(TexScale.x, TexScale.y, TexScale.z);
		DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(world);

		InstanceData.World = DirectX::XMMatrixTranspose(world);
		InstanceData.TexTransform = DirectX::XMMatrixTranspose(texTransform);
		InstanceData.WorldInvTranspose = DirectX::XMMatrixInverse(&det, world);
	}
};
struct RenderItem
{
	RenderItem() = default;
	RenderItem(const RenderItem& rhs) = delete;
	RenderItem(DX12_MeshGeometry* geo, const DX12_MeshComponent& component, bool culling = true)
		: Geo(geo)
		, Component(component)
		, FrustumCullingEnabled(culling)
	{
	}

	void Push(float3 translation, float3 scale,
		DirectX::SimpleMath::Quaternion rot, float3 texScale, UINT boundingCount = 0, UINT matIdx = 0, bool cull = true)
	{
		Datas.push_back(InstanceComponent(&Component.BoundingBox, &Component.BoundingSphere, translation, scale, rot, texScale, boundingCount, matIdx, cull));
	}

	void Push(InstanceComponent data)
	{
		Datas.push_back(data);
	}

	int NumFramesDirty = APP_NUM_BACK_BUFFERS;
	DX12_MeshGeometry* Geo = nullptr;
	DX12_MeshComponent Component;
	std::vector<InstanceComponent> Datas;
	bool FrustumCullingEnabled = false;

	UINT StartIndexLocation = 0;
	UINT IndexCount = 0;
	int BaseVertexLocation = 0;
	UINT StartInstanceLocation = 0;
	UINT InstanceCount = 0;
	
	// // TODO: 
	// // Skinned Mesh
	// UINT SkinnedCBIndex = -1;
	// SkinnedModelInstance* SkinnedModelInst = nullptr;
};