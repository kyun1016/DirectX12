//***************************************************************************************
// GeometryGenerator.h by Frank Luna (C) 2011 All Rights Reserved.
//   
// Defines a static class for procedurally generating the geometry of 
// common mathematical objects.
//
// All triangles are generated "outward" facing.  If you want "inward" 
// facing triangles (for example, if you want to place the camera inside
// a sphere to simulate a sky), you will need to:
//   1. Change the Direct3D cull mode or manually reverse the winding order.
//   2. Invert the normal.
//   3. Update the texture coordinates and tangent vectors.
//***************************************************************************************

#pragma once

#include <cstdint>
#include <DirectXMath.h>
#include <vector>

class GeometryGenerator
{
public:
	struct Vertex
	{
		Vertex()
			: Position(0.0f, 0.0f, 0.0f)
			, Normal(0.0f, 0.0f, 0.0f)
			, TangentU(0.0f, 0.0f, 0.0f)
			, TexC(0.0f, 0.0f) {}
		Vertex(
			const DirectX::XMFLOAT3& p,
			const DirectX::XMFLOAT3& n,
			const DirectX::XMFLOAT3& t,
			const DirectX::XMFLOAT2& uv) :
			Position(p),
			Normal(n),
			TangentU(t),
			TexC(uv) {}
		Vertex(
			float px, float py, float pz,
			float nx, float ny, float nz,
			float tx, float ty, float tz,
			float u, float v) :
			Position(px, py, pz),
			Normal(nx, ny, nz),
			TangentU(tx, ty, tz),
			TexC(u, v) {}

		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT2 TexC;
		DirectX::XMFLOAT3 TangentU;
	};

	struct SkinnedVertex
	{
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT2 TexC;
		DirectX::XMFLOAT3 TangentU;
		DirectX::XMFLOAT3 BoneWeights;
		BYTE BoneIndices[4];
	};

	struct MeshData
	{
		std::vector<Vertex> Vertices;
		std::vector<SkinnedVertex> SkinnedVertices;
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

public:
	GeometryGenerator() = delete;
	using uint16 = std::uint16_t;
	using uint32 = std::uint32_t;

	///<summary>
	/// Creates a box centered at the origin with the given dimensions, where each
	/// face has m rows and n columns of vertices.
	///</summary>
	static MeshData CreateBox(float width = 1.0f, float height = 1.0f, float depth = 1.0f, uint32 numSubdivisions = 3);

	///<summary>
	/// Creates a sphere centered at the origin with the given radius.  The
	/// slices and stacks parameters control the degree of tessellation.
	///</summary>
	static MeshData CreateSphere(float radius, uint32 sliceCount, uint32 stackCount);

	///<summary>
	/// Creates a geosphere centered at the origin with the given radius.  The
	/// depth controls the level of tessellation.
	///</summary>
	static void Subdivide(MeshData& meshData);
	static Vertex MidPoint(const Vertex& v0, const Vertex& v1);
	static MeshData CreateGeosphere(float radius, uint32 numSubdivisions);

	///<summary>
	/// Creates a cylinder parallel to the y-axis, and centered about the origin.  
	/// The bottom and top radius can vary to form various cone shapes rather than true
	/// cylinders.  The slices and stacks parameters control the degree of tessellation.
	///</summary>
	static MeshData CreateCylinder(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount);
	static void BuildCylinderTopCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, MeshData& meshData);
	static void BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32 sliceCount, MeshData& meshData);

	///<summary>
	/// Creates an mxn grid in the xz-plane with m rows and n columns, centered
	/// at the origin with the specified width and depth.
	///</summary>
	static MeshData CreateGrid(float width, float depth, uint32 m, uint32 n);

	///<summary>
	/// Creates a quad aligned with the screen.  This is useful for postprocessing and screen effects.
	///</summary>
	static MeshData CreateQuad(float x, float y, float w, float h, float depth);
	

	static void FindBounding(DirectX::BoundingBox& outBoundingBox, DirectX::BoundingSphere& outBoundingSphere, const std::vector<GeometryGenerator::Vertex>& vertex)
	{
		using namespace DirectX;
		DirectX::XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
		DirectX::XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

		DirectX::XMVECTOR vMin = XMLoadFloat3(&vMinf3);
		DirectX::XMVECTOR vMax = XMLoadFloat3(&vMaxf3);
		for (size_t i = 0; i < vertex.size(); ++i)
		{
			DirectX::XMVECTOR P = XMLoadFloat3(&vertex[i].Position);
			vMin = DirectX::XMVectorMin(vMin, P);
			vMax = DirectX::XMVectorMax(vMax, P);
		}
		DirectX::SimpleMath::Vector3 center = (vMin + vMax) * 0.5f;

		outBoundingBox.Center = center;
		DirectX::XMStoreFloat3(&outBoundingBox.Extents, 0.5f * (vMax - vMin));

		float maxRadius = 0.0f;
		for (size_t i = 0; i < vertex.size(); ++i)
		{
			maxRadius = max(maxRadius, (center - vertex[i].Position).Length());
		}
		maxRadius += 1e-2f;
		outBoundingSphere.Center = center;
		outBoundingSphere.Radius = maxRadius;
	}

	static void FindBounding(DirectX::BoundingBox& outBoundingBox, DirectX::BoundingSphere& outBoundingSphere, const std::vector<GeometryGenerator::SkinnedVertex>& vertex)
	{
		using namespace DirectX;
		DirectX::XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
		DirectX::XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

		DirectX::XMVECTOR vMin = XMLoadFloat3(&vMinf3);
		DirectX::XMVECTOR vMax = XMLoadFloat3(&vMaxf3);
		for (size_t i = 0; i < vertex.size(); ++i)
		{
			DirectX::XMVECTOR P = XMLoadFloat3(&vertex[i].Position);
			vMin = DirectX::XMVectorMin(vMin, P);
			vMax = DirectX::XMVectorMax(vMax, P);
		}
		DirectX::SimpleMath::Vector3 center = (vMin + vMax) * 0.5f;

		outBoundingBox.Center = center;
		DirectX::XMStoreFloat3(&outBoundingBox.Extents, 0.5f * (vMax - vMin));

		float maxRadius = 0.0f;
		for (size_t i = 0; i < vertex.size(); ++i)
		{
			maxRadius = max(maxRadius, (center - vertex[i].Position).Length());
		}
		maxRadius += 1e-2f;
		outBoundingSphere.Center = center;
		outBoundingSphere.Radius = maxRadius;
	}
};

