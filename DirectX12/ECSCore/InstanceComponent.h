#pragma once
#include "DX12_Config.h"
#include "InstanceData.h"
#include "DX12_MeshSystem.h"
#include "TransformComponent.h"

struct BoundingVolumnComponent
{
	DirectX::BoundingBox BoundingBox;
	DirectX::BoundingSphere BoundingSphere;
};

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
	InstanceComponent()
	{
		Transform.Position = {};
		Transform.Scale = { 1.0f, 1.0f, 1.0f };
		Transform.RotationQuat = { 0.0f, 0.0f, 0.0f, 1.0f };
		UpdateTransform();
	};

	bool UpdateTransform()
	{
		if (!Transform.Dirty)
			return false;

		if (MeshHandle.GeometryHandle != 0)
		{
			BoundingBox = DX12_MeshSystem::GetInstance().GetMeshComponent(MeshHandle)->BoundingBox;
			BoundingBox.Center.x *= Transform.Scale.x;
			BoundingBox.Center.y *= Transform.Scale.y;
			BoundingBox.Center.z *= Transform.Scale.z;
			BoundingBox.Center.x += Transform.Position.x;
			BoundingBox.Center.y += Transform.Position.y;
			BoundingBox.Center.z += Transform.Position.z;

			BoundingBox.Extents.x *= Transform.Scale.x;
			BoundingBox.Extents.y *= Transform.Scale.y;
			BoundingBox.Extents.z *= Transform.Scale.z;
		}
		if (MeshHandle.GeometryHandle != 0)
		{
			BoundingSphere = DX12_MeshSystem::GetInstance().GetMeshComponent(MeshHandle)->BoundingSphere;
			BoundingSphere.Center.x *= Transform.Scale.x;
			BoundingSphere.Center.y *= Transform.Scale.y;
			BoundingSphere.Center.z *= Transform.Scale.z;
			BoundingSphere.Center.x += Transform.Position.x;
			BoundingSphere.Center.y += Transform.Position.y;
			BoundingSphere.Center.z += Transform.Position.z;

			BoundingSphere.Radius *= Transform.Scale.Length();
		}

		float rx = DirectX::XMConvertToRadians(Transform.Rotation.x);
		float ry = DirectX::XMConvertToRadians(Transform.Rotation.y);
		float rz = DirectX::XMConvertToRadians(Transform.Rotation.z);
		Transform.RotationQuat = DirectX::XMQuaternionRotationRollPitchYaw(rx, ry, rz);

		DirectX::XMMATRIX rotX = DirectX::XMMatrixRotationX(rx);
		DirectX::XMMATRIX rotY = DirectX::XMMatrixRotationY(ry);
		DirectX::XMMATRIX rotZ = DirectX::XMMatrixRotationZ(rz);

		DirectX::XMMATRIX rot = rotX * rotY * rotZ;
		if (Option & eCFGInstanceComponent::UseQuat)
			rot = DirectX::XMMatrixRotationQuaternion(Transform.RotationQuat);

		DirectX::XMMATRIX world
			= DirectX::XMMatrixScaling(Transform.Scale.x, Transform.Scale.y, Transform.Scale.z)
			* rot
			* DirectX::XMMatrixTranslation(Transform.Position.x, Transform.Position.y, Transform.Position.z);
		DirectX::XMMATRIX texTransform = DirectX::XMMatrixScaling(TexScale.x, TexScale.y, TexScale.z);
		DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(world);

		InstanceData.World = DirectX::XMMatrixTranspose(world);
		InstanceData.TexTransform = DirectX::XMMatrixTranspose(texTransform);
		InstanceData.WorldInvTranspose = DirectX::XMMatrixInverse(&det, world);

		return true;
	}

	InstanceData InstanceData; // GPU 전송 전용 데이터
	DX12_MeshHandle MeshHandle;
	DirectX::BoundingBox BoundingBox;
	DirectX::BoundingSphere BoundingSphere;
	TransformComponent Transform;
	float3 TexScale;
	// Metarial 속성 추가
	eCFGInstanceComponent Option = eCFGInstanceComponent::UseCulling;
};