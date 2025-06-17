#pragma once
#include "DX12_Config.h"

#include "DX12_InstanceData.h"
#include "DX12_MaterialData.h"

struct DX12_MeshComponent
{
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	INT BaseVertexLocation = 0;

	// Bounding box of the geometry defined by this submesh. 
	// This is used in later chapters of the book.
	DirectX::BoundingBox BoundingBox;
	DirectX::BoundingSphere BoundingSphere;
};

struct MeshGeometry
{
	Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

	// Data about the buffers.
	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
	UINT IndexBufferByteSize = 0;

	// A MeshGeometry may store multiple geometries in one vertex/index buffer.
	// Use this container to define the Submesh geometries so we can draw
	// the Submeshes individually.
	std::unordered_map<std::string, DX12_MeshComponent> DrawArgs;

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView()const
	{
		D3D12_VERTEX_BUFFER_VIEW vbv
		{
			/* D3D12_GPU_VIRTUAL_ADDRESS BufferLocation	*/ .BufferLocation = VertexBufferGPU->GetGPUVirtualAddress(),
			/* UINT SizeInBytes							*/ .SizeInBytes = VertexBufferByteSize,
			/* UINT StrideInBytes						*/ .StrideInBytes = VertexByteStride
		};
		return vbv;
	}

	D3D12_INDEX_BUFFER_VIEW IndexBufferView()const
	{
		D3D12_INDEX_BUFFER_VIEW ibv
		{
			/* D3D12_GPU_VIRTUAL_ADDRESS BufferLocation	*/ .BufferLocation = IndexBufferGPU->GetGPUVirtualAddress(),
			/* UINT SizeInBytes							*/ .SizeInBytes = IndexBufferByteSize,
			/* DXGI_FORMAT Format						*/ .Format = IndexFormat
		};
		return ibv;
	}

	// We can free this memory after we finish upload to the GPU.
	void DisposeUploaders()
	{
		VertexBufferUploader = nullptr;
		IndexBufferUploader = nullptr;
	}
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

struct MaterialComponent {
	uint32_t MaterialID;
};

struct BoundingComponent {
	DirectX::BoundingBox AABB;
	DirectX::BoundingSphere Sphere;
	bool FrustumCullingEnabled;
};