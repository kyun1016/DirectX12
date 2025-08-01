#pragma once
#include "DX12_Config.h"

#include "MeshData.h"

struct MeshData
{
	std::vector<Vertex> Vertices;
	std::vector<SkinnedVertex> SkinnedVertices;
	std::vector<SpriteVertex> SpriteVertices;
	std::vector<std::uint32_t> Indices32;

	std::vector<std::uint16_t>& GetIndices16()
	{
		if (mIndices16.empty())
		{
			mIndices16.resize(Indices32.size());
			for (size_t i = 0; i < Indices32.size(); ++i)
				mIndices16[i] = static_cast<std::uint16_t>(Indices32[i]);
		}

		return mIndices16;
	}

private:
	std::vector<std::uint16_t> mIndices16;
};

struct DX12_MeshComponent
{
	static const char* GetName() { return "DX12_MeshComponent"; }
	UINT StartIndexLocation = 0;
	UINT IndexCount = 0;
	UINT StartInstanceLocation = 0;
	UINT InstanceCount = 0;
	INT BaseVertexLocation = 0;
	DirectX::BoundingBox BoundingBox;
	DirectX::BoundingSphere BoundingSphere;
};

struct DX12_MeshGeometry
{
	Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
	UINT IndexBufferByteSize = 0;

	std::vector<DX12_MeshComponent> DrawArgs;

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const
	{
		D3D12_VERTEX_BUFFER_VIEW vbv
		{
			/* D3D12_GPU_VIRTUAL_ADDRESS BufferLocation	*/ .BufferLocation = VertexBufferGPU->GetGPUVirtualAddress(),
			/* UINT SizeInBytes							*/ .SizeInBytes = VertexBufferByteSize,
			/* UINT StrideInBytes						*/ .StrideInBytes = VertexByteStride
		};
		return vbv;
	}

	D3D12_INDEX_BUFFER_VIEW IndexBufferView() const
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

inline void to_json(json& j, const DX12_MeshComponent& p) {
	j = json{
		{ "StartIndexLocation",		p.StartIndexLocation },
		{ "IndexCount",				p.IndexCount },
		{ "StartInstanceLocation",	p.StartInstanceLocation },
		{ "InstanceCount",			p.InstanceCount },
		{ "BaseVertexLocation",		p.BaseVertexLocation },
		{ "BoundingBox", {p.BoundingBox.Center.x, p.BoundingBox.Center.y, p.BoundingBox.Center.z,
						 p.BoundingBox.Extents.x, p.BoundingBox.Extents.y, p.BoundingBox.Extents.z} },
		{ "BoundingSphere", {p.BoundingSphere.Center.x, p.BoundingSphere.Center.y, p.BoundingSphere.Center.z,
							p.BoundingSphere.Radius}}
	};
}

inline void from_json(const json& j, DX12_MeshComponent& p) {
	j.at("StartIndexLocation").get_to(p.StartIndexLocation);
	j.at("IndexCount").get_to(p.IndexCount);
	j.at("StartInstanceLocation").get_to(p.StartInstanceLocation);
	j.at("InstanceCount").get_to(p.InstanceCount);
	j.at("BaseVertexLocation").get_to(p.BaseVertexLocation);
	auto boundingBox = j.at("BoundingBox");
	p.BoundingBox.Center = { boundingBox[0], boundingBox[1], boundingBox[2] };
	p.BoundingBox.Extents = { boundingBox[3], boundingBox[4], boundingBox[5] };
	auto boundingSphere = j.at("BoundingSphere");
	p.BoundingSphere.Center = { boundingSphere[0], boundingSphere[1], boundingSphere[2] };
	p.BoundingSphere.Radius = boundingSphere[3];
}