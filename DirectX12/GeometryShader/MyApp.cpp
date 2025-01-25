#include "pch.h"

#include "MyApp.h"

MyApp::MyApp()
	: MyApp(1080, 720, L"AppBase")
{
}

MyApp::MyApp(uint32_t width, uint32_t height, std::wstring name)
	: AppBase(width, height, name)
	, mImguiWidth(width)
	, mImguiHeight(height)
{
}
#pragma region Initialize
bool MyApp::Initialize()
{
	if (!AppBase::Initialize())
		return false;

	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

	mWaves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

	LoadTextures();
	BuildMaterials();

	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildShapeGeometry();
	BuildModelGeometry();
	BuildLandGeometry();
	BuildWavesGeometryBuffers();
	BuildRoomGeometry();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSO();

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	return true;
}

void MyApp::LoadTextures()
{
	
	for (const auto& a : TEXTURE_FILENAMES)
	{
		auto texture = std::make_unique<Texture>();
		texture->Filename = TEXTURE_DIR + a;
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(mDevice.Get(), mCommandList.Get(), texture->Filename.c_str(), texture->Resource, texture->UploadHeap));

		mTextures[a] = std::move(texture);
	}
}

void MyApp::BuildRootSignature()
{
	// Create root CBVs.
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // register t0

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0); // register b0
	slotRootParameter[2].InitAsConstantBufferView(1); // register b1
	slotRootParameter[3].InitAsConstantBufferView(2); // register b2

	auto staticSamplers = GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter, (UINT)staticSamplers.size(), staticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf()));
	if (errorBlob != nullptr)
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());

	ThrowIfFailed(mDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void MyApp::BuildDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc
	{
		/* D3D12_DESCRIPTOR_HEAP_TYPE Type	*/.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		/* UINT NumDescriptors				*/.NumDescriptors = (UINT) TEXTURE_FILENAMES.size() + 1,
		/* D3D12_DESCRIPTOR_HEAP_FLAGS Flags*/.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		/* UINT NodeMask					*/.NodeMask = 0
	};
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	//
	// Fill out the heap with actual descriptors.
	//
	
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc
	{
		/* DXGI_FORMAT Format															*/.Format = DXGI_FORMAT_UNKNOWN,
		/* D3D12_SRV_DIMENSION ViewDimension											*/.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
		/* UINT Shader4ComponentMapping													*/.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
		/* union {																		*/
		/* 	D3D12_BUFFER_SRV Buffer														*/
		/* 	D3D12_TEX1D_SRV Texture1D													*/
		/* 	D3D12_TEX1D_ARRAY_SRV Texture1DArray										*/
		/* 	D3D12_TEX2D_SRV Texture2D{													*/.Texture2D{
		/*		UINT MostDetailedMip													*/	.MostDetailedMip = 0,
		/*		UINT MipLevels															*/	.MipLevels = 0,
		/*		UINT PlaneSlice															*/	.PlaneSlice = 0,
		/*		FLOAT ResourceMinLODClamp												*/	.ResourceMinLODClamp = 0.0f,
		/*	}																			*/}
		/* 	D3D12_TEX2D_ARRAY_SRV Texture2DArray										*/
		/* 	D3D12_TEX2DMS_SRV Texture2DMS												*/
		/* 	D3D12_TEX2DMS_ARRAY_SRV Texture2DMSArray									*/
		/* 	D3D12_TEX3D_SRV Texture3D													*/
		/* 	D3D12_TEXCUBE_SRV TextureCube												*/
		/* 	D3D12_TEXCUBE_ARRAY_SRV TextureCubeArray									*/
		/* 	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_SRV RaytracingAccelerationStructure	*/
		/* }																			*/
	};
	

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	for (const auto& a : TEXTURE_FILENAMES)
	{
		auto texture = mTextures[a]->Resource;

		srvDesc.Format = texture->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;
		mDevice->CreateShaderResourceView(texture.Get(), &srvDesc, hDescriptor);
		hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
	}
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Texture2D.MipLevels = 1;
	mDevice->CreateShaderResourceView(mRTVTexBuffer.Get(), &srvDesc, hDescriptor);
	hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
}

