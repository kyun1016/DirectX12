#pragma once
#include "DX12_MeshComponent.h"

class DX12_MeshGenerator
{
public:
	DX12_MeshGenerator() = delete;

	static MeshData CreateSprite(float width = 1.0f, float height = 1.0f)
	{
		MeshData meshData;
		SpriteVertex v;
		v.Position = { 0.0f, 0.0f, 0.0f }; // Center position, will be transformed by instance world matrix
		v.Size = { width, height };

		meshData.SpriteVertices.push_back(v);
		meshData.Indices32.push_back(0); // One index for the one point

		return meshData;
	}

	static MeshData CreateBox(float width = 1.0f, float height = 1.0f, float depth = 1.0f, std::uint32_t numSubdivisions = 3)
	{
		float max_w = 0.5f * width;
		float max_h = 0.5f * height;
		float max_d = 0.5f * depth;
		float min_w = -max_w;
		float min_h = -max_h;
		float min_d = -max_d;

		Vertex v[24];
		// front
		v[0].Position = { min_w, max_h, min_d }; v[0].Normal = { 0.0f, 0.0f, -1.0f }; v[0].TangentU = { 1.0f, 0.0f, 0.0f }; v[0].TexC = { 0.0f, 0.0f };
		v[1].Position = { max_w, max_h, min_d }; v[1].Normal = { 0.0f, 0.0f, -1.0f }; v[1].TangentU = { 1.0f, 0.0f, 0.0f }; v[1].TexC = { 1.0f, 0.0f };
		v[2].Position = { max_w, min_h, min_d }; v[2].Normal = { 0.0f, 0.0f, -1.0f }; v[2].TangentU = { 1.0f, 0.0f, 0.0f }; v[2].TexC = { 1.0f, 1.0f };
		v[3].Position = { min_w, min_h, min_d }; v[3].Normal = { 0.0f, 0.0f, -1.0f }; v[3].TangentU = { 1.0f, 0.0f, 0.0f }; v[3].TexC = { 0.0f, 1.0f };
		// back
		v[4].Position = { max_w, max_h, max_d }; v[4].Normal = { 0.0f, 0.0f, 1.0f }; v[4].TangentU = { -1.0f, 0.0f, 0.0f }; v[4].TexC = { 0.0f, 0.0f };
		v[5].Position = { min_w, max_h, max_d }; v[5].Normal = { 0.0f, 0.0f, 1.0f }; v[5].TangentU = { -1.0f, 0.0f, 0.0f }; v[5].TexC = { 1.0f, 0.0f };
		v[6].Position = { min_w, min_h, max_d }; v[6].Normal = { 0.0f, 0.0f, 1.0f }; v[6].TangentU = { -1.0f, 0.0f, 0.0f }; v[6].TexC = { 1.0f, 1.0f };
		v[7].Position = { max_w, min_h, max_d }; v[7].Normal = { 0.0f, 0.0f, 1.0f }; v[7].TangentU = { -1.0f, 0.0f, 0.0f }; v[7].TexC = { 0.0f, 1.0f };
		// top
		v[8].Position = { min_w, max_h, max_d }; v[8].Normal = { 0.0f, 1.0f, 0.0f }; v[8].TangentU = { 1.0f, 0.0f, 0.0f }; v[8].TexC = { 0.0f, 0.0f };
		v[9].Position = { max_w, max_h, max_d }; v[9].Normal = { 0.0f, 1.0f, 0.0f }; v[9].TangentU = { 1.0f, 0.0f, 0.0f }; v[9].TexC = { 1.0f, 0.0f };
		v[10].Position = { max_w, max_h, min_d }; v[10].Normal = { 0.0f, 1.0f, 0.0f }; v[10].TangentU = { 1.0f, 0.0f, 0.0f }; v[10].TexC = { 1.0f, 1.0f };
		v[11].Position = { min_w, max_h, min_d }; v[11].Normal = { 0.0f, 1.0f, 0.0f }; v[11].TangentU = { 1.0f, 0.0f, 0.0f }; v[11].TexC = { 0.0f, 1.0f };
		// bottom
		v[12].Position = { max_w, min_h, max_d }; v[12].Normal = { 0.0f, -1.0f, 0.0f }; v[12].TangentU = { -1.0f, 0.0f, 0.0f }; v[12].TexC = { 0.0f, 0.0f };
		v[13].Position = { min_w, min_h, max_d }; v[13].Normal = { 0.0f, -1.0f, 0.0f }; v[13].TangentU = { -1.0f, 0.0f, 0.0f }; v[13].TexC = { 1.0f, 0.0f };
		v[14].Position = { min_w, min_h, min_d }; v[14].Normal = { 0.0f, -1.0f, 0.0f }; v[14].TangentU = { -1.0f, 0.0f, 0.0f }; v[14].TexC = { 1.0f, 1.0f };
		v[15].Position = { max_w, min_h, min_d }; v[15].Normal = { 0.0f, -1.0f, 0.0f }; v[15].TangentU = { -1.0f, 0.0f, 0.0f }; v[15].TexC = { 0.0f, 1.0f };
		// left
		v[16].Position = { min_w, max_h, max_d }; v[16].Normal = { -1.0f, 0.0f, 0.0f }; v[16].TangentU = { 0.0f, 0.0f, -1.0f }; v[16].TexC = { 0.0f, 0.0f };
		v[17].Position = { min_w, max_h, min_d }; v[17].Normal = { -1.0f, 0.0f, 0.0f }; v[17].TangentU = { 0.0f, 0.0f, -1.0f }; v[17].TexC = { 1.0f, 0.0f };
		v[18].Position = { min_w, min_h, min_d }; v[18].Normal = { -1.0f, 0.0f, 0.0f }; v[18].TangentU = { 0.0f, 0.0f, -1.0f }; v[18].TexC = { 1.0f, 1.0f };
		v[19].Position = { min_w, min_h, max_d }; v[19].Normal = { -1.0f, 0.0f, 0.0f }; v[19].TangentU = { 0.0f, 0.0f, -1.0f }; v[19].TexC = { 0.0f, 1.0f };
		// right
		v[20].Position = { max_w, max_h, min_d }; v[20].Normal = { 1.0f, 0.0f, 0.0f }; v[20].TangentU = { 0.0f, 0.0f, 1.0f }; v[20].TexC = { 0.0f, 0.0f };
		v[21].Position = { max_w, max_h, max_d }; v[21].Normal = { 1.0f, 0.0f, 0.0f }; v[21].TangentU = { 0.0f, 0.0f, 1.0f }; v[21].TexC = { 1.0f, 0.0f };
		v[22].Position = { max_w, min_h, max_d }; v[22].Normal = { 1.0f, 0.0f, 0.0f }; v[22].TangentU = { 0.0f, 0.0f, 1.0f }; v[22].TexC = { 1.0f, 1.0f };
		v[23].Position = { max_w, min_h, min_d }; v[23].Normal = { 1.0f, 0.0f, 0.0f }; v[23].TangentU = { 0.0f, 0.0f, 1.0f }; v[23].TexC = { 0.0f, 1.0f };


		std::uint32_t i[36] =
		{
			0,1,2,
			0,2,3,

			4,5,6,
			4,6,7,

			8,9,10,
			8,10,11,

			12,13,14,
			12,14,15,

			16,17,18,
			16,18,19,

			20,21,22,
			20,22,23
		};

		MeshData meshData;
		meshData.Vertices.assign(&v[0], &v[24]);
		meshData.Indices32.assign(&i[0], &i[36]);

		// Put a cap on the number of subdivisions.
		numSubdivisions = std::min<std::uint32_t>(numSubdivisions, 6u);

		for (std::uint32_t i = 0; i < numSubdivisions; ++i)
			Subdivide(meshData);

		return meshData;
	}

