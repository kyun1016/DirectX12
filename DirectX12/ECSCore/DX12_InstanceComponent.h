#pragma once
#include "DX12_Config.h"
#include "DX12_InstanceData.h"

struct DX12_BoundingComponent {
	DirectX::BoundingBox Box;
	DirectX::BoundingSphere Sphere;
	bool FrustumCullingEnabled;
	bool ShowBoundingBox;
	bool ShowBoundingSphere;
};

struct DX12_RenderInstanceComponent {
	InstanceData GPUData;
	bool IsPickable;
};

struct DX12_InstanceIndexComponent {
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
	// mCommandList->DrawIndexedInstanced(ri->IndexCount, ri->InstanceCount, ri->StartIndexLocation, ri->BaseVertexLocation, 0);	
};



struct InstanceObject
{
	InstanceObject() = delete;
	InstanceObject(DirectX::SimpleMath::Vector3 translation, DirectX::SimpleMath::Vector3 scale, DirectX::SimpleMath::Quaternion rot, DirectX::SimpleMath::Vector3 texScale, UINT boundingCount = 0, UINT matIdx = 0, bool cull = true)
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
	InstanceObject(DirectX::BoundingBox* baseBoundingBox, DirectX::BoundingSphere* baseBoundingSphere, DirectX::SimpleMath::Vector3 translation, DirectX::SimpleMath::Vector3 scale, DirectX::SimpleMath::Quaternion rot, DirectX::SimpleMath::Vector3 texScale, UINT boundingCount = 0, UINT matIdx = 0, bool cull = true)
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

	DirectX::SimpleMath::Vector3 Translation;
	DirectX::SimpleMath::Vector3 Scale;
	DirectX::SimpleMath::Vector3 Rotate;
	DirectX::SimpleMath::Quaternion RotationQuat;
	DirectX::SimpleMath::Vector3 TexScale;
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