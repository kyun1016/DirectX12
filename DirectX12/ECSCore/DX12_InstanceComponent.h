#pragma once
#include "DX12_Config.h"
#include "DX12_InstanceData.h"

struct DX12_InstanceIndexComponent {
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
	// mCommandList->DrawIndexedInstanced(ri->IndexCount, ri->InstanceCount, ri->StartIndexLocation, ri->BaseVertexLocation, 0);	
};
//
//
//
//struct InstanceObject
//{
//	InstanceData InstanceData; // GPU 전송 전용 데이터
//
//	DirectX::BoundingBox* BaseBoundingBox;
//	DirectX::BoundingSphere* BaseBoundingSphere;
//	DirectX::BoundingBox BoundingBox;
//	DirectX::BoundingSphere BoundingSphere;
//
//	DirectX::SimpleMath::Vector3 Translation;
//	DirectX::SimpleMath::Vector3 Scale;
//	DirectX::SimpleMath::Vector3 Rotate;
//	DirectX::SimpleMath::Quaternion RotationQuat;
//	DirectX::SimpleMath::Vector3 TexScale;
//	UINT BoundingCount;	// 추후 BoundingBox, BoundingSphere 표현을 위한 구조에서 연동하여 활용
//	bool FrustumCullingEnabled;
//	bool ShowBoundingBox;
//	bool ShowBoundingSphere;
//	bool IsPickable;
//	bool useQuat;
//
//private:
//	void Update()
//	{
//
//		float rx = DirectX::XMConvertToRadians(Rotate.x);
//		float ry = DirectX::XMConvertToRadians(Rotate.y);
//		float rz = DirectX::XMConvertToRadians(Rotate.z);
//		RotationQuat = DirectX::XMQuaternionRotationRollPitchYaw(rx, ry, rz);
//
//		DirectX::XMMATRIX rotX = DirectX::XMMatrixRotationX(rx);
//		DirectX::XMMATRIX rotY = DirectX::XMMatrixRotationY(ry);
//		DirectX::XMMATRIX rotZ = DirectX::XMMatrixRotationZ(rz);
//
//		DirectX::XMMATRIX rot = rotX * rotY * rotZ;
//		if (useQuat)
//			rot = DirectX::XMMatrixRotationQuaternion(RotationQuat);
//
//		DirectX::XMMATRIX world
//			= DirectX::XMMatrixScaling(Scale.x, Scale.y, Scale.z)
//			* rot
//			* DirectX::XMMatrixTranslation(Translation.x, Translation.y, Translation.z);
//		DirectX::XMMATRIX texTransform = DirectX::XMMatrixScaling(TexScale.x, TexScale.y, TexScale.z);
//		DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(world);
//
//		InstanceData.World = DirectX::XMMatrixTranspose(world);
//		InstanceData.TexTransform = DirectX::XMMatrixTranspose(texTransform);
//		InstanceData.WorldInvTranspose = DirectX::XMMatrixInverse(&det, world);
//	}
//};