void MyApp::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	for (size_t i = 0; i < VS_DIR.size(); ++i) mShaders[VS_NAME[i]] = D3DUtil::LoadBinary(VS_DIR[i]);
	for (size_t i = 0; i < GS_DIR.size(); ++i) mShaders[GS_NAME[i]] = D3DUtil::LoadBinary(GS_DIR[i]);
	for (size_t i = 0; i < PS_DIR.size(); ++i) mShaders[PS_NAME[i]] = D3DUtil::LoadBinary(PS_DIR[i]);

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void MyApp::BuildLandGeometry()
{
	GeometryGenerator::MeshData grid = GeometryGenerator::CreateGrid(160.0f, 160.0f, 50, 50);

	std::vector<Vertex> vertices(grid.Vertices.size());
	std::vector<std::uint16_t> indices = grid.GetIndices16();

	for (size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		auto& p = grid.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Pos.y = GetHillsHeight(p.x, p.z);
		vertices[i].Normal = GetHillsNormal(p.x, p.z);
		vertices[i].TexC = grid.Vertices[i].TexC;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = GEO_MESH_NAMES[2].first;
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
	geo->VertexBufferGPU = D3DUtil::CreateDefaultBuffer(mDevice.Get(), mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(mDevice.Get(), mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs[GEO_MESH_NAMES[2].second[0]] = submesh;
	mGeometries[geo->Name] = std::move(geo);
}


void MyApp::BuildWavesGeometryBuffers()
{
	std::vector<std::uint16_t> indices(3 * mWaves->TriangleCount()); // 3 indices per face
	assert(mWaves->VertexCount() < 0x0000ffff);

	// Iterate over each quad.
	int m = mWaves->RowCount();
	int n = mWaves->ColumnCount();
	int k = 0;
	for (int i = 0; i < m - 1; ++i)
	{
		for (int j = 0; j < n - 1; ++j)
		{
			indices[k] = i * n + j;
			indices[k + 1] = i * n + j + 1;
			indices[k + 2] = (i + 1) * n + j;

			indices[k + 3] = (i + 1) * n + j;
			indices[k + 4] = i * n + j + 1;
			indices[k + 5] = (i + 1) * n + j + 1;

			k += 6; // next quad
		}
	}
	const UINT vbByteSize = mWaves->VertexCount() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = GEO_MESH_NAMES[3].first;

	// Set dynamically.
	geo->VertexBufferCPU = nullptr;
	geo->VertexBufferGPU = nullptr;
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(mDevice.Get(), mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs[GEO_MESH_NAMES[3].second[0]] = submesh;

	mGeometries[geo->Name] = std::move(geo);
}

void MyApp::BuildShapeGeometry()
{
	//=========================================================
	// Part 1. Mesh 생성
	//=========================================================
	std::vector<GeometryGenerator::MeshData> meshes(GEO_MESH_NAMES[0].second.size());
	meshes[0] = GeometryGenerator::CreateBox(1.5f, 0.5f, 1.5f, 3);
	meshes[1] = GeometryGenerator::CreateGrid(20.0f, 30.0f, 60, 40);
	meshes[2] = GeometryGenerator::CreateSphere(0.5f, 20, 20);
	meshes[3] = GeometryGenerator::CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

	//=========================================================
	// Part 2. Sub Mesh 생성 및 통합 vertices & indices 생성
	//=========================================================
	size_t totalVertexCount = 0;
	for (const auto& a : meshes)
		totalVertexCount += a.Vertices.size();

	std::vector<SubmeshGeometry> submeshes(GEO_MESH_NAMES[0].second.size());
	std::vector<Vertex> vertices(totalVertexCount);
	std::vector<std::uint16_t> indices;
	UINT k = 0;
	for (size_t i = 0; i < GEO_MESH_NAMES[0].second.size(); ++i)
	{
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
			submeshes[i].BaseVertexLocation = submeshes[i - 1].BaseVertexLocation + (UINT)meshes[i - 1].Vertices.size();
		}
		indices.insert(indices.end(), meshes[i].GetIndices16().begin(), meshes[i].GetIndices16().end());
		for (const auto& ver: meshes[i].Vertices)
		{
			vertices[k].Pos = ver.Position;
			vertices[k].Normal = ver.Normal;
			vertices[k++].TexC = ver.TexC;
		}
	}

	//=========================================================
	// Part 3. Indices 할당 (16bit)
	//=========================================================
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = GEO_MESH_NAMES[0].first;
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
	geo->VertexBufferGPU = D3DUtil::CreateDefaultBuffer(mDevice.Get(), mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(mDevice.Get(), mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	for (size_t i = 0; i < GEO_MESH_NAMES[0].second.size(); ++i)
	{
		geo->DrawArgs[GEO_MESH_NAMES[0].second[i]] = submeshes[i];
	}
	mGeometries[geo->Name] = std::move(geo);
}

void MyApp::BuildModelGeometry()
{
	std::ifstream fin(MESH_MODEL_DIR + MESH_MODEL_FILE_NAMES[0]);

	if (!fin)
	{
		MessageBox(0, L"../Data/Models/skull.txt not found.", 0, 0);
		return;
	}

	UINT vcount = 0;
	UINT tcount = 0;
	std::string ignore;

	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;

	std::vector<Vertex> vertices(vcount);
	for (UINT i = 0; i < vcount; ++i)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
	}

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	std::vector<std::int32_t> indices(3 * tcount);
	for (UINT i = 0; i < tcount; ++i)
	{
		fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
	}

	fin.close();

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::int32_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = GEO_MESH_NAMES[1].first;

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = D3DUtil::CreateDefaultBuffer(mDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(mDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R32_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs[GEO_MESH_NAMES[1].second[0]] = submesh;

	mGeometries[geo->Name] = std::move(geo);
}

void MyApp::BuildRoomGeometry()
{
	// Create and specify geometry.  For this sample we draw a floor
// and a wall with a mirror on it.  We put the floor, wall, and
// mirror geometry in one vertex buffer.
//
//   |--------------|
//   |              |
//   |----|----|----|
//   |Wall|Mirr|Wall|
//   |    | or |    |
//   /--------------/
//  /   Floor      /
// /--------------/

	std::array<Vertex, 20> vertices =
	{
		// Floor: Observe we tile texture coordinates.
		Vertex(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f), // 0 
		Vertex(-3.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
		Vertex(7.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f),
		Vertex(7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 4.0f, 4.0f),

		// Wall: Observe we tile texture coordinates, and that we
		// leave a gap in the middle for the mirror.
		Vertex(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 4
		Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f),
		Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 2.0f),

		Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 8 
		Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f),
		Vertex(7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 2.0f),

		Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 12
		Vertex(-3.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f),
		Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 1.0f),

		// Mirror
		Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 16
		Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f),
		Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f)
	};

	std::array<std::int16_t, 30> indices =
	{
		// Floor
		0, 1, 2,
		0, 2, 3,

		// Walls
		4, 5, 6,
		4, 6, 7,

		8, 9, 10,
		8, 10, 11,

		12, 13, 14,
		12, 14, 15,

		// Mirror
		16, 17, 18,
		16, 18, 19
	};

	SubmeshGeometry floorSubmesh;
	floorSubmesh.IndexCount = 6;
	floorSubmesh.StartIndexLocation = 0;
	floorSubmesh.BaseVertexLocation = 0;

	SubmeshGeometry wallSubmesh;
	wallSubmesh.IndexCount = 18;
	wallSubmesh.StartIndexLocation = 6;
	wallSubmesh.BaseVertexLocation = 0;

	SubmeshGeometry mirrorSubmesh;
	mirrorSubmesh.IndexCount = 6;
	mirrorSubmesh.StartIndexLocation = 24;
	mirrorSubmesh.BaseVertexLocation = 0;

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = GEO_MESH_NAMES[4].first;

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = D3DUtil::CreateDefaultBuffer(mDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(mDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs[GEO_MESH_NAMES[4].second[0]] = floorSubmesh;
	geo->DrawArgs[GEO_MESH_NAMES[4].second[1]] = wallSubmesh;
	geo->DrawArgs[GEO_MESH_NAMES[4].second[2]] = mirrorSubmesh;

	mGeometries[geo->Name] = std::move(geo);
}

void MyApp::BuildMaterials()
{
	for (size_t i = 0; i < TEXTURE_FILENAMES.size(); ++i)
	{
		auto mat = std::make_unique<Material>();
		mat->Name = MATERIAL_NAMES[i];
		mat->MatCBIndex = i;
		mat->DiffuseSrvHeapIndex = i;
		mat->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mat->FresnelR0 = DirectX::XMFLOAT3(0.02f, 0.02f, 0.02f);
		mat->Roughness = 0.1f;
		mMaterials[mat->Name] = std::move(mat);
	}
	mMaterials["water1"]->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	mMaterials["ice"]->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.3f);

	UINT offset = TEXTURE_FILENAMES.size();
	auto skullMat = std::make_unique<Material>();
	skullMat->Name = MATERIAL_NAMES[offset];
	skullMat->MatCBIndex = offset;
	skullMat->DiffuseSrvHeapIndex = 0;
	skullMat->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	skullMat->FresnelR0 = DirectX::XMFLOAT3(0.05f, 0.05f, 0.05f);
	skullMat->Roughness = 0.3f;
	mMaterials[skullMat->Name] = std::move(skullMat);

	auto shadowMat = std::make_unique<Material>();
	shadowMat->Name = MATERIAL_NAMES[offset+1];
	shadowMat->MatCBIndex = offset + 1;
	shadowMat->DiffuseSrvHeapIndex = 0;
	shadowMat->DiffuseAlbedo = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
	shadowMat->FresnelR0 = DirectX::XMFLOAT3(0.001f, 0.001f, 0.001f);
	shadowMat->Roughness = 0.0f;
	mMaterials[shadowMat->Name] = std::move(shadowMat);
}


void MyApp::BuildRenderItems()
{
	UINT objCBIndex = 0;

	//=========================================================
	// GEO_MESH_NAMES[0]: ShapeGeo
	//=========================================================
	for (int i = 0; i < TEXTURE_FILENAMES.size(); ++i)
	{
		auto boxRitem = std::make_unique<RenderItem>();
		DirectX::XMStoreFloat4x4(&boxRitem->World, DirectX::XMMatrixScaling(2.0f, 2.0f, 2.0f) * DirectX::XMMatrixTranslation(i * 5.0f, 15.5f, 0.0f));
		DirectX::XMStoreFloat4x4(&boxRitem->TexTransform, DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f));
		boxRitem->ObjCBIndex = objCBIndex++;
		boxRitem->Mat = mMaterials[MATERIAL_NAMES[i]].get();
		boxRitem->Geo = mGeometries[GEO_MESH_NAMES[0].first].get();
		boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		boxRitem->IndexCount = boxRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[0]].IndexCount;
		boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[0]].StartIndexLocation;
		boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[0]].BaseVertexLocation;
		mAllRitems.push_back(std::move(boxRitem));
	}
	for (int i = 0; i < TEXTURE_FILENAMES.size(); ++i)
	{
		auto boxRitem = std::make_unique<RenderItem>();
		DirectX::XMStoreFloat4x4(&boxRitem->World, DirectX::XMMatrixScaling(2.0f, 2.0f, 2.0f) * DirectX::XMMatrixTranslation(i * 5.0f, 15.5f, 5.0f));
		DirectX::XMStoreFloat4x4(&boxRitem->TexTransform, DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f));
		boxRitem->ObjCBIndex = objCBIndex++;
		boxRitem->Mat = mMaterials[MATERIAL_NAMES[i]].get();
		boxRitem->Geo = mGeometries[GEO_MESH_NAMES[0].first].get();
		boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		boxRitem->IndexCount = boxRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[0]].IndexCount;
		boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[0]].StartIndexLocation;
		boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[0]].BaseVertexLocation;
		boxRitem->LayerFlag = (1 << (int)RenderLayer::Subdivision) | (1 << (int)RenderLayer::Normal);
		mAllRitems.push_back(std::move(boxRitem));
	}

	auto boxRitem1 = std::make_unique<RenderItem>();
	DirectX::XMStoreFloat4x4(&boxRitem1->World, DirectX::XMMatrixScaling(2.0f, 2.0f, 2.0f) * DirectX::XMMatrixTranslation(0.0f, 6.5f, 10.0f));
	DirectX::XMStoreFloat4x4(&boxRitem1->TexTransform, DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f));
	boxRitem1->ObjCBIndex = objCBIndex++;
	boxRitem1->Mat = mMaterials[MATERIAL_NAMES[5]].get();
	boxRitem1->Geo = mGeometries[GEO_MESH_NAMES[0].first].get();
	boxRitem1->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem1->IndexCount = boxRitem1->Geo->DrawArgs[GEO_MESH_NAMES[0].second[0]].IndexCount;
	boxRitem1->StartIndexLocation = boxRitem1->Geo->DrawArgs[GEO_MESH_NAMES[0].second[0]].StartIndexLocation;
	boxRitem1->BaseVertexLocation = boxRitem1->Geo->DrawArgs[GEO_MESH_NAMES[0].second[0]].BaseVertexLocation;
	boxRitem1->LayerFlag = (1 << (int)RenderLayer::AlphaTested);
	mAllRitems.push_back(std::move(boxRitem1));


	auto gridRitem = std::make_unique<RenderItem>();
	DirectX::XMStoreFloat4x4(&gridRitem->World, DirectX::XMMatrixTranslation(0.0f, 5.0f, 0.0f));
	DirectX::XMStoreFloat4x4(&gridRitem->TexTransform, DirectX::XMMatrixScaling(8.0f, 8.0f, 1.0f));
	gridRitem->ObjCBIndex = objCBIndex++;
	gridRitem->Mat = mMaterials[MATERIAL_NAMES[2]].get();
	gridRitem->Geo = mGeometries[GEO_MESH_NAMES[0].first].get();
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[1]].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[1]].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[1]].BaseVertexLocation;
	mAllRitems.push_back(std::move(gridRitem));

	DirectX::XMMATRIX brickTexTransform = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);
	for (int i = 0; i < 5; ++i)
	{
		auto leftCylRitem = std::make_unique<RenderItem>();
		auto rightCylRitem = std::make_unique<RenderItem>();
		auto leftSphereRitem = std::make_unique<RenderItem>();
		auto rightSphereRitem = std::make_unique<RenderItem>();

		DirectX::XMMATRIX leftCylWorld = DirectX::XMMatrixTranslation(-5.0f, 6.5f, -10.0f + i * 5.0f);
		DirectX::XMMATRIX rightCylWorld = DirectX::XMMatrixTranslation(+5.0f, 6.5f, -10.0f + i * 5.0f);
		DirectX::XMMATRIX leftSphereWorld = DirectX::XMMatrixTranslation(-5.0f, 8.5f, -10.0f + i * 5.0f);
		DirectX::XMMATRIX rightSphereWorld = DirectX::XMMatrixTranslation(+5.0f, 8.5f, -10.0f + i * 5.0f);

		DirectX::XMStoreFloat4x4(&leftCylRitem->World, leftCylWorld);
		DirectX::XMStoreFloat4x4(&leftCylRitem->TexTransform, brickTexTransform);
		leftCylRitem->ObjCBIndex = objCBIndex++;
		leftCylRitem->Mat = mMaterials[MATERIAL_NAMES[1]].get();
		leftCylRitem->Geo = mGeometries[GEO_MESH_NAMES[0].first].get();
		leftCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[3]].IndexCount;
		leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[3]].StartIndexLocation;
		leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[3]].BaseVertexLocation;

		DirectX::XMStoreFloat4x4(&rightCylRitem->World, rightCylWorld);
		DirectX::XMStoreFloat4x4(&rightCylRitem->TexTransform, brickTexTransform);
		rightCylRitem->ObjCBIndex = objCBIndex++;
		rightCylRitem->Mat = mMaterials[MATERIAL_NAMES[1]].get();
		rightCylRitem->Geo = mGeometries[GEO_MESH_NAMES[0].first].get();
		rightCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[3]].IndexCount;
		rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[3]].StartIndexLocation;
		rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[3]].BaseVertexLocation;

		DirectX::XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
		leftSphereRitem->TexTransform = MathHelper::Identity4x4();
		leftSphereRitem->ObjCBIndex = objCBIndex++;
		leftSphereRitem->Mat = mMaterials[MATERIAL_NAMES[1]].get();
		leftSphereRitem->Geo = mGeometries[GEO_MESH_NAMES[0].first].get();
		leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[2]].IndexCount;
		leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[2]].StartIndexLocation;
		leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[2]].BaseVertexLocation;

		DirectX::XMStoreFloat4x4(&rightSphereRitem->World, rightSphereWorld);
		rightSphereRitem->TexTransform = MathHelper::Identity4x4();
		rightSphereRitem->ObjCBIndex = objCBIndex++;
		rightSphereRitem->Mat = mMaterials[MATERIAL_NAMES[1]].get();
		rightSphereRitem->Geo = mGeometries[GEO_MESH_NAMES[0].first].get();
		rightSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRitem->IndexCount = rightSphereRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[2]].IndexCount;
		rightSphereRitem->StartIndexLocation = rightSphereRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[2]].StartIndexLocation;
		rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[2]].BaseVertexLocation;

		mAllRitems.push_back(std::move(leftCylRitem));
		mAllRitems.push_back(std::move(leftSphereRitem));
		mAllRitems.push_back(std::move(rightCylRitem));
		mAllRitems.push_back(std::move(rightSphereRitem));
	}
	//=========================================================
	// GEO_MESH_NAMES[1]:ModelGeo
	//=========================================================
	auto skullRitem = std::make_unique<RenderItem>();
	DirectX::XMStoreFloat4x4(&skullRitem->World, DirectX::XMMatrixTranslation(0.0f, 7.0f, 0.0f));
	DirectX::XMStoreFloat4x4(&skullRitem->TexTransform, DirectX::XMMatrixScaling(8.0f, 8.0f, 1.0f));
	skullRitem->ObjCBIndex = objCBIndex++;
	skullRitem->Mat = mMaterials[MATERIAL_NAMES[7]].get();
	skullRitem->Geo = mGeometries[GEO_MESH_NAMES[1].first].get();
	skullRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullRitem->IndexCount = skullRitem->Geo->DrawArgs[GEO_MESH_NAMES[1].second[0]].IndexCount;
	skullRitem->StartIndexLocation = skullRitem->Geo->DrawArgs[GEO_MESH_NAMES[1].second[0]].StartIndexLocation;
	skullRitem->BaseVertexLocation = skullRitem->Geo->DrawArgs[GEO_MESH_NAMES[1].second[0]].BaseVertexLocation;
	mAllRitems.push_back(std::move(skullRitem));

	//=========================================================
	// GEO_MESH_NAMES[2]:LandGeo
	//=========================================================
	auto landRitem = std::make_unique<RenderItem>();
	landRitem->World = MathHelper::Identity4x4();
	landRitem->ObjCBIndex = objCBIndex++;
	landRitem->Mat = mMaterials[MATERIAL_NAMES[3]].get();
	landRitem->Geo = mGeometries[GEO_MESH_NAMES[2].first].get();
	landRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	landRitem->IndexCount = landRitem->Geo->DrawArgs[GEO_MESH_NAMES[2].second[0]].IndexCount;
	landRitem->StartIndexLocation = landRitem->Geo->DrawArgs[GEO_MESH_NAMES[2].second[0]].StartIndexLocation;
	landRitem->BaseVertexLocation = landRitem->Geo->DrawArgs[GEO_MESH_NAMES[2].second[0]].BaseVertexLocation;
	mAllRitems.push_back(std::move(landRitem));

	//=========================================================
	// GEO_MESH_NAMES[3]:WaterGeo
	//=========================================================
	auto wavesRitem = std::make_unique<RenderItem>();
	wavesRitem->World = MathHelper::Identity4x4();
	wavesRitem->ObjCBIndex = objCBIndex++;
	wavesRitem->Mat = mMaterials[MATERIAL_NAMES[4]].get();
	wavesRitem->Geo = mGeometries[GEO_MESH_NAMES[3].first].get();
	wavesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wavesRitem->IndexCount = wavesRitem->Geo->DrawArgs[GEO_MESH_NAMES[3].second[0]].IndexCount;
	wavesRitem->StartIndexLocation = wavesRitem->Geo->DrawArgs[GEO_MESH_NAMES[3].second[0]].StartIndexLocation;
	wavesRitem->BaseVertexLocation = wavesRitem->Geo->DrawArgs[GEO_MESH_NAMES[3].second[0]].BaseVertexLocation;
	mWavesRitem = wavesRitem.get();
	wavesRitem->LayerFlag = (1 << (int)RenderLayer::Transparent);
	mAllRitems.push_back(std::move(wavesRitem));

	//=========================================================
	// GEO_MESH_NAMES[4]: RoomGeo
	//=========================================================
	auto mirrorRitem = std::make_unique<RenderItem>();
	DirectX::XMStoreFloat4x4(&mirrorRitem->World, DirectX::XMMatrixScaling(10.0f, 10.0f, 1.0f));
	mirrorRitem->TexTransform = MathHelper::Identity4x4();
	mirrorRitem->ObjCBIndex = objCBIndex++;
	mirrorRitem->Mat = mMaterials["ice"].get();
	mirrorRitem->Geo = mGeometries[GEO_MESH_NAMES[4].first].get();
	mirrorRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mirrorRitem->IndexCount = mirrorRitem->Geo->DrawArgs[GEO_MESH_NAMES[4].second[2]].IndexCount;
	mirrorRitem->StartIndexLocation = mirrorRitem->Geo->DrawArgs[GEO_MESH_NAMES[4].second[2]].StartIndexLocation;
	mirrorRitem->BaseVertexLocation = mirrorRitem->Geo->DrawArgs[GEO_MESH_NAMES[4].second[2]].BaseVertexLocation;
	mirrorRitem->LayerFlag
		= (1 << (int)RenderLayer::Mirror) 
		| (1 << (int)RenderLayer::Transparent);
	mAllRitems.push_back(std::move(mirrorRitem));
}

