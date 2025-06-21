//#pragma once
//
//#include "../EngineCore/SimpleMath.h"
//#include "ECSRepository.h"
//#include "MeshComponent.h"
//#include <memory>
//#include <string>
//
////struct Vertex
////{
////	Vertex()
////		: Position(0.0f, 0.0f, 0.0f)
////		, Normal(0.0f, 0.0f, 0.0f)
////		, TangentU(0.0f, 0.0f, 0.0f)
////		, TexC(0.0f, 0.0f) {
////	}
////	Vertex(
////		const DirectX::SimpleMath::Vector3& p,
////		const DirectX::SimpleMath::Vector3& n,
////		const DirectX::SimpleMath::Vector3& t,
////		const DirectX::SimpleMath::Vector2& uv) :
////		Position(p),
////		Normal(n),
////		TangentU(t),
////		TexC(uv) {
////	}
////	Vertex(
////		float px, float py, float pz,
////		float nx, float ny, float nz,
////		float tx, float ty, float tz,
////		float u, float v) :
////		Position(px, py, pz),
////		Normal(nx, ny, nz),
////		TangentU(tx, ty, tz),
////		TexC(u, v) {
////	}
////
////	DirectX::SimpleMath::Vector3 Position;
////	DirectX::SimpleMath::Vector3 Normal;
////	DirectX::SimpleMath::Vector2 TexC;
////	DirectX::SimpleMath::Vector3 TangentU;
////};
////
////struct SkinnedVertex
////{
////	DirectX::SimpleMath::Vector3 Position;
////	DirectX::SimpleMath::Vector3 Normal;
////	DirectX::SimpleMath::Vector2 TexC;
////	DirectX::SimpleMath::Vector3 TangentU;
////	DirectX::SimpleMath::Vector3 BoneWeights;
////	uint8_t BoneIndices[4];
////};
//
//struct Mesh
//{
//	int id = 0; // 메쉬 ID
//	std::string path;
//	//union vertex {
//	//	std::vector<Vertex> Vertices;
//	//	std::vector<SkinnedVertex> SkinnedVertices;
//	//};
//
//	//union index {
//	//	std::vector<std::uint16_t> Indices16;
//	//	std::vector<std::uint32_t> Indices32;
//	//};
//
//	// TODO: GPUBufferHandle vertexBuffer;
//	// TODO: GPUBufferHandle indexBuffer;
//};
//
//class MeshRepository : public ECS::IRepository<Mesh> {
//public:
//	static MeshRepository& GetInstance() {
//		static MeshRepository instance;
//		return instance;
//	}
//protected:
//	virtual bool LoadResourceInternal(const std::string& path, Mesh* ptr)
//	{
//		ptr->id = mNextHandle; // 임시로 핸들을 ID로 사용
//		ptr->path = path;
//		// TODO: 파일에서 메쉬 데이터를 로드하는 로직 구현
//		// TODO: GPU 업로드 로직 구현
//		// 예시로 임의의 데이터를 채워넣음
//		//mesh->vertex.Vertices.push_back(Vertex());
//		//mesh->index.Indices32.push_back(0);
//
//		return true;
//	}
//	virtual bool UnloadResource(ECS::RepoHandle handle)
//	{
//		if (IRepository<Mesh>::UnloadResource(handle))
//		{
//			// TODO: GPU 리소스 해제 (예: DestroyBuffer(mesh->vertexBuffer))
//
//			return true;
//		}
//
//		return false;
//	}
//};