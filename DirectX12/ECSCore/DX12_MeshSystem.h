#pragma once
#include "DX12_MeshRepository.h"

class DX12_MeshSystem {
	DEFAULT_SINGLETON(DX12_MeshSystem)
public:
	void Initialize() {
		mGeoCount.emplace_back(0);
		BuildSpriteMesh();
		BuildSquereMeshes();
	}

	void BuildSquereMeshes()
	{
		std::vector<MeshData> meshes;
		meshes.push_back(DX12_MeshGenerator::CreateSquare(100.0f));
		meshes.push_back(DX12_MeshGenerator::CreateSquare(2000.0f));
		DX12_MeshRepository::GetInstance().LoadMesh("Square", meshes, DX12_MeshRepository::eMeshType::STANDARD, false);
		mGeoCount.emplace_back(meshes.size());
	}

	void BuildBoxMeshes()
	{
		std::vector<MeshData> meshes;
		meshes.push_back(DX12_MeshGenerator::CreateBox(1.0f, 1.0f, 1.0f, 3));
		meshes.push_back(DX12_MeshGenerator::CreateBox(1.0f, 2.0f, 1.0f, 3));
		DX12_MeshRepository::GetInstance().LoadMesh("Box", meshes, DX12_MeshRepository::eMeshType::STANDARD, false);
		mGeoCount.emplace_back(meshes.size());
	}

	void BuildSpriteMesh()
	{
		std::vector<MeshData> meshes;
		meshes.push_back(DX12_MeshGenerator::CreateSprite(1.0f, 1.0f));
		meshes.push_back(DX12_MeshGenerator::CreateSprite(0.5f, 0.5f));

		meshes.push_back(DX12_MeshGenerator::CreateSprite(0.3f, 0.3f));
		// meshes.push_back(DX12_MeshGenerator::CreateSquare(100.0f));
		DX12_MeshRepository::GetInstance().LoadMesh("Sprite", meshes, DX12_MeshRepository::eMeshType::SPRITE, false);
		mGeoCount.emplace_back(meshes.size());
	}

	inline DX12_MeshGeometry* GetGeometry(ECS::RepoHandle handle) const {
		return DX12_MeshRepository::GetInstance().Get(handle);
	}
	inline DX12_MeshGeometry* GetGeometry(DX12_MeshHandle handle) const {
		return GetGeometry(handle.GeometryHandle);
	}
	inline DX12_MeshComponent* GetMeshComponent(DX12_MeshHandle handle) const {
		auto* geo = GetGeometry(handle);
		if (geo->DrawArgs.size() > handle.MeshHandle)
			return &geo->DrawArgs[handle.MeshHandle];
		return nullptr;
	}

	const std::vector<uint32_t>& GetMeshCount() const {
		return mGeoCount;
	}
private:
	std::vector<uint32_t> mGeoCount;
};