void MyApp::BuildFrameResources()
{
	for (int i = 0; i < APP_NUM_FRAME_RESOURCES; ++i)
		mFrameResources.push_back(std::make_unique<FrameResource>(mDevice.Get(), 2, (UINT)mAllRitems.size() * 3, (UINT)mMaterials.size(), mWaves->VertexCount()));
}

void MyApp::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc
	{
		/* ID3D12RootSignature* pRootSignature								*/.pRootSignature = mRootSignature.Get(),
		/* D3D12_SHADER_BYTECODE VS											*/.VS = {reinterpret_cast<BYTE*>(mShaders[VS_NAME[0]]->GetBufferPointer()), mShaders[VS_NAME[0]]->GetBufferSize()},
		/* D3D12_SHADER_BYTECODE PS											*/.PS = {reinterpret_cast<BYTE*>(mShaders[PS_NAME[0]]->GetBufferPointer()), mShaders[PS_NAME[0]]->GetBufferSize()},
		/* D3D12_SHADER_BYTECODE DS											*/.DS = {NULL, 0},
		/* D3D12_SHADER_BYTECODE HS											*/.HS = {NULL, 0},
		/* D3D12_SHADER_BYTECODE GS											*/.GS = {NULL, 0},
		/* D3D12_STREAM_OUTPUT_DESC StreamOutput{							*/.StreamOutput = {
		/*		const D3D12_SO_DECLARATION_ENTRY* pSODeclaration{			*/	NULL,
		/*			UINT Stream;											*/
		/*			LPCSTR SemanticName;									*/
		/*			UINT SemanticIndex;										*/
		/*			BYTE StartComponent;									*/
		/*			BYTE ComponentCount;									*/
		/*			BYTE OutputSlot;										*/
		/*		}															*/
		/*		UINT NumEntries;											*/	0,
		/*		const UINT* pBufferStrides;									*/	0,
		/*		UINT NumStrides;											*/	0,
		/*		UINT RasterizedStream;										*/	0
		/* }																*/},
		/* D3D12_BLEND_DESC BlendState{										*/.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
		/*		BOOL AlphaToCoverageEnable									*/
		/*		BOOL IndependentBlendEnable									*/
		/*		D3D12_RENDER_TARGET_BLEND_DESC RenderTarget[8]				*/
		/* }																*/
		/* UINT SampleMask													*/.SampleMask = UINT_MAX,
		/* D3D12_RASTERIZER_DESC RasterizerState{							*/.RasterizerState = { // CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		/*		D3D12_FILL_MODE FillMode									*/		D3D12_FILL_MODE_SOLID, // D3D12_FILL_MODE_WIREFRAME,
		/*		D3D12_CULL_MODE CullMode									*/		D3D12_CULL_MODE_BACK,
		/*		BOOL FrontCounterClockwise									*/		false,
		/*		INT DepthBias												*/		D3D12_DEFAULT_DEPTH_BIAS,
		/*		FLOAT DepthBiasClamp										*/		D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
		/*		FLOAT SlopeScaledDepthBias									*/		D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
		/*		BOOL DepthClipEnable										*/		true,
		/*		BOOL MultisampleEnable										*/		false,
		/*		BOOL AntialiasedLineEnable									*/		false,
		/*		UINT ForcedSampleCount										*/		0,
		/*		D3D12_CONSERVATIVE_RASTERIZATION_MODE ConservativeRaster	*/		D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
		/* }																*/},
		/* D3D12_DEPTH_STENCIL_DESC DepthStencilState {						*/.DepthStencilState = { // CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		/*		BOOL DepthEnable											*/		.DepthEnable = true,
		/*		D3D12_DEPTH_WRITE_MASK DepthWriteMask						*/		.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
		/*		D3D12_COMPARISON_FUNC DepthFunc								*/		.DepthFunc = D3D12_COMPARISON_FUNC_LESS,
		/*		BOOL StencilEnable											*/		.StencilEnable = false,
		/*		UINT8 StencilReadMask										*/		.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
		/*		UINT8 StencilWriteMask										*/		.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
		/*		D3D12_DEPTH_STENCILOP_DESC FrontFace {						*/		.FrontFace = {
		/*			D3D12_STENCIL_OP StencilFailOp							*/			.StencilFailOp = D3D12_STENCIL_OP_KEEP,
		/*			D3D12_STENCIL_OP StencilDepthFailOp						*/			.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
		/*			D3D12_STENCIL_OP StencilPassOp							*/			.StencilPassOp = D3D12_STENCIL_OP_KEEP,
		/*			D3D12_COMPARISON_FUNC StencilFunc						*/			.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
		/*		}															*/		},
		/*		D3D12_DEPTH_STENCILOP_DESC BackFace							*/		.BackFace = {
		/*			D3D12_STENCIL_OP StencilFailOp							*/			.StencilFailOp = D3D12_STENCIL_OP_KEEP,
		/*			D3D12_STENCIL_OP StencilDepthFailOp						*/			.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
		/*			D3D12_STENCIL_OP StencilPassOp							*/			.StencilPassOp = D3D12_STENCIL_OP_KEEP,
		/*			D3D12_COMPARISON_FUNC StencilFunc						*/			.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
		/*		}															*/		},
		/* }																*/ },
		/* D3D12_INPUT_LAYOUT_DESC InputLayout{								*/.InputLayout = {
		/*		const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs			*/		.pInputElementDescs = mInputLayout.data(),
		/*		UINT NumElements											*/		.NumElements = (UINT)mInputLayout.size()
		/*	}																*/ },
		/* D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue				*/.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
		/* D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType				*/.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		/* UINT NumRenderTargets											*/.NumRenderTargets = 1,
		/* DXGI_FORMAT RTVFormats[8]										*/.RTVFormats = {mBackBufferFormat, DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN},	// 0
		/* DXGI_FORMAT DSVFormat											*/.DSVFormat = mDepthStencilFormat,
		/* DXGI_SAMPLE_DESC SampleDesc{										*/.SampleDesc = {
		/*		UINT Count;													*/		.Count = m4xMsaaState ? 4u : 1u,
		/*		UINT Quality;												*/		.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0
		/*	}																*/},
		/* UINT NodeMask													*/.NodeMask = 0,
		/* D3D12_CACHED_PIPELINE_STATE CachedPSO							*/.CachedPSO = {NULL, 0},
		/* D3D12_PIPELINE_STATE_FLAGS Flags									*/.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
	};

	//=====================================================
	// PSO for marking stencil mirrors.
	//=====================================================
	D3D12_GRAPHICS_PIPELINE_STATE_DESC markMirrorsPsoDesc = opaquePsoDesc;
	markMirrorsPsoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = 0;
	markMirrorsPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	markMirrorsPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	markMirrorsPsoDesc.DepthStencilState.StencilEnable = true;
	markMirrorsPsoDesc.DepthStencilState.StencilReadMask = 0xff;
	markMirrorsPsoDesc.DepthStencilState.StencilWriteMask = 0xff;

	markMirrorsPsoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	markMirrorsPsoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	markMirrorsPsoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	markMirrorsPsoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	markMirrorsPsoDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	markMirrorsPsoDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	markMirrorsPsoDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	markMirrorsPsoDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;


	//=====================================================
	// PSO for stencil reflections.
	//=====================================================
	D3D12_GRAPHICS_PIPELINE_STATE_DESC drawReflectionsPsoDesc = opaquePsoDesc;
	drawReflectionsPsoDesc.DepthStencilState.DepthEnable = true;
	drawReflectionsPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	drawReflectionsPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	drawReflectionsPsoDesc.DepthStencilState.StencilEnable = true;
	drawReflectionsPsoDesc.DepthStencilState.StencilReadMask = 0xff;
	drawReflectionsPsoDesc.DepthStencilState.StencilWriteMask = 0xff;

	drawReflectionsPsoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	drawReflectionsPsoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	drawReflectionsPsoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	drawReflectionsPsoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

	// We are not rendering backfacing polygons, so these settings do not matter.
	drawReflectionsPsoDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	drawReflectionsPsoDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	drawReflectionsPsoDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	drawReflectionsPsoDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

	drawReflectionsPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	drawReflectionsPsoDesc.RasterizerState.FrontCounterClockwise = true;

	//=====================================================
	// PSO for transparent objects
	//=====================================================
	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;

	transparentPsoDesc.BlendState.RenderTarget[0] =
	{
		/* BOOL BlendEnable				*/true,
		/* BOOL LogicOpEnable			*/false,
		/* D3D12_BLEND SrcBlend			*/D3D12_BLEND_SRC_ALPHA,
		/* D3D12_BLEND DestBlend		*/D3D12_BLEND_INV_SRC_ALPHA,
		/* D3D12_BLEND_OP BlendOp		*/D3D12_BLEND_OP_ADD,
		/* D3D12_BLEND SrcBlendAlpha	*/D3D12_BLEND_ONE,
		/* D3D12_BLEND DestBlendAlpha	*/D3D12_BLEND_ZERO,
		/* D3D12_BLEND_OP BlendOpAlpha	*/D3D12_BLEND_OP_ADD,
		/* D3D12_LOGIC_OP LogicOp		*/D3D12_LOGIC_OP_NOOP,
		/* UINT8 RenderTargetWriteMask	*/D3D12_COLOR_WRITE_ENABLE_ALL
	};
	transparentPsoDesc.BlendState.RenderTarget[1] =
	{
		/* BOOL BlendEnable				*/true,
		/* BOOL LogicOpEnable			*/false,
		/* D3D12_BLEND SrcBlend			*/D3D12_BLEND_SRC_ALPHA,
		/* D3D12_BLEND DestBlend		*/D3D12_BLEND_INV_SRC_ALPHA,
		/* D3D12_BLEND_OP BlendOp		*/D3D12_BLEND_OP_ADD,
		/* D3D12_BLEND SrcBlendAlpha	*/D3D12_BLEND_ONE,
		/* D3D12_BLEND DestBlendAlpha	*/D3D12_BLEND_ZERO,
		/* D3D12_BLEND_OP BlendOpAlpha	*/D3D12_BLEND_OP_ADD,
		/* D3D12_LOGIC_OP LogicOp		*/D3D12_LOGIC_OP_NOOP,
		/* UINT8 RenderTargetWriteMask	*/D3D12_COLOR_WRITE_ENABLE_ALL
	};
	//=====================================================
	// PSO for alpha tested objects
	//=====================================================
	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
	alphaTestedPsoDesc.PS = {reinterpret_cast<BYTE*>(mShaders[PS_NAME[1]]->GetBufferPointer()),	mShaders[PS_NAME[1]]->GetBufferSize()};
	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	
	//=====================================================
	// PSO for shadow objects
	//=====================================================
	D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowPsoDesc = transparentPsoDesc;

	shadowPsoDesc.DepthStencilState.DepthEnable = true;
	shadowPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	shadowPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	shadowPsoDesc.DepthStencilState.StencilEnable = true;
	shadowPsoDesc.DepthStencilState.StencilReadMask = 0xff;
	shadowPsoDesc.DepthStencilState.StencilWriteMask = 0xff;

	shadowPsoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	shadowPsoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	shadowPsoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
	shadowPsoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

	// We are not rendering backfacing polygons, so these settings do not matter.
	shadowPsoDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	shadowPsoDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	shadowPsoDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
	shadowPsoDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

	//=====================================================
	// PSO for Subdivision
	//=====================================================
	D3D12_GRAPHICS_PIPELINE_STATE_DESC subdivisionDesc = opaquePsoDesc;
	subdivisionDesc.VS = { reinterpret_cast<BYTE*>(mShaders[VS_NAME[1]]->GetBufferPointer()), mShaders[VS_NAME[1]]->GetBufferSize() };
	subdivisionDesc.GS = { reinterpret_cast<BYTE*>(mShaders[GS_NAME[0]]->GetBufferPointer()), mShaders[GS_NAME[0]]->GetBufferSize() };

	//=====================================================
	// PSO for Normal
	//=====================================================
	D3D12_GRAPHICS_PIPELINE_STATE_DESC normalDesc = opaquePsoDesc;
	normalDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	normalDesc.VS = { reinterpret_cast<BYTE*>(mShaders[VS_NAME[2]]->GetBufferPointer()), mShaders[VS_NAME[2]]->GetBufferSize() };
	normalDesc.GS = { reinterpret_cast<BYTE*>(mShaders[GS_NAME[1]]->GetBufferPointer()), mShaders[GS_NAME[1]]->GetBufferSize() };
	normalDesc.PS = { reinterpret_cast<BYTE*>(mShaders[PS_NAME[2]]->GetBufferPointer()), mShaders[PS_NAME[2]]->GetBufferSize() };

	//=====================================================
	// Create PSO
	//=====================================================
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs[gPSOName[(int)RenderLayer::Opaque]])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&markMirrorsPsoDesc, IID_PPV_ARGS(&mPSOs[gPSOName[(int)RenderLayer::Mirror]])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&drawReflectionsPsoDesc, IID_PPV_ARGS(&mPSOs[gPSOName[(int)RenderLayer::Reflected]])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs[gPSOName[(int)RenderLayer::Transparent]])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs[gPSOName[(int)RenderLayer::AlphaTested]])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(&mPSOs[gPSOName[(int)RenderLayer::Shadow]])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&subdivisionDesc, IID_PPV_ARGS(&mPSOs[gPSOName[(int)RenderLayer::Subdivision]])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&normalDesc, IID_PPV_ARGS(&mPSOs[gPSOName[(int)RenderLayer::Normal]])));

	//=====================================================
	// PSO for wireframe objects.
	//=====================================================
	opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	markMirrorsPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	drawReflectionsPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	transparentPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	alphaTestedPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	shadowPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	subdivisionDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	normalDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs[gPSOName[(int)RenderLayer::Count + (int)RenderLayer::Opaque]])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&markMirrorsPsoDesc, IID_PPV_ARGS(&mPSOs[gPSOName[(int)RenderLayer::Count + (int)RenderLayer::Mirror]])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&drawReflectionsPsoDesc, IID_PPV_ARGS(&mPSOs[gPSOName[(int)RenderLayer::Count + (int)RenderLayer::Reflected]])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs[gPSOName[(int)RenderLayer::Count + (int)RenderLayer::Transparent]])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs[gPSOName[(int)RenderLayer::Count + (int)RenderLayer::AlphaTested]])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(&mPSOs[gPSOName[(int)RenderLayer::Count + (int)RenderLayer::Shadow]])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&subdivisionDesc, IID_PPV_ARGS(&mPSOs[gPSOName[(int)RenderLayer::Count + (int)RenderLayer::Subdivision]])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&normalDesc, IID_PPV_ARGS(&mPSOs[gPSOName[(int)RenderLayer::Count + (int)RenderLayer::Normal]])));
}
std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> MyApp::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}
#pragma endregion Initialize