	static MeshData CreateSquare(float scale)
	{
		MeshData meshData;
		Vertex v[4] = {
			{ { -scale, 0.0f, -scale }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
			{ { scale, 0.0f, -scale }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
			{ { scale, 0.0f, scale }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },
			{ { -scale, 0.0f, scale }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } }
		};
		std::uint32_t i[6] = {
			0,1,2,
			0,2,3
		};
		meshData.Vertices.assign(&v[0], &v[4]);
		meshData.Indices32.assign(&i[0], &i[6]);
		return meshData;
	}
	// static MeshData CreateSphere(float radius, std::uint32_t sliceCount, std::uint32_t stackCount);

	static void Subdivide(MeshData& meshData)
	{
		// Save a copy of the input geometry.
		MeshData inputCopy = meshData;


		meshData.Vertices.resize(0);
		meshData.Indices32.resize(0);

		//       v1
		//       *
		//      / \
		//     /   \
		//  m0*-----*m1
		//   / \   / \
		//  /   \ /   \
		// *-----*-----*
		// v0    m2     v2

		std::uint32_t numTris = (std::uint32_t)inputCopy.Indices32.size() / 3;
		for (std::uint32_t i = 0; i < numTris; ++i)
		{
			Vertex v0 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 0]];
			Vertex v1 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 1]];
			Vertex v2 = inputCopy.Vertices[inputCopy.Indices32[i * 3 + 2]];

			//
			// Generate the midpoints.
			//

			Vertex m0 = MidPoint(v0, v1);
			Vertex m1 = MidPoint(v1, v2);
			Vertex m2 = MidPoint(v0, v2);

			//
			// Add new geometry.
			//

			meshData.Vertices.push_back(v0); // 0
			meshData.Vertices.push_back(v1); // 1
			meshData.Vertices.push_back(v2); // 2
			meshData.Vertices.push_back(m0); // 3
			meshData.Vertices.push_back(m1); // 4
			meshData.Vertices.push_back(m2); // 5

			meshData.Indices32.push_back(i * 6 + 0);
			meshData.Indices32.push_back(i * 6 + 3);
			meshData.Indices32.push_back(i * 6 + 5);

			meshData.Indices32.push_back(i * 6 + 3);
			meshData.Indices32.push_back(i * 6 + 4);
			meshData.Indices32.push_back(i * 6 + 5);

			meshData.Indices32.push_back(i * 6 + 5);
			meshData.Indices32.push_back(i * 6 + 4);
			meshData.Indices32.push_back(i * 6 + 2);

			meshData.Indices32.push_back(i * 6 + 3);
			meshData.Indices32.push_back(i * 6 + 1);
			meshData.Indices32.push_back(i * 6 + 4);
		}
	}
	static Vertex MidPoint(const Vertex& v0, const Vertex& v1)
	{
		DirectX::XMVECTOR p0 = DirectX::XMLoadFloat3(&v0.Position);
		DirectX::XMVECTOR p1 = DirectX::XMLoadFloat3(&v1.Position);

		DirectX::XMVECTOR n0 = DirectX::XMLoadFloat3(&v0.Normal);
		DirectX::XMVECTOR n1 = DirectX::XMLoadFloat3(&v1.Normal);

		DirectX::XMVECTOR tan0 = DirectX::XMLoadFloat3(&v0.TangentU);
		DirectX::XMVECTOR tan1 = DirectX::XMLoadFloat3(&v1.TangentU);

		DirectX::XMVECTOR tex0 = DirectX::XMLoadFloat2(&v0.TexC);
		DirectX::XMVECTOR tex1 = DirectX::XMLoadFloat2(&v1.TexC);

		// Compute the midpoints of all the attributes.  Vectors need to be normalized
		// since linear interpolating can make them not unit length.  
		float3 pos = 0.5f * (v0.Position + v1.Position);
		float3 normal = DirectX::XMVector3Normalize(0.5f * (v0.Normal + v1.Normal));
		float3 tangent = DirectX::XMVector3Normalize(0.5f * (v0.TangentU + v1.TangentU));
		float2 tex = 0.5f * (v0.TexC + v1.TexC);

		Vertex v{
			.Position = pos,
			.Normal = normal,
			.TexC = tex,
			.TangentU = tangent
		};

		return v;
	}
	//static MeshData CreateGeosphere(float radius, std::uint32_t numSubdivisions);

	//static MeshData CreateCylinder(float bottomRadius, float topRadius, float height, std::uint32_t sliceCount, std::uint32_t stackCount);
	//static void BuildCylinderTopCap(float bottomRadius, float topRadius, float height, std::uint32_t sliceCount, MeshData& meshData);
	//static void BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, std::uint32_t sliceCount, MeshData& meshData);

	//static MeshData CreateGrid(float width, float depth, std::uint32_t m, std::uint32_t n);

	//static MeshData CreateQuad(float x, float y, float w, float h, float depth);

	//static MeshData CreateSquare(float scale);

	static void FindBounding(DirectX::BoundingBox& outBoundingBox, DirectX::BoundingSphere& outBoundingSphere, const std::vector<Vertex>& vertex)
	{
		float3 vMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
		float3 vMax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

		for (size_t i = 0; i < vertex.size(); ++i)
		{
			auto& p = vertex[i].Position;
			vMin.x = vMin.x < p.x ? vMin.x : p.x;
			vMin.y = vMin.y < p.y ? vMin.y : p.y;
			vMin.z = vMin.z < p.z ? vMin.z : p.z;
			vMax.x = vMax.x > p.x ? vMax.x : p.x;
			vMax.y = vMax.y > p.y ? vMax.y : p.y;
			vMax.z = vMax.z > p.z ? vMax.z : p.z;
		}
		DirectX::XMStoreFloat3(&outBoundingBox.Center, 0.5f * (vMin + vMax));
		DirectX::XMStoreFloat3(&outBoundingSphere.Center, 0.5f * (vMin + vMax));
		DirectX::XMStoreFloat3(&outBoundingBox.Extents, 0.5f * (vMax - vMin));

		outBoundingSphere.Radius = 0.0f;
		for (size_t i = 0; i < vertex.size(); ++i)
			outBoundingSphere.Radius = std::max(outBoundingSphere.Radius, (outBoundingSphere.Center - vertex[i].Position).Length());
		outBoundingSphere.Radius += 1e-2f;
	}

	static void FindBounding(DirectX::BoundingBox& outBoundingBox, DirectX::BoundingSphere& outBoundingSphere, const std::vector<SkinnedVertex>& vertex)
	{
		float3 vMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
		float3 vMax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

		for (size_t i = 0; i < vertex.size(); ++i)
		{
			auto& p = vertex[i].Position;
			vMin.x = vMin.x < p.x ? vMin.x : p.x;
			vMin.y = vMin.y < p.y ? vMin.y : p.y;
			vMin.z = vMin.z < p.z ? vMin.z : p.z;
			vMax.x = vMax.x > p.x ? vMax.x : p.x;
			vMax.y = vMax.y > p.y ? vMax.y : p.y;
			vMax.z = vMax.z > p.z ? vMax.z : p.z;
		}
		outBoundingBox.Center = (vMin + vMax) * 0.5f;
		outBoundingBox.Extents = (vMax - vMin) * 0.5f;
		outBoundingSphere.Center = outBoundingBox.Center;

		outBoundingSphere.Radius = 0.0f;
		for (size_t i = 0; i < vertex.size(); ++i)
			outBoundingSphere.Radius = std::max(outBoundingSphere.Radius, (outBoundingSphere.Center - vertex[i].Position).Length());
		outBoundingSphere.Radius += 1e-2f;
	}

		static void FindBounding(DirectX::BoundingBox& outBoundingBox, DirectX::BoundingSphere& outBoundingSphere, const std::vector<SpriteVertex>& vertex)
	{
		float3 vMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
		float3 vMax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

		for (size_t i = 0; i < vertex.size(); ++i)
		{
			auto& p = vertex[i].Position;
			vMin.x = vMin.x < p.x ? vMin.x : p.x;
			vMin.y = vMin.y < p.y ? vMin.y : p.y;
			vMin.z = vMin.z < p.z ? vMin.z : p.z;
			vMax.x = vMax.x > p.x ? vMax.x : p.x;
			vMax.y = vMax.y > p.y ? vMax.y : p.y;
			vMax.z = vMax.z > p.z ? vMax.z : p.z;
		}
		outBoundingBox.Center = (vMin + vMax) * 0.5f;
		outBoundingBox.Extents = (vMax - vMin) * 0.5f;
		outBoundingSphere.Center = outBoundingBox.Center;

		outBoundingSphere.Radius = 0.0f;
		for (size_t i = 0; i < vertex.size(); ++i)
			outBoundingSphere.Radius = std::max(outBoundingSphere.Radius, (outBoundingSphere.Center - vertex[i].Position).Length());
		outBoundingSphere.Radius += 1e-2f;
	}
};