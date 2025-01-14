#include "pch.h"

#include "MyApp.h"

MyApp::MyApp()
	: AppBase()
{
}

MyApp::MyApp(uint32_t width, uint32_t height, std::wstring name)
	: AppBase(width, height, name)
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
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildShapeGeometry();
	BuildSkullGeometry();
	BuildLandGeometry();
	BuildWavesGeometryBuffers();
	BuildMaterials();
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
	for (size_t i = 0; i < TEXTURE_NUM; ++i)
	{
		auto texture = std::make_unique<Texture>();
		texture->Name = TEXTURE_NAMES[i];
		texture->Filename = TEXTUER_DIR + TEXTUER_FILE_NAMES[i];
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(mDevice.Get(), mCommandList.Get(), texture->Filename.c_str(), texture->Resource, texture->UploadHeap));

		mTextures[texture->Name] = std::move(texture);
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
		/* UINT NumDescriptors				*/.NumDescriptors = TEXTURE_NUM,
		/* D3D12_DESCRIPTOR_HEAP_FLAGS Flags*/.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		/* UINT NodeMask					*/.NodeMask = 0
	};
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	//
	// Fill out the heap with actual descriptors.
	//
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
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
	for (size_t i = 0; i < TEXTURE_NUM; ++i)
	{
		auto texture = mTextures[TEXTURE_NAMES[i]]->Resource;

		srvDesc.Format = texture->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;
		mDevice->CreateShaderResourceView(texture.Get(), &srvDesc, hDescriptor);
		hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
	}
}

void MyApp::BuildShadersAndInputLayout()
{
	mShaders[VS_NAME] = D3DUtil::LoadBinary(VS_DIR);
	mShaders[PS_NAME] = D3DUtil::LoadBinary(PS_DIR);

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
	geo->Name = MESH_GEOMETRY_NAMES[2];
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

	geo->DrawArgs[MESH_MAIN_NAMES[1]] = submesh;
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
	geo->Name = MESH_GEOMETRY_NAMES[3];

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

	geo->DrawArgs[MESH_MAIN_NAMES[1]] = submesh;

	mGeometries[geo->Name] = std::move(geo);
}

void MyApp::BuildShapeGeometry()
{
	//=========================================================
	// Part 1. Mesh 생성
	//=========================================================
	std::vector<GeometryGenerator::MeshData> meshes(MESH_MAIN_NUM);
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

	std::vector<SubmeshGeometry> submeshes(MESH_MAIN_NUM);
	std::vector<Vertex> vertices(totalVertexCount);
	std::vector<std::uint16_t> indices;
	UINT k = 0;
	for (size_t i = 0; i < MESH_MAIN_NUM; ++i)
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
	geo->Name = MESH_GEOMETRY_NAMES[0];
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

	for (size_t i = 0; i < MESH_MAIN_NUM; ++i)
	{
		geo->DrawArgs[MESH_MAIN_NAMES[i]] = submeshes[i];
	}
	mGeometries[geo->Name] = std::move(geo);
}

void MyApp::BuildSkullGeometry()
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
	geo->Name = MESH_GEOMETRY_NAMES[1];

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

	geo->DrawArgs[MESH_MODEL_NAMES[0]] = submesh;

	mGeometries[geo->Name] = std::move(geo);
}