#pragma region Update
void MyApp::OnResize()
{
	AppBase::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, mAspectRatio, 1.0f, 1000.0f);
	DirectX::XMStoreFloat4x4(&mProj, P);

	//CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	//hDescriptor.Offset(TEXTURE_FILENAMES.size(), mCbvSrvUavDescriptorSize);
	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc
	//{
	//	/* DXGI_FORMAT Format															*/.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
	//	/* D3D12_SRV_DIMENSION ViewDimension											*/.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
	//	/* UINT Shader4ComponentMapping													*/.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
	//	/* union {																		*/
	//	/* 	D3D12_BUFFER_SRV Buffer														*/
	//	/* 	D3D12_TEX1D_SRV Texture1D													*/
	//	/* 	D3D12_TEX1D_ARRAY_SRV Texture1DArray										*/
	//	/* 	D3D12_TEX2D_SRV Texture2D{													*/.Texture2D{
	//	/*		UINT MostDetailedMip													*/	.MostDetailedMip = 0,
	//	/*		UINT MipLevels															*/	.MipLevels = 1,
	//	/*		UINT PlaneSlice															*/	.PlaneSlice = 0,
	//	/*		FLOAT ResourceMinLODClamp												*/	.ResourceMinLODClamp = 0.0f,
	//	/*	}																			*/}
	//	/* 	D3D12_TEX2D_ARRAY_SRV Texture2DArray										*/
	//	/* 	D3D12_TEX2DMS_SRV Texture2DMS												*/
	//	/* 	D3D12_TEX2DMS_ARRAY_SRV Texture2DMSArray									*/
	//	/* 	D3D12_TEX3D_SRV Texture3D													*/
	//	/* 	D3D12_TEXCUBE_SRV TextureCube												*/
	//	/* 	D3D12_TEXCUBE_ARRAY_SRV TextureCubeArray									*/
	//	/* 	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_SRV RaytracingAccelerationStructure	*/
	//	/* }																			*/
	//};
	//mDevice->CreateShaderResourceView(mRTVTexBuffer.Get(), &srvDesc, mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	//BuildRootSignature();
	//BuildDescriptorHeaps();
	//BuildFrameResources();
}
void MyApp::Update()
{
	OnKeyboardInput();
	UpdateCamera();

	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % APP_NUM_FRAME_RESOURCES;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, mFenceEvent));
		WaitForSingleObject(mFenceEvent, INFINITE);
	}

	AnimateMaterials();
	UpdateObjectCBs();
	UpdateMaterialCBs();
	UpdateMainPassCB();
	UpdateWaves();
	UpdateReflectedPassCB();
}

