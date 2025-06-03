#pragma once

#include "../EngineCore/SimpleMath.h"
#include "RenderComponent.h"

struct Vertex
{
	Vertex()
		: Position(0.0f, 0.0f, 0.0f)
		, Normal(0.0f, 0.0f, 0.0f)
		, TangentU(0.0f, 0.0f, 0.0f)
		, TexC(0.0f, 0.0f) {
	}
	Vertex(
		const DirectX::SimpleMath::Vector3& p,
		const DirectX::SimpleMath::Vector3& n,
		const DirectX::SimpleMath::Vector3& t,
		const DirectX::SimpleMath::Vector2& uv) :
		Position(p),
		Normal(n),
		TangentU(t),
		TexC(uv) {
	}
	Vertex(
		float px, float py, float pz,
		float nx, float ny, float nz,
		float tx, float ty, float tz,
		float u, float v) :
		Position(px, py, pz),
		Normal(nx, ny, nz),
		TangentU(tx, ty, tz),
		TexC(u, v) {
	}

	DirectX::SimpleMath::Vector3 Position;
	DirectX::SimpleMath::Vector3 Normal;
	DirectX::SimpleMath::Vector2 TexC;
	DirectX::SimpleMath::Vector3 TangentU;
};

struct SkinnedVertex
{
	DirectX::SimpleMath::Vector3 Position;
	DirectX::SimpleMath::Vector3 Normal;
	DirectX::SimpleMath::Vector2 TexC;
	DirectX::SimpleMath::Vector3 TangentU;
	DirectX::SimpleMath::Vector3 BoneWeights;
	uint8_t BoneIndices[4];
};

struct Mesh
{
	union vertex {
		std::vector<Vertex> Vertices;
		std::vector<SkinnedVertex> SkinnedVertices;
	};

	union index {
		std::vector<std::uint16_t> Indices16;
		std::vector<std::uint32_t> Indices32;
	};

	// TODO: GPUBufferHandle vertexBuffer;
	// TODO: GPUBufferHandle indexBuffer;
};

struct MeshEntry {
	std::unique_ptr<Mesh> mesh;
	int refCount = 0;
	bool gpuUploaded = false;
};

class MeshRepository {
public:
	static MeshHandle LoadMesh(const std::string& path);
	static void UnloadMesh(MeshHandle handle);
	static Mesh* GetMesh(MeshHandle handle);
	static void UploadToGPUIfNeeded(MeshHandle handle);

private:
	static inline std::unordered_map<std::string, MeshHandle> sPathToHandle;
	static inline std::unordered_map<MeshHandle, MeshEntry> sMeshStorage;
	static inline MeshHandle sNextHandle = 1;
};