void MyApp::BuildMaterials()
{
	auto bricks0 = std::make_unique<Material>();
	bricks0->Name = MATERIAL_NAMES[0];
	bricks0->MatCBIndex = 0;
	bricks0->DiffuseSrvHeapIndex = 1;
	bricks0->DiffuseAlbedo = DirectX::XMFLOAT4(DirectX::Colors::ForestGreen);
	bricks0->FresnelR0 = DirectX::XMFLOAT3(0.02f, 0.02f, 0.02f);
	bricks0->Roughness = 0.1f;

	auto stone0 = std::make_unique<Material>();
	stone0->Name = MATERIAL_NAMES[1];
	stone0->MatCBIndex = 1;
	stone0->DiffuseSrvHeapIndex = 2;
	stone0->DiffuseAlbedo = DirectX::XMFLOAT4(DirectX::Colors::LightSteelBlue);
	stone0->FresnelR0 = DirectX::XMFLOAT3(0.05f, 0.05f, 0.05f);
	stone0->Roughness = 0.3f;

	auto tile0 = std::make_unique<Material>();
	tile0->Name = MATERIAL_NAMES[2];
	tile0->MatCBIndex = 2;
	tile0->DiffuseSrvHeapIndex = 3;
	tile0->DiffuseAlbedo = DirectX::XMFLOAT4(DirectX::Colors::LightGray);
	tile0->FresnelR0 = DirectX::XMFLOAT3(0.02f, 0.02f, 0.02f);
	tile0->Roughness = 0.2f;

	auto skullMat = std::make_unique<Material>();
	skullMat->Name = MATERIAL_NAMES[3];
	skullMat->MatCBIndex = 3;
	skullMat->DiffuseSrvHeapIndex = 0;
	skullMat->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	skullMat->FresnelR0 = DirectX::XMFLOAT3(0.05f, 0.05f, 0.05f);
	skullMat->Roughness = 0.3f;

	auto grass = std::make_unique<Material>();
	grass->Name = MATERIAL_NAMES[4];
	grass->MatCBIndex = 4;
	grass->DiffuseSrvHeapIndex = 4;
	grass->DiffuseAlbedo = DirectX::XMFLOAT4(0.2f, 0.6f, 0.2f, 1.0f);
	grass->FresnelR0 = DirectX::XMFLOAT3(0.01f, 0.01f, 0.01f);
	grass->Roughness = 0.125f;

	auto water = std::make_unique<Material>();
	water->Name = MATERIAL_NAMES[5];
	water->MatCBIndex = 5;
	water->DiffuseSrvHeapIndex = 5;
	water->DiffuseAlbedo = DirectX::XMFLOAT4(0.0f, 0.2f, 0.6f, 1.0f);
	water->FresnelR0 = DirectX::XMFLOAT3(0.1f, 0.1f, 0.1f);
	water->Roughness = 0.0f;

	mMaterials[MATERIAL_NAMES[0]] = std::move(bricks0);
	mMaterials[MATERIAL_NAMES[1]] = std::move(stone0);
	mMaterials[MATERIAL_NAMES[2]] = std::move(tile0);
	mMaterials[MATERIAL_NAMES[3]] = std::move(skullMat);
	mMaterials[MATERIAL_NAMES[4]] = std::move(grass);
	mMaterials[MATERIAL_NAMES[5]] = std::move(water);
}


