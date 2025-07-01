#pragma once
#include "DX12_Config.h"
#include "DX12_InstanceData.h"
#include "DX12_MeshComponent.h"
#include "DX12_PSOSystem.h"

#ifndef SKINNEDDATA_H
#define SKINNEDDATA_H
struct Keyframe
{
	Keyframe();
	~Keyframe();

	float TimePos;
	DirectX::XMFLOAT3 Translation;
	DirectX::XMFLOAT3 Scale;
	DirectX::XMFLOAT4 RotationQuat;
};

struct BoneAnimation
{
	float GetStartTime()const;
	float GetEndTime()const;

	void Interpolate(float t, DirectX::XMFLOAT4X4& M)const;

	std::vector<Keyframe> Keyframes;
};

struct AnimationClip
{
	float GetClipStartTime()const;
	float GetClipEndTime()const;

	void Interpolate(float t, std::vector<DirectX::XMFLOAT4X4>& boneTransforms)const;

	std::vector<BoneAnimation> BoneAnimations;
};

class SkinnedData
{
public:

	UINT BoneCount()const;

	float GetClipStartTime(const std::string& clipName)const;
	float GetClipEndTime(const std::string& clipName)const;

	void Set(
		std::vector<int>& boneHierarchy,
		std::vector<DirectX::XMFLOAT4X4>& boneOffsets,
		std::unordered_map<std::string, AnimationClip>& animations);

	void GetFinalTransforms(const std::string& clipName, float timePos,
		std::vector<DirectX::XMFLOAT4X4>& finalTransforms)const;

private:
	std::vector<int> mBoneHierarchy;
	std::vector<DirectX::XMFLOAT4X4> mBoneOffsets;
	std::unordered_map<std::string, AnimationClip> mAnimations;
};

#endif // SKINNEDDATA_H


struct SkinnedModelInstance
{
	SkinnedData SkinnedInfo;
	std::vector<DirectX::XMFLOAT4X4> FinalTransforms;
	std::string ClipName;
	float TimePos = 0.0f;

	// Called every frame and increments the time position, interpolates the 
	// animations for each bone based on the current animation clip, and 
	// generates the final transforms which are ultimately set to the effect
	// for processing in the vertex shader.
	void UpdateSkinnedAnimation(float dt)
	{
		TimePos += dt;

		// Loop animation
		if (TimePos > SkinnedInfo.GetClipEndTime(ClipName))
			TimePos = 0.0f;

		// Compute the final transforms for this time position.
		SkinnedInfo.GetFinalTransforms(ClipName, TimePos, FinalTransforms);
	}
};

struct SceneInstance
{
	SceneInstance() = delete;
	SceneInstance(float3 translation, float3 scale, DirectX::SimpleMath::Quaternion rot, float3 texScale, UINT boundingCount = 0, UINT matIdx = 0, bool cull = true)
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
		Update();
	}
	SceneInstance(DirectX::BoundingBox* baseBoundingBox, DirectX::BoundingSphere* baseBoundingSphere, float3 translation, float3 scale, DirectX::SimpleMath::Quaternion rot, float3 texScale, UINT boundingCount = 0, UINT matIdx = 0, bool cull = true)
		: BaseBoundingBox(baseBoundingBox)
		, BaseBoundingSphere(baseBoundingSphere)
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
		InstanceData.MaterialIndex = matIdx;
		Update();
	}
	void UpdateTranslation(DirectX::SimpleMath::Vector3 translation)
	{
		Translation = translation;
		Update();
	}
	void UpdateScale(DirectX::SimpleMath::Vector3 scale)
	{
		Scale = scale;
		Update();
	}
	void UpdateRotate(DirectX::SimpleMath::Vector3 rotate)
	{
		Rotate = rotate;
		Update();
	}
	void UpdateTexScale(DirectX::SimpleMath::Vector3 scale)
	{
		TexScale = scale;
		Update();
	}

	InstanceData InstanceData; // GPU 전송 전용 데이터

	DirectX::BoundingBox* BaseBoundingBox;
	DirectX::BoundingSphere* BaseBoundingSphere;
	DirectX::BoundingBox BoundingBox;
	DirectX::BoundingSphere BoundingSphere;

	float3 Translation;
	float3 Scale;
	float3 Rotate;
	float3 TexScale;
	DirectX::SimpleMath::Quaternion RotationQuat;
	
	UINT BoundingCount;	// 추후 BoundingBox, BoundingSphere 표현을 위한 구조에서 연동하여 활용
	bool FrustumCullingEnabled;
	bool ShowBoundingBox;
	bool ShowBoundingSphere;
	bool IsPickable;
	bool useQuat;

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

	void Push(DirectX::SimpleMath::Vector3 translation, DirectX::SimpleMath::Vector3 scale,
		DirectX::SimpleMath::Quaternion rot, DirectX::SimpleMath::Vector3 texScale, UINT boundingCount = 0, UINT matIdx = 0, bool cull = true)
	{
		Datas.push_back(SceneInstance(&Component.BoundingBox, &Component.BoundingSphere, translation, scale, rot, texScale, boundingCount, matIdx, cull));
	}

	void Push(SceneInstance data)
	{
		Datas.push_back(data);
	}

	int NumFramesDirty = APP_NUM_BACK_BUFFERS;
	DX12_MeshGeometry* Geo = nullptr;
	DX12_MeshComponent Component;
	std::vector<SceneInstance> Datas;
	bool FrustumCullingEnabled = false;

	eRenderLayer RenderLayer
		= eRenderLayer::Opaque
		| eRenderLayer::Reflected
		| eRenderLayer::Normal
		| eRenderLayer::OpaqueWireframe
		| eRenderLayer::ReflectedWireframe
		| eRenderLayer::NormalWireframe;

	// TODO: 
	// Skinned Mesh
	UINT SkinnedCBIndex = -1;
	SkinnedModelInstance* SkinnedModelInst = nullptr;
};