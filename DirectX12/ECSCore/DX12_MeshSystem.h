#pragma once
#include "DX12_MeshRepository.h"

class DX12_MeshSystem {
	DEFAULT_SINGLETON(DX12_MeshSystem)
public:
	void Initialize(ID3D12GraphicsCommandList6* commandList) {
		mCommandList = commandList;
		std::vector<MeshData> meshes;
		meshes.push_back(DX12_MeshGenerator::CreateBox(1.0f, 1.0f, 1.0f, 3));
		meshes.push_back(DX12_MeshGenerator::CreateBox(1.0f, 2.0f, 1.0f, 3));

		DX12_MeshRepository::GetInstance().LoadMesh("Box", meshes, false, false);
	}

	inline void SetupMesh(uint32_t meshID, const std::string& rootSigName) {
		auto mesh = DX12_MeshRepository::GetInstance().Get(meshID);
		mCommandList->SetGraphicsRootSignature(DX12_RootSignatureSystem::GetInstance().GetGraphicsSignature(rootSigName));

		mLastVertexBufferView = mesh->VertexBufferView();
		mLastIndexBufferView = mesh->IndexBufferView();
		mLastPrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		mCommandList->IASetVertexBuffers(0, 1, &mLastVertexBufferView);
		mCommandList->IASetIndexBuffer(&mLastIndexBufferView);
		mCommandList->IASetPrimitiveTopology(mLastPrimitiveType);
	}


private:
	ID3D12GraphicsCommandList6* mCommandList;

	D3D12_VERTEX_BUFFER_VIEW mLastVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW mLastIndexBufferView;
	D3D12_PRIMITIVE_TOPOLOGY mLastPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
};