void MyApp::Render()
{
	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(mCurrFrameResource->CmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	int offset = mIsWireframe ? (int)RenderLayer::Count : 0;
	ThrowIfFailed(mCommandList->Reset(mCurrFrameResource->CmdListAlloc.Get(), mPSOs[gPSOName[offset + (int)RenderLayer::Opaque]].Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	D3D12_RESOURCE_BARRIER RenderBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mSwapChainBuffer[mCurrBackBuffer].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	mCommandList->ResourceBarrier(1, &RenderBarrier);
	D3D12_RESOURCE_BARRIER RTVTexBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mRTVTexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
	mCommandList->ResourceBarrier(1, &RTVTexBarrier);

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(mSwapChainDescriptor[mCurrBackBuffer], (float*)&mMainPassCB.FogColor, 0, nullptr);
	mCommandList->ClearDepthStencilView(mDepthStencilDescriptor, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	D3D12_CPU_DESCRIPTOR_HANDLE rtvs[2];
	rtvs[0] = mSwapChainDescriptor[mCurrBackBuffer];		// 기존
	rtvs[1] = mSwapChainDescriptor[APP_NUM_BACK_BUFFERS];   // 새로 만든 RTV
	mCommandList->OMSetRenderTargets(2, rtvs, false, &mDepthStencilDescriptor);
	// mCommandList->OMSetRenderTargets(1, &mSwapChainDescriptor[mCurrBackBuffer], true, &mDepthStencilDescriptor);

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	// Bind per-pass constant buffer.  We only need to do this once per-pass.
	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());
	DrawRenderItems(RenderLayer::Opaque);
	mCommandList->SetPipelineState(mPSOs[gPSOName[offset + (int)RenderLayer::Subdivision]].Get());
	DrawRenderItems(RenderLayer::Subdivision);

	mCommandList->OMSetStencilRef(1);
	mCommandList->SetPipelineState(mPSOs[gPSOName[offset + (int)RenderLayer::Mirror]].Get());
	DrawRenderItems(RenderLayer::Mirror);

	UINT passCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress() + 1 * passCBByteSize);
	mCommandList->SetPipelineState(mPSOs[gPSOName[offset + (int)RenderLayer::Reflected]].Get());
	DrawRenderItems(RenderLayer::Reflected);
	// Restore main pass constants and stencil ref.
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());
	mCommandList->OMSetStencilRef(0);

	mCommandList->SetPipelineState(mPSOs[gPSOName[offset + (int)RenderLayer::AlphaTested]].Get());
	DrawRenderItems(RenderLayer::AlphaTested);

	mCommandList->SetPipelineState(mPSOs[gPSOName[offset + (int)RenderLayer::Transparent]].Get());
	DrawRenderItems(RenderLayer::Transparent);

	mCommandList->SetPipelineState(mPSOs[gPSOName[offset + (int)RenderLayer::Shadow]].Get());
	DrawRenderItems(RenderLayer::Shadow);

	mCommandList->SetPipelineState(mPSOs[gPSOName[offset + (int)RenderLayer::Normal]].Get());
	DrawRenderItems(RenderLayer::Normal);

	// Indicate a state transition on the resource usage.
	D3D12_RESOURCE_BARRIER DefaultBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mSwapChainBuffer[mCurrBackBuffer].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	mCommandList->ResourceBarrier(1, &DefaultBarrier);
	RTVTexBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mRTVTexBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
	mCommandList->ResourceBarrier(1, &RTVTexBarrier);
}

void MyApp::AnimateMaterials()
{
	// Scroll the water material texture coordinates.
	auto waterMat = mMaterials[MATERIAL_NAMES[4]].get();

	float& tu = waterMat->MatTransform(3, 0);
	float& tv = waterMat->MatTransform(3, 1);

	tu += 0.1f * mTimer.DeltaTime();
	tv += 0.02f * mTimer.DeltaTime();

	if (tu >= 1.0f)
		tu -= 1.0f;

	if (tv >= 1.0f)
		tv -= 1.0f;

	waterMat->MatTransform(3, 0) = tu;
	waterMat->MatTransform(3, 1) = tv;

	// Material has changed, so need to update cbuffer.
	waterMat->NumFramesDirty = APP_NUM_FRAME_RESOURCES;
}

void MyApp::UpdateObjectCBs()
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : mAllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		if (e->NumFramesDirty > 0)
		{
			DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&e->World);
			DirectX::XMMATRIX texTransform = DirectX::XMLoadFloat4x4(&e->TexTransform);

			ObjectConstants objConstants;
			DirectX::XMStoreFloat4x4(&objConstants.World, DirectX::XMMatrixTranspose(world));
			DirectX::XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
			
			DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(world);
			DirectX::XMStoreFloat4x4(&objConstants.WorldInvTranspose, DirectX::XMMatrixInverse(&det, world));

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);
		}
	}

	for (auto& e : mAllRitems)
	{
		if (e->NumFramesDirty > 0)
		{
			//DirectX::XMMATRIX rotate = DirectX::XMMatrixRotationY(e->AngleY * MathHelper::Pi);
			//DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(e->ScaleX, e->ScaleY, e->ScaleZ);
			//DirectX::XMMATRIX offset = DirectX::XMMatrixTranslation(e->OffsetX, e->OffsetY, e->OffsetZ);
			//DirectX::XMMATRIX world = rotate * scale * offset;

			// Update reflection world matrix.
			if (e->LayerFlag & (1 << (int)RenderLayer::Reflected))
			{
				DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&e->World);
				DirectX::XMMATRIX texTransform = DirectX::XMLoadFloat4x4(&e->TexTransform);

				DirectX::XMVECTOR mirrorPlane = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // xy plane
				DirectX::XMMATRIX R = DirectX::XMMatrixReflect(mirrorPlane);
				ObjectConstants objConstants;
				DirectX::XMStoreFloat4x4(&objConstants.World, DirectX::XMMatrixTranspose(world * R));
				DirectX::XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
				currObjectCB->CopyData(mAllRitems.size() + e->ObjCBIndex, objConstants);
			}
		}
	}

	for (auto& e : mAllRitems)
	{
		if (e->NumFramesDirty > 0)
		{
			e->NumFramesDirty--;

			// Update reflection world matrix.
			if (e->LayerFlag & (1 << (int)RenderLayer::Shadow))
			{
				using namespace DirectX;
				DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&e->World);
				DirectX::XMMATRIX texTransform = DirectX::XMLoadFloat4x4(&e->TexTransform);

				DirectX::XMVECTOR shadowPlane = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // xz plane
				DirectX::XMVECTOR toMainLight = -DirectX::XMLoadFloat3(&mMainPassCB.Lights[0].Direction);
				DirectX::XMMATRIX S = DirectX::XMMatrixShadow(shadowPlane, toMainLight);
				DirectX::XMMATRIX shadowOffsetY = DirectX::XMMatrixTranslation(0.0f, 0.001f, 0.0f);

				ObjectConstants objConstants;
				DirectX::XMStoreFloat4x4(&objConstants.World, DirectX::XMMatrixTranspose(world * S * shadowOffsetY));
				DirectX::XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
				currObjectCB->CopyData(mAllRitems.size() * 2 + e->ObjCBIndex, objConstants);
			}
		}
	}
}

