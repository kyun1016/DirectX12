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

struct MaterialComponent {
	uint32_t MaterialID;
};

struct BoundingComponent {
	DirectX::BoundingBox AABB;
	DirectX::BoundingSphere Sphere;
	bool FrustumCullingEnabled;
};