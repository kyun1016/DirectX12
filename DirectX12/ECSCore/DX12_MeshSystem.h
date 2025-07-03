#pragma once
#include "DX12_MeshRepository.h"

class DX12_MeshSystem {
	DEFAULT_SINGLETON(DX12_MeshSystem)
public:
	void Initialize() {
		// BuildSquereMeshes();
		BuildSpriteMesh();
	}

	void BuildSquereMeshes()
	{
		std::vector<MeshData> meshes;
		meshes.push_back(DX12_MeshGenerator::CreateSquare(100.0f));
		meshes.push_back(DX12_MeshGenerator::CreateSquare(2000.0f));
		DX12_MeshRepository::GetInstance().LoadMesh("Square", meshes, DX12_MeshRepository::eMeshType::STANDARD, false);
	}

	void BuildBoxMeshes()
	{
		std::vector<MeshData> meshes;
		meshes.push_back(DX12_MeshGenerator::CreateBox(1.0f, 1.0f, 1.0f, 3));
		meshes.push_back(DX12_MeshGenerator::CreateBox(1.0f, 2.0f, 1.0f, 3));
		DX12_MeshRepository::GetInstance().LoadMesh("Box", meshes, DX12_MeshRepository::eMeshType::STANDARD, false);
	}

	void BuildSpriteMesh()
	{
		std::vector<MeshData> meshes;
		meshes.push_back(DX12_MeshGenerator::CreateSprite(10.0f, 10.0f));
		meshes.push_back(DX12_MeshGenerator::CreateSprite(20.0f, 20.0f));
		DX12_MeshRepository::GetInstance().LoadMesh("Sprite", meshes, DX12_MeshRepository::eMeshType::SPRITE, false);
	}

	//inline void SetMesh(ECS::RepoHandle meshID) {
	//	auto mesh = DX12_MeshRepository::GetInstance().Get(meshID);
	//	mLastVertexBufferView = mesh->VertexBufferView();
	//	mLastIndexBufferView = mesh->IndexBufferView();
	//	mLastPrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	//	mCommandList->IASetVertexBuffers(0, 1, &mLastVertexBufferView);
	//	mCommandList->IASetIndexBuffer(&mLastIndexBufferView);
	//	mCommandList->IASetPrimitiveTopology(mLastPrimitiveType);
	//}

	inline DX12_MeshGeometry* GetGeometry(ECS::RepoHandle handle) const {
		return DX12_MeshRepository::GetInstance().Get(handle);
	}
	inline DX12_MeshGeometry* GetGeometry(DX12_MeshHandle handle) const {
		return DX12_MeshRepository::GetInstance().Get(handle.GeometryHandle);
	}
	inline DX12_MeshComponent& GetMeshComponent(DX12_MeshHandle handle) const {
		return DX12_MeshRepository::GetInstance().Get(handle.GeometryHandle)->DrawArgs[handle.MeshHandle];
	}
	
};