void MyApp::BuildRenderItems()
{
	UINT objCBIndex = 0;

	auto boxRitem = std::make_unique<RenderItem>();
	DirectX::XMStoreFloat4x4(&boxRitem->World, DirectX::XMMatrixScaling(2.0f, 2.0f, 2.0f) * DirectX::XMMatrixTranslation(0.0f, 0.5f, 0.0f));
	DirectX::XMStoreFloat4x4(&boxRitem->TexTransform, DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f));
	boxRitem->ObjCBIndex = objCBIndex++;
	boxRitem->Mat = mMaterials[MATERIAL_NAMES[1]].get();
	boxRitem->Geo = mGeometries[MESH_GEOMETRY_NAMES[0]].get();
	boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs[MESH_MAIN_NAMES[0]].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs[MESH_MAIN_NAMES[0]].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs[MESH_MAIN_NAMES[0]].BaseVertexLocation;
	mAllRitems.push_back(std::move(boxRitem));

	auto gridRitem = std::make_unique<RenderItem>();
	gridRitem->World = MathHelper::Identity4x4();
	DirectX::XMStoreFloat4x4(&gridRitem->TexTransform, DirectX::XMMatrixScaling(8.0f, 8.0f, 1.0f));
	gridRitem->ObjCBIndex = objCBIndex++;
	gridRitem->Mat = mMaterials[MATERIAL_NAMES[2]].get();
	gridRitem->Geo = mGeometries[MESH_GEOMETRY_NAMES[0]].get();
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs[MESH_MAIN_NAMES[1]].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs[MESH_MAIN_NAMES[1]].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs[MESH_MAIN_NAMES[1]].BaseVertexLocation;
	mAllRitems.push_back(std::move(gridRitem));

	auto skullRitem = std::make_unique<RenderItem>();
	skullRitem->World = MathHelper::Identity4x4();
	DirectX::XMStoreFloat4x4(&skullRitem->TexTransform, DirectX::XMMatrixScaling(8.0f, 8.0f, 1.0f));
	skullRitem->ObjCBIndex = objCBIndex++;
	skullRitem->Mat = mMaterials[MATERIAL_NAMES[3]].get();
	skullRitem->Geo = mGeometries[MESH_GEOMETRY_NAMES[1]].get();
	skullRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullRitem->IndexCount = skullRitem->Geo->DrawArgs[MESH_MODEL_NAMES[0]].IndexCount;
	skullRitem->StartIndexLocation = skullRitem->Geo->DrawArgs[MESH_MODEL_NAMES[0]].StartIndexLocation;
	skullRitem->BaseVertexLocation = skullRitem->Geo->DrawArgs[MESH_MODEL_NAMES[0]].BaseVertexLocation;
	mAllRitems.push_back(std::move(skullRitem));

	DirectX::XMMATRIX brickTexTransform = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);
	for (int i = 0; i < 5; ++i)
	{
		auto leftCylRitem = std::make_unique<RenderItem>();
		auto rightCylRitem = std::make_unique<RenderItem>();
		auto leftSphereRitem = std::make_unique<RenderItem>();
		auto rightSphereRitem = std::make_unique<RenderItem>();

		DirectX::XMMATRIX leftCylWorld = DirectX::XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
		DirectX::XMMATRIX rightCylWorld = DirectX::XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);
		DirectX::XMMATRIX leftSphereWorld = DirectX::XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
		DirectX::XMMATRIX rightSphereWorld = DirectX::XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);

		DirectX::XMStoreFloat4x4(&leftCylRitem->World, leftCylWorld);
		DirectX::XMStoreFloat4x4(&leftCylRitem->TexTransform, brickTexTransform);
		leftCylRitem->ObjCBIndex = objCBIndex++;
		leftCylRitem->Mat = mMaterials[MATERIAL_NAMES[0]].get();
		leftCylRitem->Geo = mGeometries[MESH_GEOMETRY_NAMES[0]].get();
		leftCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs[MESH_MAIN_NAMES[3]].IndexCount;
		leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs[MESH_MAIN_NAMES[3]].StartIndexLocation;
		leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs[MESH_MAIN_NAMES[3]].BaseVertexLocation;

		DirectX::XMStoreFloat4x4(&rightCylRitem->World, rightCylWorld);
		DirectX::XMStoreFloat4x4(&rightCylRitem->TexTransform, brickTexTransform);
		rightCylRitem->ObjCBIndex = objCBIndex++;
		rightCylRitem->Mat = mMaterials[MATERIAL_NAMES[0]].get();
		rightCylRitem->Geo = mGeometries[MESH_GEOMETRY_NAMES[0]].get();
		rightCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs[MESH_MAIN_NAMES[3]].IndexCount;
		rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs[MESH_MAIN_NAMES[3]].StartIndexLocation;
		rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs[MESH_MAIN_NAMES[3]].BaseVertexLocation;

		DirectX::XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
		leftSphereRitem->TexTransform = MathHelper::Identity4x4();
		leftSphereRitem->ObjCBIndex = objCBIndex++;
		leftSphereRitem->Mat = mMaterials[MATERIAL_NAMES[1]].get();
		leftSphereRitem->Geo = mGeometries[MESH_GEOMETRY_NAMES[0]].get();
		leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs[MESH_MAIN_NAMES[2]].IndexCount;
		leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs[MESH_MAIN_NAMES[2]].StartIndexLocation;
		leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs[MESH_MAIN_NAMES[2]].BaseVertexLocation;

		DirectX::XMStoreFloat4x4(&rightSphereRitem->World, rightSphereWorld);
		rightSphereRitem->TexTransform = MathHelper::Identity4x4();
		rightSphereRitem->ObjCBIndex = objCBIndex++;
		rightSphereRitem->Mat = mMaterials[MATERIAL_NAMES[1]].get();
		rightSphereRitem->Geo = mGeometries[MESH_GEOMETRY_NAMES[0]].get();
		rightSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRitem->IndexCount = rightSphereRitem->Geo->DrawArgs[MESH_MAIN_NAMES[2]].IndexCount;
		rightSphereRitem->StartIndexLocation = rightSphereRitem->Geo->DrawArgs[MESH_MAIN_NAMES[2]].StartIndexLocation;
		rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geo->DrawArgs[MESH_MAIN_NAMES[2]].BaseVertexLocation;

		mAllRitems.push_back(std::move(leftCylRitem));
		mAllRitems.push_back(std::move(leftSphereRitem));
		mAllRitems.push_back(std::move(rightCylRitem));
		mAllRitems.push_back(std::move(rightSphereRitem));
	}

	auto wavesRitem = std::make_unique<RenderItem>();
	wavesRitem->World = MathHelper::Identity4x4();
	wavesRitem->ObjCBIndex = objCBIndex++;
	wavesRitem->Mat = mMaterials[MATERIAL_NAMES[5]].get();
	wavesRitem->Geo = mGeometries[MESH_GEOMETRY_NAMES[3]].get();
	wavesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wavesRitem->IndexCount = wavesRitem->Geo->DrawArgs[MESH_MAIN_NAMES[1]].IndexCount;
	wavesRitem->StartIndexLocation = wavesRitem->Geo->DrawArgs[MESH_MAIN_NAMES[1]].StartIndexLocation;
	wavesRitem->BaseVertexLocation = wavesRitem->Geo->DrawArgs[MESH_MAIN_NAMES[1]].BaseVertexLocation;
	mWavesRitem = wavesRitem.get();
	mAllRitems.push_back(std::move(wavesRitem));

	auto landRitem = std::make_unique<RenderItem>();
	landRitem->World = MathHelper::Identity4x4();
	landRitem->ObjCBIndex = objCBIndex++;
	landRitem->Mat = mMaterials[MATERIAL_NAMES[4]].get();
	landRitem->Geo = mGeometries[MESH_GEOMETRY_NAMES[2]].get();
	landRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	landRitem->IndexCount = landRitem->Geo->DrawArgs[MESH_MAIN_NAMES[1]].IndexCount;
	landRitem->StartIndexLocation = landRitem->Geo->DrawArgs[MESH_MAIN_NAMES[1]].StartIndexLocation;
	landRitem->BaseVertexLocation = landRitem->Geo->DrawArgs[MESH_MAIN_NAMES[1]].BaseVertexLocation;
	mAllRitems.push_back(std::move(landRitem));

	// All the render items are opaque.
	for (auto& e : mAllRitems)
		mRitemLayer[(int)RenderLayer::Opaque].push_back(e.get());
}

