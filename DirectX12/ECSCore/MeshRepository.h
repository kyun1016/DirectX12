#pragma once

#include "../EngineCore/SimpleMath.h"
#include "MeshComponent.h"
#include <memory>
#include <string>

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
	int refCount = 1;
	bool gpuUploaded = false;
};

class MeshRepository {
public:

	static bool IsLoaded(MeshHandle handle) {
		auto it = sMeshStorage.find(handle);
		return it != sMeshStorage.end();
	}

	static MeshHandle LoadMesh(const std::string& path)
	{
		auto it = sPathToHandle.find(path);
		if (it != sPathToHandle.end()) {
			MeshHandle handle = it->second;
			sMeshStorage[handle].refCount++;
			return handle;
		}

		MeshHandle newHandle = sNextHandle++;
		std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>();

		// TODO: GPU 업로드 로직 구현

		sMeshStorage[newHandle] = MeshEntry{ std::move(mesh), 1 };
		sPathToHandle[path] = newHandle;

		return newHandle;
	}
	static void UnloadMesh(MeshHandle handle)
	{
		// TODO: GPU 언로드 로직 구현
		auto it = sMeshStorage.find(handle);
		if (it == sMeshStorage.end())
			return;

		it->second.refCount--;
		if (it->second.refCount <= 0) {
			// TODO: GPU 리소스 해제 (예: DestroyBuffer(mesh->vertexBuffer))

			sMeshStorage.erase(it);
			// 역방향 매핑 제거
			for (auto pathIt = sPathToHandle.begin(); pathIt != sPathToHandle.end(); ++pathIt) {
				if (pathIt->second == handle) {
					sPathToHandle.erase(pathIt);
					break;
				}
			}
		}
	}
	static Mesh* GetMesh(MeshHandle handle)
	{
		auto it = sMeshStorage.find(handle);
		if (it != sMeshStorage.end()) {
			return it->second.mesh.get();
		}
		return nullptr;
	}

	static void UploadToGPUIfNeeded(MeshHandle handle)
	{
		// TODO: GPU 업로드 로직 구현
	}

	static void Shutdown() {
		sPathToHandle.clear();
		sMeshStorage.clear();
		sNextHandle = 1;
	}

private:
	static inline std::unordered_map<std::string, MeshHandle> sPathToHandle;
	static inline std::unordered_map<MeshHandle, MeshEntry> sMeshStorage;
	static inline MeshHandle sNextHandle = 1;
};