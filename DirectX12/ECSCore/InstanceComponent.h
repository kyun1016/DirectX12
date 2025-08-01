#pragma once
#include "DX12_Config.h"
#include "InstanceData.h"
#include "DX12_MeshSystem.h"
#include "TransformComponent.h"

struct BoundingVolumnComponent
{
	static const char* GetName() { return "BoundingVolumnComponent"; }
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
struct CFGInstanceComponent
{
	static const char* GetName() { return "CFGInstanceComponent"; }
	eCFGInstanceComponent Option = eCFGInstanceComponent::None;
};

inline void to_json(json& j, const BoundingVolumnComponent& p)
{
	j = json{
		{"BoundingBox", {p.BoundingBox.Center.x, p.BoundingBox.Center.y, p.BoundingBox.Center.z,
						 p.BoundingBox.Extents.x, p.BoundingBox.Extents.y, p.BoundingBox.Extents.z}},
		{"BoundingSphere", {p.BoundingSphere.Center.x, p.BoundingSphere.Center.y, p.BoundingSphere.Center.z,
							p.BoundingSphere.Radius}}
	};
}
inline void from_json(const json& j, BoundingVolumnComponent& p)
{
	auto boundingBox = j.at("BoundingBox");
	p.BoundingBox.Center = { boundingBox[0], boundingBox[1], boundingBox[2] };
	p.BoundingBox.Extents = { boundingBox[3], boundingBox[4], boundingBox[5] };
	auto boundingSphere = j.at("BoundingSphere");
	p.BoundingSphere.Center = { boundingSphere[0], boundingSphere[1], boundingSphere[2] };
	p.BoundingSphere.Radius = boundingSphere[3];
}
inline void to_json(json& j, const CFGInstanceComponent& p)
{
	j = static_cast<std::uint32_t>(p.Option);
}
inline void from_json(const json& j, CFGInstanceComponent& p)
{
	p.Option = static_cast<eCFGInstanceComponent>(j.get<std::uint32_t>());
}
ENUM_OPERATORS_32(eCFGInstanceComponent)

struct TextureScaleComponent
{
	static const char* GetName() { return "TextureScaleComponent"; }
	float3 TextureScale;
};
inline void to_json(json& j, const TextureScaleComponent& p)
{
	j = json{
		{"TextureScale", {p.TextureScale.x, p.TextureScale.y, p.TextureScale.z}}
	};
}
inline void from_json(const json& j, TextureScaleComponent& p)
{
	auto textureScale = j.at("TextureScale");
	p.TextureScale = { textureScale[0], textureScale[1], textureScale[2] };
}
struct InstanceComponent
{
	static const char* GetName() { return "InstanceComponent"; }
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

		if (GeometryHandle != 0 && MeshHandle != 0) {
			BoundingBox = DX12_MeshSystem::GetInstance().GetMeshComponent(GeometryHandle, MeshHandle)->BoundingBox;
			BoundingBox.Center.x *= Transform.Scale.x;
			BoundingBox.Center.y *= Transform.Scale.y;
			BoundingBox.Center.z *= Transform.Scale.z;
			BoundingBox.Center.x += Transform.Position.x;
			BoundingBox.Center.y += Transform.Position.y;
			BoundingBox.Center.z += Transform.Position.z;

			BoundingBox.Extents.x *= Transform.Scale.x;
			BoundingBox.Extents.y *= Transform.Scale.y;
			BoundingBox.Extents.z *= Transform.Scale.z;

			BoundingSphere = DX12_MeshSystem::GetInstance().GetMeshComponent(GeometryHandle, MeshHandle)->BoundingSphere;
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
	// DX12_MeshHandle MeshHandle;
	ECS::RepoHandle GeometryHandle = 0; // Geometry Handle
	ECS::RepoHandle MeshHandle = 0;
	DirectX::BoundingBox BoundingBox;
	DirectX::BoundingSphere BoundingSphere;
	TransformComponent Transform;
	float3 TexScale;
	// Metarial 속성 추가
	eCFGInstanceComponent Option = eCFGInstanceComponent::UseCulling;
};

inline void to_json(json& j, const InstanceComponent& p)
{

}
inline void from_json(const json& j, InstanceComponent& p)
{

}