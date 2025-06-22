#pragma once
#include "ECSCoordinator.h"
#include "ECSRepository.h"
#include "DX12_MeshComponent.h"
#include "DX12_MeshGenerator.h"
#include "DX12_DeviceSystem.h"
#include "DX12_CommandSystem.h"

class DX12_MeshRepository : public ECS::IRepository<DX12_MeshGeometry> {
    DEFAULT_SINGLETON(DX12_MeshRepository)

public:
	ECS::RepoHandle LoadMesh(const std::string& name, std::vector<MeshData>& meshes, bool useIndex32 = false, bool useSkinnedMesh = false) {
		std::lock_guard<std::mutex> lock(mtx);
		auto it = mNameToHandle.find(name);
		if (it != mNameToHandle.end()) {
			mResourceStorage[it->second].refCount++;
			return it->second;
		}

		auto geo = std::make_unique<DX12_MeshGeometry>();
		{
			//=========================================================
			// Part 1. vertices & indices 병합
			//=========================================================
			std::vector<Vertex> vertices;
			std::vector<SkinnedVertex> skinnedVertices;
			std::vector<std::uint16_t> indices16;
			std::vector<std::uint32_t> indices32;

			for (size_t i = 0; i < meshes.size(); ++i)
			{
				indices16.insert(indices16.end(), meshes[i].GetIndices16().begin(), meshes[i].GetIndices16().end());
				indices32.insert(indices32.end(), meshes[i].Indices32.begin(), meshes[i].Indices32.end());
				vertices.insert(vertices.end(), meshes[i].Vertices.begin(), meshes[i].Vertices.end());
				skinnedVertices.insert(skinnedVertices.end(), meshes[i].SkinnedVertices.begin(), meshes[i].SkinnedVertices.end());
			}

			//=========================================================
			// Part 2. GPU 할당 (16/32 bit)
			//=========================================================
			const UINT vbByteSize = useSkinnedMesh
				? (UINT)skinnedVertices.size() * sizeof(SkinnedVertex)
				: (UINT)vertices.size() * sizeof(Vertex);
			const UINT ibByteSize = useIndex32
				? (UINT)indices32.size() * sizeof(std::uint32_t)
				: (UINT)indices16.size() * sizeof(std::uint16_t);

			geo->VertexByteStride = useSkinnedMesh ? sizeof(SkinnedVertex) : sizeof(Vertex);
			geo->VertexBufferByteSize = vbByteSize;
			geo->IndexFormat = useIndex32 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
			geo->IndexBufferByteSize = ibByteSize;

			ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
			ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
			if (useSkinnedMesh)
			{
				CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), skinnedVertices.data(), vbByteSize);
				geo->VertexBufferGPU = CreateDefaultBuffer(DX12_DeviceSystem::GetInstance().GetDevice(), DX12_CommandSystem::GetInstance().GetCommandList(),
					skinnedVertices.data(), vbByteSize, geo->VertexBufferUploader);
			}
			else
			{
				CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
				geo->VertexBufferGPU = CreateDefaultBuffer(DX12_DeviceSystem::GetInstance().GetDevice(), DX12_CommandSystem::GetInstance().GetCommandList(),
					vertices.data(), vbByteSize, geo->VertexBufferUploader);
			}

			if (useIndex32)
			{
				CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices32.data(), ibByteSize);
				geo->IndexBufferGPU = CreateDefaultBuffer(DX12_DeviceSystem::GetInstance().GetDevice(), DX12_CommandSystem::GetInstance().GetCommandList(),
					indices32.data(), ibByteSize, geo->IndexBufferUploader);
			}
			else
			{
				CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices16.data(), ibByteSize);
				geo->IndexBufferGPU = CreateDefaultBuffer(DX12_DeviceSystem::GetInstance().GetDevice(), DX12_CommandSystem::GetInstance().GetCommandList(),
					indices16.data(), ibByteSize, geo->IndexBufferUploader);
			}

			//=========================================================
			// Part 3. SubmeshGeometry 생성
			//=========================================================
			std::vector<DX12_MeshComponent> submeshes(meshes.size());
			for (size_t i = 0; i < meshes.size(); ++i)
			{
				if (useSkinnedMesh)
					DX12_MeshGenerator::FindBounding(submeshes[i].BoundingBox, submeshes[i].BoundingSphere, meshes[i].SkinnedVertices);
				else
					DX12_MeshGenerator::FindBounding(submeshes[i].BoundingBox, submeshes[i].BoundingSphere, meshes[i].Vertices);
				if (i == 0)
				{
					submeshes[0].IndexCount = (UINT)meshes[0].Indices32.size();
					submeshes[0].StartIndexLocation = 0;
					submeshes[0].BaseVertexLocation = 0;
				}
				else
				{
					submeshes[i].IndexCount = (UINT)meshes[i].Indices32.size();
					submeshes[i].StartIndexLocation = submeshes[i - 1].StartIndexLocation + (UINT)meshes[i - 1].Indices32.size();
					submeshes[i].BaseVertexLocation = useSkinnedMesh
						? submeshes[i - 1].BaseVertexLocation + (UINT)meshes[i - 1].SkinnedVertices.size()
						: submeshes[i - 1].BaseVertexLocation + (UINT)meshes[i - 1].Vertices.size();
				}
				submeshes[i].InstanceCount = i;
				geo->DrawArgs[i] = submeshes[i];
			}
		}

		ECS::RepoHandle handle = mNextHandle++;
		mResourceStorage[handle] = { std::move(geo), 1 };
		mNameToHandle[name] = handle;
		return handle;
	}
protected:
	virtual bool UnloadResource(ECS::RepoHandle handle) override
	{
		std::lock_guard<std::mutex> lock(mtx);
		auto it = mResourceStorage.find(handle);
		if (it != mResourceStorage.end() && it->second.refCount == 1)
		{
			LOG_INFO("Mesh with handle {} released, refCount: {}", handle, it->second.refCount);
			it->second.resource->VertexBufferCPU.Reset();
			it->second.resource->IndexBufferCPU.Reset();
			it->second.resource->VertexBufferGPU.Reset();
			it->second.resource->IndexBufferGPU.Reset();
			it->second.resource->VertexBufferUploader.Reset();
			it->second.resource->IndexBufferUploader.Reset();
		}

		return true;
	}
};