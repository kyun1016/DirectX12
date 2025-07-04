#pragma once
#include "DX12_Config.h"
#include "DX12_InstanceData.h"
#include "DX12_MeshSystem.h"

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

enum class eCFGInstanceComponent : std::uint32_t
{
	None = 0,
	UseCulling = 1 << 0,
	ShowBoundingBox = 1 << 1,
	ShowBoundingSphere = 1 << 2,
	Pickable = 1 << 2,
	UseQuat = 1 << 3
};
ENUM_OPERATORS_32(eCFGInstanceComponent)

struct InstanceComponent
{
	InstanceComponent() = delete;
	InstanceComponent(const float3& translation = {}, const float3& scale = { 1.0f, 1.0f, 1.0f }, const DirectX::SimpleMath::Quaternion& rot = { 0.0f, 0.0f, 0.0f, 1.0f }, const float3& texScale = { 1.0f, 1.0f, 1.0f })
		: TexScale(texScale)
	{
		Transform.r_Position = translation;
		Transform.r_Scale = scale;
		Transform.r_RotationQuat = rot;
		UpdateTransform();
	}
	InstanceComponent(const DX12_MeshHandle& meshHandle, const float3& translation = {}, const float3& scale = { 1.0f, 1.0f, 1.0f }, const DirectX::SimpleMath::Quaternion& rot = { 0.0f, 0.0f, 0.0f, 1.0f }, const float3& texScale = { 1.0f, 1.0f, 1.0f }, UINT matIdx = 0)
		: MeshHandle(meshHandle)
		, TexScale(texScale)
	{
		Transform.r_Position = translation;
		Transform.r_Scale = scale;
		Transform.r_RotationQuat = rot;
		InstanceData.MaterialIndex = matIdx;
		UpdateTransform();
	}

	void UpdateTexScale(const float3& scale)
	{
		TexScale = scale;
		UpdateTransform();
	}

	InstanceData InstanceData; // GPU 전송 전용 데이터
	DX12_MeshHandle MeshHandle;
	DirectX::BoundingBox BoundingBox;
	DirectX::BoundingSphere BoundingSphere;
	DX12_TransformComponent Transform;
	float3 TexScale;
	// Metarial 속성 추가
	eCFGInstanceComponent Option = eCFGInstanceComponent::UseCulling;

private:
	void UpdateTransform()
	{
		if (!Transform.Dirty)
			return;
		if (MeshHandle.GeometryHandle != 0)
		{
			BoundingBox = DX12_MeshSystem::GetInstance().GetMeshComponent(MeshHandle)->BoundingBox;
			BoundingBox.Center.x *= Transform.r_Scale.x;
			BoundingBox.Center.y *= Transform.r_Scale.y;
			BoundingBox.Center.z *= Transform.r_Scale.z;
			BoundingBox.Center.x += Transform.r_Position.x;
			BoundingBox.Center.y += Transform.r_Position.y;
			BoundingBox.Center.z += Transform.r_Position.z;

			BoundingBox.Extents.x *= Transform.r_Scale.x;
			BoundingBox.Extents.y *= Transform.r_Scale.y;
			BoundingBox.Extents.z *= Transform.r_Scale.z;
		}
		if (MeshHandle.GeometryHandle != 0)
		{
			BoundingSphere = DX12_MeshSystem::GetInstance().GetMeshComponent(MeshHandle)->BoundingSphere;
			BoundingSphere.Center.x *= Transform.r_Scale.x;
			BoundingSphere.Center.y *= Transform.r_Scale.y;
			BoundingSphere.Center.z *= Transform.r_Scale.z;
			BoundingSphere.Center.x += Transform.r_Position.x;
			BoundingSphere.Center.y += Transform.r_Position.y;
			BoundingSphere.Center.z += Transform.r_Position.z;

			BoundingSphere.Radius *= Transform.r_Scale.Length();
		}

		float rx = DirectX::XMConvertToRadians(Transform.r_RotationEuler.x);
		float ry = DirectX::XMConvertToRadians(Transform.r_RotationEuler.y);
		float rz = DirectX::XMConvertToRadians(Transform.r_RotationEuler.z);
		Transform.r_RotationQuat = DirectX::XMQuaternionRotationRollPitchYaw(rx, ry, rz);

		DirectX::XMMATRIX rotX = DirectX::XMMatrixRotationX(rx);
		DirectX::XMMATRIX rotY = DirectX::XMMatrixRotationY(ry);
		DirectX::XMMATRIX rotZ = DirectX::XMMatrixRotationZ(rz);

		DirectX::XMMATRIX rot = rotX * rotY * rotZ;
		if (Option & eCFGInstanceComponent::UseQuat)
			rot = DirectX::XMMatrixRotationQuaternion(Transform.r_RotationQuat);

		DirectX::XMMATRIX world
			= DirectX::XMMatrixScaling(Transform.r_Scale.x, Transform.r_Scale.y, Transform.r_Scale.z)
			* rot
			* DirectX::XMMatrixTranslation(Transform.r_Position.x, Transform.r_Position.y, Transform.r_Position.z);
		DirectX::XMMATRIX texTransform = DirectX::XMMatrixScaling(TexScale.x, TexScale.y, TexScale.z);
		DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(world);

		InstanceData.World = DirectX::XMMatrixTranspose(world);
		InstanceData.TexTransform = DirectX::XMMatrixTranspose(texTransform);
		InstanceData.WorldInvTranspose = DirectX::XMMatrixInverse(&det, world);
	}
};