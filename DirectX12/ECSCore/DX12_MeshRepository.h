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
	enum class eMeshType { STANDARD, SKINNED, SPRITE };

	ECS::RepoHandle LoadMesh(const std::string& name, std::vector<MeshData>& meshes, eMeshType meshType = eMeshType::STANDARD, bool useIndex32 = false) {
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
			std::vector<SpriteVertex> spriteVertices;
			std::vector<SkinnedVertex> skinnedVertices;
			std::vector<std::uint16_t> indices16;
			std::vector<std::uint32_t> indices32;

			for (size_t i = 0; i < meshes.size(); ++i)
			{
				indices16.insert(indices16.end(), meshes[i].GetIndices16().begin(), meshes[i].GetIndices16().end());
				indices32.insert(indices32.end(), meshes[i].Indices32.begin(), meshes[i].Indices32.end());
				vertices.insert(vertices.end(), meshes[i].Vertices.begin(), meshes[i].Vertices.end());
				spriteVertices.insert(spriteVertices.end(), meshes[i].SpriteVertices.begin(), meshes[i].SpriteVertices.end());
				skinnedVertices.insert(skinnedVertices.end(), meshes[i].SkinnedVertices.begin(), meshes[i].SkinnedVertices.end());
			}

			//=========================================================
			// Part 2-1. Vertex GPU 할당
			//=========================================================
			const void* vertexData = nullptr;
			switch (meshType)
			{
			case eMeshType::STANDARD:
				geo->VertexBufferByteSize = (UINT)vertices.size() * sizeof(Vertex);
				geo->VertexByteStride = sizeof(Vertex);
				geo->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				vertexData = vertices.data();
				break;
			case eMeshType::SKINNED:
				geo->VertexBufferByteSize = (UINT)skinnedVertices.size() * sizeof(SkinnedVertex);
				geo->VertexByteStride = sizeof(SkinnedVertex);
				geo->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				vertexData = skinnedVertices.data();
				break;
			case eMeshType::SPRITE:
				geo->VertexBufferByteSize = (UINT)spriteVertices.size() * sizeof(SpriteVertex);
				geo->VertexByteStride = sizeof(SpriteVertex);
				geo->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
				vertexData = spriteVertices.data();
				break;
			}
			geo->VertexBufferGPU = CreateDefaultBuffer(DX12_DeviceSystem::GetInstance().GetDevice(), DX12_CommandSystem::GetInstance().GetCommandList(),
			 vertexData, geo->VertexBufferByteSize, geo->VertexBufferUploader);
			ThrowIfFailed(D3DCreateBlob(geo->VertexBufferByteSize, &geo->VertexBufferCPU));
			CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertexData, geo->VertexBufferByteSize);

			//=========================================================
			// Part 2-2. Index GPU 할당 (16/32 bit)
			//=========================================================
			const void* indexData = nullptr;
			if(useIndex32)
			{
				geo->IndexBufferByteSize = (UINT)indices32.size() * sizeof(std::uint32_t);
				geo->IndexFormat = DXGI_FORMAT_R32_UINT;
				indexData = indices32.data();
			}
			else
			{
				geo->IndexBufferByteSize = (UINT)indices16.size() * sizeof(std::uint16_t);
				geo->IndexFormat =  DXGI_FORMAT_R16_UINT;
				indexData = indices16.data();
			}
			geo->IndexBufferGPU = CreateDefaultBuffer(DX12_DeviceSystem::GetInstance().GetDevice(), DX12_CommandSystem::GetInstance().GetCommandList(),
			 indexData, geo->IndexBufferByteSize, geo->IndexBufferUploader);
			ThrowIfFailed(D3DCreateBlob(geo->IndexBufferByteSize, &geo->IndexBufferCPU));
			CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indexData, geo->IndexBufferByteSize);

			//=========================================================
			// Part 3. SubmeshGeometry 생성
			//=========================================================
			geo->DrawArgs.resize(meshes.size());
			for (size_t i = 0; i < meshes.size(); ++i)
			{
				switch (meshType)
				{
				case eMeshType::STANDARD:
					DX12_MeshGenerator::FindBounding(geo->DrawArgs[i].BoundingBox, geo->DrawArgs[i].BoundingSphere, meshes[i].Vertices);
					break;
				case eMeshType::SKINNED:
					DX12_MeshGenerator::FindBounding(geo->DrawArgs[i].BoundingBox, geo->DrawArgs[i].BoundingSphere, meshes[i].SkinnedVertices);
					break;
				case eMeshType::SPRITE:
					DX12_MeshGenerator::FindBounding(geo->DrawArgs[i].BoundingBox, geo->DrawArgs[i].BoundingSphere, meshes[i].SpriteVertices);
					break;
				}
				if (i == 0)
				{
					geo->DrawArgs[0].IndexCount = (UINT)meshes[0].Indices32.size();
					geo->DrawArgs[0].StartIndexLocation = 0;
					geo->DrawArgs[0].BaseVertexLocation = 0;
				}
				else
				{
					geo->DrawArgs[i].IndexCount = (UINT)meshes[i].Indices32.size();
					geo->DrawArgs[i].StartIndexLocation = geo->DrawArgs[i - 1].StartIndexLocation + (UINT)meshes[i - 1].Indices32.size();
					switch (meshType)
					{
					case eMeshType::STANDARD:
						geo->DrawArgs[i].BaseVertexLocation = geo->DrawArgs[i - 1].BaseVertexLocation + (UINT)meshes[i - 1].Vertices.size();
						break;
					case eMeshType::SKINNED:
						geo->DrawArgs[i].BaseVertexLocation = geo->DrawArgs[i - 1].BaseVertexLocation + (UINT)meshes[i - 1].SkinnedVertices.size();
						break;
					case eMeshType::SPRITE:
						geo->DrawArgs[i].BaseVertexLocation = geo->DrawArgs[i - 1].BaseVertexLocation + (UINT)meshes[i - 1].SpriteVertices.size();
						break;
					}
				}
				geo->DrawArgs[i].InstanceCount = 1;
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