void MyApp::BuildFrameResources()
{
	for (int i = 0; i < APP_NUM_FRAME_RESOURCES; ++i)
		mFrameResources.push_back(std::make_unique<FrameResource>(mDevice.Get(), 1, (UINT)mAllRitems.size(), (UINT)mMaterials.size(), mWaves->VertexCount()));
}

void MyApp::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc
	{
		/* ID3D12RootSignature* pRootSignature								*/.pRootSignature = mRootSignature.Get(),
		/* D3D12_SHADER_BYTECODE VS											*/.VS = {reinterpret_cast<BYTE*>(mShaders[VS_NAME]->GetBufferPointer()), mShaders[VS_NAME]->GetBufferSize()},
		/* D3D12_SHADER_BYTECODE PS											*/.PS = {reinterpret_cast<BYTE*>(mShaders[PS_NAME]->GetBufferPointer()), mShaders[PS_NAME]->GetBufferSize()},
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
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs[gPSOName[0]])));

	//
	// PSO for opaque wireframe objects.
	//
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs[gPSOName[1]])));
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
}

void MyApp::AnimateMaterials()
{
	// Scroll the water material texture coordinates.
	auto waterMat = mMaterials[MATERIAL_NAMES[5]].get();

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

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
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

void MyApp::Render()
{
	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(mCurrFrameResource->CmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	if (mIsWireframe)
	{
		ThrowIfFailed(mCommandList->Reset(mCurrFrameResource->CmdListAlloc.Get(), mPSOs[gPSOName[1]].Get()));
	}
	else
	{
		ThrowIfFailed(mCommandList->Reset(mCurrFrameResource->CmdListAlloc.Get(), mPSOs[gPSOName[0]].Get()));
	}

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	D3D12_RESOURCE_BARRIER RenderBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mSwapChainBuffer[mCurrBackBuffer].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	mCommandList->ResourceBarrier(1, &RenderBarrier);

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(mSwapChainDescriptor[mCurrBackBuffer], DirectX::Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(mDepthStencilDescriptor, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &mSwapChainDescriptor[mCurrBackBuffer], true, &mDepthStencilDescriptor);

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	// Bind per-pass constant buffer.  We only need to do this once per-pass.
	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

	DrawRenderItems(mRitemLayer[(int)RenderLayer::Opaque]);

	// Indicate a state transition on the resource usage.
	D3D12_RESOURCE_BARRIER DefaultBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mSwapChainBuffer[mCurrBackBuffer].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	mCommandList->ResourceBarrier(1, &DefaultBarrier);

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(1, 0));
	//// Swap the back and front buffers
	//ThrowIfFailed(mSwapChain->Present(1, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % APP_NUM_BACK_BUFFERS;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void MyApp::DrawRenderItems(const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));
	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto materialCB = mCurrFrameResource->MaterialCB->Resource();

	// For each render item...
	for (const auto& ri : ritems)
	{
		D3D12_VERTEX_BUFFER_VIEW vbv = ri->Geo->VertexBufferView();
		D3D12_INDEX_BUFFER_VIEW ibv = ri->Geo->IndexBufferView();
		mCommandList->IASetVertexBuffers(0, 1, &vbv);
		mCommandList->IASetIndexBuffer(&ibv);
		mCommandList->IASetPrimitiveTopology(ri->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvUavDescriptorSize);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = materialCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * matCBByteSize;

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