void MyApp::UpdateMaterialCBs()
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for (auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			DirectX::XMMATRIX matTransform = DirectX::XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			DirectX::XMStoreFloat4x4(&matConstants.MatTransform, DirectX::XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void MyApp::UpdateMainPassCB()
{
	DirectX::XMMATRIX view = DirectX::XMLoadFloat4x4(&mView);
	DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&mProj);
	DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply(view, proj);

	DirectX::XMVECTOR viewDeterminant = DirectX::XMMatrixDeterminant(view);
	DirectX::XMVECTOR projDeterminant = DirectX::XMMatrixDeterminant(proj);
	DirectX::XMVECTOR viewProjDeterminant = DirectX::XMMatrixDeterminant(viewProj);

	DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(&viewDeterminant, view);
	DirectX::XMMATRIX invProj = DirectX::XMMatrixInverse(&projDeterminant, proj);
	DirectX::XMMATRIX invViewProj = DirectX::XMMatrixInverse(&viewProjDeterminant, viewProj);

	DirectX::XMStoreFloat4x4(&mMainPassCB.View, DirectX::XMMatrixTranspose(view));
	DirectX::XMStoreFloat4x4(&mMainPassCB.InvView, DirectX::XMMatrixTranspose(invView));
	DirectX::XMStoreFloat4x4(&mMainPassCB.Proj, DirectX::XMMatrixTranspose(proj));
	DirectX::XMStoreFloat4x4(&mMainPassCB.InvProj, DirectX::XMMatrixTranspose(invProj));
	DirectX::XMStoreFloat4x4(&mMainPassCB.ViewProj, DirectX::XMMatrixTranspose(viewProj));
	DirectX::XMStoreFloat4x4(&mMainPassCB.InvViewProj, DirectX::XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = DirectX::XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = DirectX::XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = mTimer.TotalTime();
	mMainPassCB.DeltaTime = mTimer.DeltaTime();
	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}
void MyApp::UpdateReflectedPassCB()
{
	mReflectedPassCB = mMainPassCB;

	DirectX::XMVECTOR mirrorPlane = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // xy plane
	DirectX::XMMATRIX R = DirectX::XMMatrixReflect(mirrorPlane);

	// Reflect the lighting.
	for (int i = 0; i < MaxLights; ++i)
	{
		DirectX::XMVECTOR lightDir = DirectX::XMLoadFloat3(&mMainPassCB.Lights[i].Direction);
		DirectX::XMVECTOR reflectedLightDir = DirectX::XMVector3TransformNormal(lightDir, R);
		DirectX::XMStoreFloat3(&mReflectedPassCB.Lights[i].Direction, reflectedLightDir);
	}

	// Reflected pass stored in index 1
	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(1, mReflectedPassCB);
}
void MyApp::UpdateWaves()
{
	// Every quarter second, generate a random wave.
	static float t_base = 0.0f;
	if ((mTimer.TotalTime() - t_base) >= 0.25f)
	{
		t_base += 0.25f;

		int i = MathHelper::Rand(4, mWaves->RowCount() - 5);
		int j = MathHelper::Rand(4, mWaves->ColumnCount() - 5);

		float r = MathHelper::RandF(0.2f, 0.5f);

		mWaves->Disturb(i, j, r);
	}

	// Update the wave simulation.
	mWaves->Update(mTimer.DeltaTime());

	// Update the wave vertex buffer with the new solution.
	auto currWavesVB = mCurrFrameResource->WavesVB.get();
	for (int i = 0; i < mWaves->VertexCount(); ++i)
	{
		Vertex v;

		v.Pos = mWaves->Position(i);
		v.Normal = mWaves->Normal(i);

		// Derive tex-coords from position by 
		// mapping [-w/2,w/2] --> [0,1]
		v.TexC.x = 0.5f + v.Pos.x / mWaves->Width();
		v.TexC.y = 0.5f - v.Pos.z / mWaves->Depth();

		currWavesVB->CopyData(i, v);
	}

	// Set the dynamic VB of the wave renderitem to the current frame VB.
	mWavesRitem->Geo->VertexBufferGPU = currWavesVB->Resource();
}

#pragma endregion Update

void MyApp::DrawRenderItems(const RenderLayer flag)
{
	UINT offset 
		= (flag == RenderLayer::Reflected) 
			? mAllRitems.size() 
			: (flag == RenderLayer::Shadow)
				? mAllRitems.size() * 2
				: 0;
	UINT objCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));
	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto materialCB = mCurrFrameResource->MaterialCB->Resource();

	// For each render item...
	for (const auto& ri : mAllRitems)
	{
		if (!(ri->LayerFlag & (1 << (int) flag)))
			continue;
		
		D3D12_VERTEX_BUFFER_VIEW vbv = ri->Geo->VertexBufferView();
		D3D12_INDEX_BUFFER_VIEW ibv = ri->Geo->IndexBufferView();
		mCommandList->IASetVertexBuffers(0, 1, &vbv);
		mCommandList->IASetIndexBuffer(&ibv);
		mCommandList->IASetPrimitiveTopology(ri->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvUavDescriptorSize);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + (ri->ObjCBIndex + offset) * objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = materialCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * matCBByteSize;
		if(flag == RenderLayer::Shadow)
			matCBAddress = materialCB->GetGPUVirtualAddress() + (MATERIAL_NAMES.size() - 1) * matCBByteSize;


		mCommandList->SetGraphicsRootDescriptorTable(0, tex);
		mCommandList->SetGraphicsRootConstantBufferView(1, objCBAddress);
		mCommandList->SetGraphicsRootConstantBufferView(3, matCBAddress);

		mCommandList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

void MyApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mHwndWindow);
}

void MyApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void MyApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.
		mTheta += dx;
		mPhi += dy;

		// Restrict the angle mPhi.
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.2 unit in the scene.
		float dx = 0.05f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.05f * static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void MyApp::OnKeyboardInput()
{
	if (GetAsyncKeyState('1') & 0x8000)
		mIsWireframe = true;
	else
		mIsWireframe = false;
}

void MyApp::UpdateCamera()
{
	// Convert Spherical to Cartesian coordinates.
	mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
	mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);
	mEyePos.y = mRadius * cosf(mPhi);

	// Build the view matrix.
	DirectX::XMVECTOR pos = DirectX::XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	DirectX::XMVECTOR target = DirectX::XMVectorZero();
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
	DirectX::XMStoreFloat4x4(&mView, view);
}

void MyApp::UpdateImGui()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ShowMainWindow();

	if (mShowDemoWindow)
		ImGui::ShowDemoWindow(&mShowDemoWindow);
}

void MyApp::ShowMainWindow()
{
	ImGui::Begin("Root");

	ImTextureID my_tex_id = (ImTextureID)mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr + mCbvSrvUavDescriptorSize * mImguiIdxTexture;
	ImGui::Checkbox("Demo Window", &mShowDemoWindow);      // Edit bools storing our window open/close state
	ImGui::Text("size: %d x %d", mImguiWidth, mImguiHeight);
	ImGui::Text("GPU handle = %p", my_tex_id);
	{
		ImGuiSliderFlags flags = ImGuiSliderFlags_None & ~ImGuiSliderFlags_WrapAround;
		ImGui::SliderInt("Width [1, 1920]", &mImguiWidth, 1, 1920, "%d", flags);
		ImGui::SliderInt("Height [1, 1080]", &mImguiHeight, 1, 1080, "%d", flags);

		ImGui::SliderInt((std::string("Texture [0, ") + std::to_string(TEXTURE_FILENAMES.size()) + "]").c_str(), &mImguiIdxTexture, 0, TEXTURE_FILENAMES.size(), "%d", flags);
		
		if (mClientWidth != mImguiWidth || mClientHeight != mImguiHeight)
		{
			mClientWidth = mImguiWidth;
			mClientHeight = mImguiHeight;
			
			// mOnResizeDirty = true;
		}
	}

	{
		ImVec2 pos = ImGui::GetCursorScreenPos();
		
		ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
		ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
		ImVec4 tint_col = true ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // No tint
		ImVec4 border_col = ImGui::GetStyleColorVec4(ImGuiCol_Border);
		ImGui::Image(my_tex_id, ImVec2(mImguiWidth, mImguiHeight), uv_min, uv_max, tint_col, border_col);
	}
	ImGui::End();
}

float MyApp::GetHillsHeight(const float x, const float z) const
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

DirectX::XMFLOAT3 MyApp::GetHillsNormal(const float x, const float z) const
{
	// n = (-df/dx, 1, -df/dz)
	DirectX::XMFLOAT3 n(
		-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
		1.0f,
		-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

	DirectX::XMVECTOR unitNormal = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&n));
	DirectX::XMStoreFloat3(&n, unitNormal);

	return n;
}
