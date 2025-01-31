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
	mLayerType[0] = RenderLayer::Opaque;
	mLayerType[1] = RenderLayer::AlphaTested;
	mLayerType[2] = RenderLayer::Mirror;
	mLayerType[3] = RenderLayer::Reflected;
	mLayerType[4] = RenderLayer::Subdivision;
	mLayerType[5] = RenderLayer::Transparent;
	mLayerType[6] = RenderLayer::TreeSprites;
	mLayerType[7] = RenderLayer::Normal;

	mLayerStencil[0] = 0;
	mLayerStencil[1] = 0;
	mLayerStencil[2] = 1;
	mLayerStencil[3] = 1;
	mLayerStencil[4] = 0;
	mLayerStencil[5] = 0;
	mLayerStencil[6] = 0;
	mLayerStencil[7] = 0;

	mLayerCBIdx[0] = 0;
	mLayerCBIdx[1] = 0;
	mLayerCBIdx[2] = 1;
	mLayerCBIdx[3] = 1;
	mLayerCBIdx[4] = 0;
	mLayerCBIdx[5] = 0;
	mLayerCBIdx[6] = 0;
	mLayerCBIdx[7] = 0;

	for (int i = 8; i < MAX_LAYER_DEPTH; ++i)
	{
		mLayerType[i] = RenderLayer::None;
		mLayerStencil[i] = 0;
		mLayerCBIdx[i] = 0;
	}
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
	mCSWaves = std::make_unique<GpuWaves>(mDevice.Get(), mCommandList.Get(), 256, 256, 0.25f, 0.03f, 2.0f, 0.2f, m4xMsaaState, m4xMsaaQuality, mRootSignature.Get());
	mCSBlurFilter = std::make_unique<BlurFilter>(mDevice.Get(), mClientWidth, mClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
	mCSAdd = std::make_unique<CSAdd>(mDevice.Get(), mCommandList.Get());
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildShapeGeometry();
	BuildModelGeometry();
	BuildLandGeometry();
	BuildWavesGeometryBuffers();
	BuildRoomGeometry();
	BuildTreeSpritesGeometry();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSO();


	if (!InitImgui())
		return false;

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

	CD3DX12_DESCRIPTOR_RANGE displacementMapTable;
	displacementMapTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[5];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0); // register b0
	slotRootParameter[2].InitAsConstantBufferView(1); // register b1
	slotRootParameter[3].InitAsConstantBufferView(2); // register b2
	slotRootParameter[4].InitAsDescriptorTable(1, &displacementMapTable, D3D12_SHADER_VISIBILITY_ALL);

	auto staticSamplers = GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(5, slotRootParameter, (UINT)staticSamplers.size(), staticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
	if (errorBlob != nullptr)
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	ThrowIfFailed(hr);

	ThrowIfFailed(mDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void MyApp::BuildDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
	// 현재 SRV Heap은 4가지 데이터를 종합하여 관리하고있다.
	// 1. Imgui Data (SRV_IMGUI_SIZE(64))
	// 2. Texture Data (TEXTURE_FILENAMES.size())
	// 3. Viewport Buffer (SRV_USER_SIZE)
	// 4. CS Blar Buffer (SRV_CS_BLAR_SIZE(4))
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc
	{
		/* D3D12_DESCRIPTOR_HEAP_TYPE Type	*/.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		/* UINT NumDescriptors				*/.NumDescriptors = SRV_IMGUI_SIZE + (UINT)TEXTURE_FILENAMES.size() + SRV_USER_SIZE + mCSBlurFilter->DescriptorCount() + mCSWaves->DescriptorCount(),
		/* D3D12_DESCRIPTOR_HEAP_FLAGS Flags*/.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		/* UINT NodeMask					*/.NodeMask = 0
	};
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	BuildShaderResourceViews();
	BuildCSBlurShaderResourceViews();
	BuildCSWavesShaderResourceViews();
}

void MyApp::BuildShaderResourceViews()
{
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
	hDescriptor.Offset(SRV_IMGUI_SIZE, mCbvSrvUavDescriptorSize);
	for (int i = 0; i < SIZE_STD_TEX; ++i)
	{
		auto texture = mTextures[TEXTURE_FILENAMES[i]]->Resource;

		srvDesc.Format = texture->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;
		mDevice->CreateShaderResourceView(texture.Get(), &srvDesc, hDescriptor);
		hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
	}

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2DArray.MipLevels = -1;
	srvDesc.Texture2DArray.FirstArraySlice = 0;
	for (int i = SIZE_STD_TEX; i < TEXTURE_FILENAMES.size(); ++i)
	{
		auto texture = mTextures[TEXTURE_FILENAMES[i]]->Resource;

		srvDesc.Format = texture->GetDesc().Format;
		srvDesc.Texture2DArray.ArraySize = texture->GetDesc().DepthOrArraySize;
		mDevice->CreateShaderResourceView(texture.Get(), &srvDesc, hDescriptor);
		hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
	}

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	for (int i = 0; i < SRV_USER_SIZE; ++i)
	{
		mDevice->CreateShaderResourceView(mSRVUserBuffer[i].Get(), &srvDesc, hDescriptor);
		hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
	}
}

void MyApp::BuildCSBlurShaderResourceViews()
{
	//
	// Fill out the heap with the descriptors to the BlurFilter resources.
	//
	mCSBlurFilter->BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), SRV_IMGUI_SIZE + (UINT)TEXTURE_FILENAMES.size() + SRV_USER_SIZE, mCbvSrvUavDescriptorSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), SRV_IMGUI_SIZE + (UINT)TEXTURE_FILENAMES.size() + SRV_USER_SIZE, mCbvSrvUavDescriptorSize),
		mCbvSrvUavDescriptorSize);
}

void MyApp::BuildCSWavesShaderResourceViews()
{
	mCSWaves->BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), SRV_IMGUI_SIZE + (UINT)TEXTURE_FILENAMES.size() + SRV_USER_SIZE + mCSBlurFilter->DescriptorCount(), mCbvSrvUavDescriptorSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), SRV_IMGUI_SIZE + (UINT)TEXTURE_FILENAMES.size() + SRV_USER_SIZE + mCSBlurFilter->DescriptorCount(), mCbvSrvUavDescriptorSize),
		mCbvSrvUavDescriptorSize);
}

void MyApp::BuildShadersAndInputLayout()
{
	//const D3D_SHADER_MACRO defines[] =
	//{
	//	"FOG", "1",
	//	NULL, NULL
	//};

	//const D3D_SHADER_MACRO alphaTestDefines[] =
	//{
	//	"FOG", "1",
	//	"ALPHA_TEST", "1",
	//	NULL, NULL
	//};
	//mShaders["standardVS"] = D3DUtil::CompileShader(L"Shaders\\Default.hlsl", nullptr, "VS", "vs_5_0");
	//mShaders["opaquePS"] = D3DUtil::CompileShader(L"Shaders\\Default.hlsl", defines, "PS", "ps_5_0");
	//mShaders["alphaTestedPS"] = D3DUtil::CompileShader(L"Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_0");

	//mShaders["treeSpriteVS"] = D3DUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "VS", "vs_5_0");
	//mShaders["treeSpriteGS"] = D3DUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "GS", "gs_5_0");
	// mShaders["treeSpritePS"] = D3DUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", alphaTestDefines, "PS", "ps_5_0");

	for (size_t i = 0; i < VS_DIR.size(); ++i) mShaders[VS_NAME[i]] = D3DUtil::LoadBinary(VS_DIR[i]);
	for (size_t i = 0; i < GS_DIR.size(); ++i) mShaders[GS_NAME[i]] = D3DUtil::LoadBinary(GS_DIR[i]);
	for (size_t i = 0; i < CS_DIR.size(); ++i) mShaders[CS_NAME[i]] = D3DUtil::LoadBinary(CS_DIR[i]);
	for (size_t i = 0; i < PS_DIR.size(); ++i) mShaders[PS_NAME[i]] = D3DUtil::LoadBinary(PS_DIR[i]);

	mMainInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	mTreeSpriteInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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

void MyApp::BuildCSWavesGeometry()
{
	GeometryGenerator::MeshData grid = GeometryGenerator::CreateGrid(160.0f, 160.0f, mCSWaves->RowCount(), mCSWaves->ColumnCount());

	std::vector<Vertex> vertices(grid.Vertices.size());
	for (size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		vertices[i].Pos = grid.Vertices[i].Position;
		vertices[i].Normal = grid.Vertices[i].Normal;
		vertices[i].TexC = grid.Vertices[i].TexC;
	}

	std::vector<std::uint32_t> indices = grid.Indices32;

	UINT vbByteSize = mCSWaves->VertexCount() * sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint32_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = GEO_MESH_NAMES[3].first;

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

	geo->DrawArgs[GEO_MESH_NAMES[3].second[0]] = submesh;

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
		for (const auto& ver : meshes[i].Vertices)
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

void MyApp::BuildTreeSpritesGeometry()
{
	struct TreeSpriteVertex
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT2 Size;
	};

	static const int treeCount = 10;
	std::array<TreeSpriteVertex, treeCount> vertices;
	std::array<std::int16_t, treeCount> indices;
	for (int i = 0; i < treeCount; ++i)
	{
		float x = MathHelper::RandF(-45.0f, 45.0f);
		float z = MathHelper::RandF(-45.0f, 45.0f);
		float y = GetHillsHeight(x, z);

		// Move tree slightly above land height.
		y += 8.0f;

		vertices[i].Pos = DirectX::XMFLOAT3(x, y, z);
		vertices[i].Size = DirectX::XMFLOAT2(20.0f, 20.0f);
		indices[i] = i;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(TreeSpriteVertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = GEO_MESH_NAMES[5].first;

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = D3DUtil::CreateDefaultBuffer(mDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(mDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(TreeSpriteVertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs[GEO_MESH_NAMES[5].second[0]] = submesh;

	mGeometries[geo->Name] = std::move(geo);
}

void MyApp::BuildMaterials()
{
	for (size_t i = 0; i < TEXTURE_FILENAMES.size(); ++i)
	{
		auto mat = std::make_unique<Material>();
		mat->Name = MATERIAL_NAMES[i];
		mat->MatCBIndex = i;
		mat->DiffuseSrvHeapIndex = i + SRV_IMGUI_SIZE;
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
	skullMat->MatCBIndex = offset++;
	skullMat->DiffuseSrvHeapIndex = SRV_IMGUI_SIZE;
	skullMat->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	skullMat->FresnelR0 = DirectX::XMFLOAT3(0.05f, 0.05f, 0.05f);
	skullMat->Roughness = 0.3f;
	mMaterials[skullMat->Name] = std::move(skullMat);

	auto shadowMat = std::make_unique<Material>();
	shadowMat->Name = MATERIAL_NAMES[offset];
	shadowMat->MatCBIndex = offset++;
	shadowMat->DiffuseSrvHeapIndex = SRV_IMGUI_SIZE;
	shadowMat->DiffuseAlbedo = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
	shadowMat->FresnelR0 = DirectX::XMFLOAT3(0.001f, 0.001f, 0.001f);
	shadowMat->Roughness = 0.0f;
	mMaterials[shadowMat->Name] = std::move(shadowMat);

	auto viewportMat = std::make_unique<Material>();
	viewportMat->Name = MATERIAL_NAMES[offset];
	viewportMat->MatCBIndex = offset++;
	viewportMat->DiffuseSrvHeapIndex = SRV_IMGUI_SIZE + TEXTURE_FILENAMES.size();
	viewportMat->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	viewportMat->FresnelR0 = DirectX::XMFLOAT3(0.001f, 0.001f, 0.001f);
	viewportMat->Roughness = 0.0f;
	mMaterials[viewportMat->Name] = std::move(viewportMat);
}


void MyApp::BuildRenderItems()
{
	UINT objCBIndex = 0;

	//=========================================================
	// GEO_MESH_NAMES[0]: ShapeGeo
	//=========================================================
	auto boxRitem = std::make_unique<RenderItem>();
	for (int i = 0; i < MATERIAL_NAMES.size(); ++i)
	{
		boxRitem = std::make_unique<RenderItem>();
		DirectX::XMStoreFloat4x4(&boxRitem->World, DirectX::XMMatrixScaling(2.0f, 2.0f, 2.0f) * DirectX::XMMatrixTranslation((i % 5) * 5.0f, 15.5f, -5.0 + -5.0 * (i / 5)));
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
	for (int i = 0; i < MATERIAL_NAMES.size(); ++i)
	{
		boxRitem = std::make_unique<RenderItem>();
		DirectX::XMStoreFloat4x4(&boxRitem->World, DirectX::XMMatrixScaling(2.0f, 2.0f, 2.0f) * DirectX::XMMatrixTranslation((i % 5) * 5.0f, 15.5f, 5.0f * (i / 5)));
		DirectX::XMStoreFloat4x4(&boxRitem->TexTransform, DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f));
		boxRitem->ObjCBIndex = objCBIndex++;
		boxRitem->Mat = mMaterials[MATERIAL_NAMES[i]].get();
		boxRitem->Geo = mGeometries[GEO_MESH_NAMES[0].first].get();
		boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		boxRitem->IndexCount = boxRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[0]].IndexCount;
		boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[0]].StartIndexLocation;
		boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[0]].BaseVertexLocation;
		boxRitem->LayerFlag
			= (1 << (int)RenderLayer::Subdivision)
			| (1 << (int)RenderLayer::Normal)
			| (1 << (int)RenderLayer::SubdivisionWireframe)
			| (1 << (int)RenderLayer::NormalWireframe);
		mAllRitems.push_back(std::move(boxRitem));
	}

	boxRitem = std::make_unique<RenderItem>();
	DirectX::XMStoreFloat4x4(&boxRitem->World, DirectX::XMMatrixScaling(2.0f, 2.0f, 2.0f) * DirectX::XMMatrixTranslation(0.0f, 6.5f, 10.0f));
	DirectX::XMStoreFloat4x4(&boxRitem->TexTransform, DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f));
	boxRitem->ObjCBIndex = objCBIndex++;
	boxRitem->Mat = mMaterials[MATERIAL_NAMES[5]].get();
	boxRitem->Geo = mGeometries[GEO_MESH_NAMES[0].first].get();
	boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[0]].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[0]].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs[GEO_MESH_NAMES[0].second[0]].BaseVertexLocation;
	boxRitem->LayerFlag 
		= (1 << (int)RenderLayer::AlphaTested)
		| (1 << (int)RenderLayer::AlphaTestedWireframe);
	mAllRitems.push_back(std::move(boxRitem));


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
	landRitem->Mat = mMaterials["grass"].get();
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
	wavesRitem->Mat = mMaterials["water1"].get();
	wavesRitem->Geo = mGeometries[GEO_MESH_NAMES[3].first].get();
	wavesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wavesRitem->IndexCount = wavesRitem->Geo->DrawArgs[GEO_MESH_NAMES[3].second[0]].IndexCount;
	wavesRitem->StartIndexLocation = wavesRitem->Geo->DrawArgs[GEO_MESH_NAMES[3].second[0]].StartIndexLocation;
	wavesRitem->BaseVertexLocation = wavesRitem->Geo->DrawArgs[GEO_MESH_NAMES[3].second[0]].BaseVertexLocation;
	wavesRitem->LayerFlag 
		= (1 << (int)RenderLayer::Transparent);
	mWavesRitem = wavesRitem.get();
	mAllRitems.push_back(std::move(wavesRitem));

	wavesRitem = std::make_unique<RenderItem>();
	wavesRitem->World = MathHelper::Identity4x4();
	XMStoreFloat4x4(&wavesRitem->TexTransform, DirectX::XMMatrixScaling(5.0f, 5.0f, 1.0f));
	wavesRitem->DisplacementMapTexelSize.x = 1.0f / mCSWaves->ColumnCount();
	wavesRitem->DisplacementMapTexelSize.y = 1.0f / mCSWaves->RowCount();
	wavesRitem->GridSpatialStep = mCSWaves->SpatialStep();
	wavesRitem->ObjCBIndex = objCBIndex++;
	wavesRitem->Mat = mMaterials["ice"].get();
	wavesRitem->Geo = mGeometries[GEO_MESH_NAMES[3].first].get();
	wavesRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wavesRitem->IndexCount = wavesRitem->Geo->DrawArgs[GEO_MESH_NAMES[3].second[0]].IndexCount;
	wavesRitem->StartIndexLocation = wavesRitem->Geo->DrawArgs[GEO_MESH_NAMES[3].second[0]].StartIndexLocation;
	wavesRitem->BaseVertexLocation = wavesRitem->Geo->DrawArgs[GEO_MESH_NAMES[3].second[0]].BaseVertexLocation;
	wavesRitem->LayerFlag
		= (1 << (int)RenderLayer::WaveVS);
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

	//=========================================================
	// GEO_MESH_NAMES[5]: TreeSpritesGeo
	//=========================================================
	auto treeSpritesRitem = std::make_unique<RenderItem>();
	treeSpritesRitem->World = MathHelper::Identity4x4();
	treeSpritesRitem->ObjCBIndex = objCBIndex++;
	treeSpritesRitem->Mat = mMaterials["treearray"].get();
	treeSpritesRitem->Geo = mGeometries[GEO_MESH_NAMES[5].first].get();
	treeSpritesRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	treeSpritesRitem->IndexCount = treeSpritesRitem->Geo->DrawArgs[GEO_MESH_NAMES[5].second[0]].IndexCount;
	treeSpritesRitem->StartIndexLocation = treeSpritesRitem->Geo->DrawArgs[GEO_MESH_NAMES[5].second[0]].StartIndexLocation;
	treeSpritesRitem->BaseVertexLocation = treeSpritesRitem->Geo->DrawArgs[GEO_MESH_NAMES[5].second[0]].BaseVertexLocation;
	treeSpritesRitem->LayerFlag
		= (1 << (int)RenderLayer::TreeSprites)
		| (1 << (int)RenderLayer::Normal);
	mAllRitems.push_back(std::move(treeSpritesRitem));
}

void MyApp::BuildFrameResources()
{
	for (int i = 0; i < APP_NUM_FRAME_RESOURCES; ++i)
		mFrameResources.push_back(std::make_unique<FrameResource>(mDevice.Get(), 2, (UINT)mAllRitems.size() * 3, (UINT)mMaterials.size(), mWaves->VertexCount()));
}

void MyApp::BuildPSO()
{
	// Mesh Shader(2018) <- Compute Shader (고착화)
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
		/*		const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs			*/		.pInputElementDescs = mMainInputLayout.data(),
		/*		UINT NumElements											*/		.NumElements = (UINT)mMainInputLayout.size()
		/*	}																*/ },
		/* D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue				*/.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
		/* D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType				*/.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		/* UINT NumRenderTargets											*/.NumRenderTargets = 2,
		/* DXGI_FORMAT RTVFormats[8]										*/.RTVFormats = {mBackBufferFormat, DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN},	// 0
		/* DXGI_FORMAT DSVFormat											*/.DSVFormat = mDepthStencilFormat,
		/* DXGI_SAMPLE_DESC SampleDesc{										*/.SampleDesc = {
		/*		UINT Count;													*/		.Count = m4xMsaaState ? 4u : 1u,
		/*		UINT Quality;												*/		.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0
		/*	}																*/},
		/* UINT NodeMask													*/.NodeMask = 0,
		/* D3D12_CACHED_PIPELINE_STATE CachedPSO							*/.CachedPSO = {NULL, 0},
		/* D3D12_PIPELINE_STATE_FLAGS Flags									*/.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
	};
	opaquePsoDesc.BlendState.IndependentBlendEnable = true;

	//=====================================================
	// PSO for marking stencil mirrors.
	//=====================================================
	D3D12_GRAPHICS_PIPELINE_STATE_DESC markMirrorsPsoDesc = opaquePsoDesc;
	markMirrorsPsoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = 0;
	markMirrorsPsoDesc.BlendState.RenderTarget[1].RenderTargetWriteMask = 0;
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
	alphaTestedPsoDesc.PS = { reinterpret_cast<BYTE*>(mShaders[PS_NAME[1]]->GetBufferPointer()),	mShaders[PS_NAME[1]]->GetBufferSize() };
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
	// normalDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	normalDesc.VS = { reinterpret_cast<BYTE*>(mShaders[VS_NAME[2]]->GetBufferPointer()), mShaders[VS_NAME[2]]->GetBufferSize() };
	normalDesc.GS = { reinterpret_cast<BYTE*>(mShaders[GS_NAME[1]]->GetBufferPointer()), mShaders[GS_NAME[1]]->GetBufferSize() };
	normalDesc.PS = { reinterpret_cast<BYTE*>(mShaders[PS_NAME[2]]->GetBufferPointer()), mShaders[PS_NAME[2]]->GetBufferSize() };

	//=====================================================
	// PSO for tree sprites
	//=====================================================
	D3D12_GRAPHICS_PIPELINE_STATE_DESC treeSpritePsoDesc = opaquePsoDesc;
	treeSpritePsoDesc.VS = { reinterpret_cast<BYTE*>(mShaders["billboardVS"]->GetBufferPointer()), mShaders["billboardVS"]->GetBufferSize() };
	treeSpritePsoDesc.GS = { reinterpret_cast<BYTE*>(mShaders["billboardGS"]->GetBufferPointer()), mShaders["billboardGS"]->GetBufferSize() };
	treeSpritePsoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["billboardPS"]->GetBufferPointer()), mShaders["billboardPS"]->GetBufferSize() };
	treeSpritePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	treeSpritePsoDesc.InputLayout = { mTreeSpriteInputLayout.data(), (UINT)mTreeSpriteInputLayout.size() };
	treeSpritePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	//=====================================================
	// Create PSO
	//=====================================================
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::Opaque])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&markMirrorsPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::Mirror])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&drawReflectionsPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::Reflected])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::Transparent])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::AlphaTested])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::Shadow])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&subdivisionDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::Subdivision])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&normalDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::Normal])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&treeSpritePsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::TreeSprites])));

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
	treeSpritePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;

	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::OpaqueWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&markMirrorsPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::MirrorWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&drawReflectionsPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::ReflectedWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::TransparentWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::AlphaTestedWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::ShadowWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&subdivisionDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::SubdivisionWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&normalDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::NormalWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&treeSpritePsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::TreeSpritesWireframe])));
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

	if (mSrvDescriptorHeap)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc
		{
			/* DXGI_FORMAT Format															*/.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			/* D3D12_SRV_DIMENSION ViewDimension											*/.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
			/* UINT Shader4ComponentMapping													*/.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			/* union {																		*/
			/* 	D3D12_BUFFER_SRV Buffer														*/
			/* 	D3D12_TEX1D_SRV Texture1D													*/
			/* 	D3D12_TEX1D_ARRAY_SRV Texture1DArray										*/
			/* 	D3D12_TEX2D_SRV Texture2D{													*/.Texture2D{
			/*		UINT MostDetailedMip													*/	.MostDetailedMip = 0,
			/*		UINT MipLevels															*/	.MipLevels = 1,
			/*		UINT PlaneSlice															*/	.PlaneSlice = 0,
			/*		FLOAT ResourceMinLODClamp}												*/	.ResourceMinLODClamp = 0.0f}
			/* 	D3D12_TEX2D_ARRAY_SRV Texture2DArray										*/
			/* 	D3D12_TEX2DMS_SRV Texture2DMS												*/
			/* 	D3D12_TEX2DMS_ARRAY_SRV Texture2DMSArray									*/
			/* 	D3D12_TEX3D_SRV Texture3D													*/
			/* 	D3D12_TEXCUBE_SRV TextureCube												*/
			/* 	D3D12_TEXCUBE_ARRAY_SRV TextureCubeArray									*/
			/* 	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_SRV RaytracingAccelerationStructure	*/
			/* }																			*/
		};

		CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), SRV_IMGUI_SIZE + TEXTURE_FILENAMES.size(), mCbvSrvUavDescriptorSize);

		for (int i = 0; i < SRV_USER_SIZE; ++i)
		{
			mDevice->CreateShaderResourceView(mSRVUserBuffer[i].Get(), &srvDesc, hDescriptor);
			hDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
		}
	}

	if (mCSBlurFilter)
		mCSBlurFilter->OnResize(mClientWidth, mClientHeight);
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
	ThrowIfFailed(mCommandList->Reset(mCurrFrameResource->CmdListAlloc.Get(), nullptr));

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// UpdateCSWaves();

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	D3D12_RESOURCE_BARRIER RenderBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mSwapChainBuffer[mCurrBackBuffer].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	D3D12_RESOURCE_BARRIER SRVUserBufBarrier[SRV_USER_SIZE];
	for(int i=0; i<SRV_USER_SIZE; ++i)
		SRVUserBufBarrier[i] = CD3DX12_RESOURCE_BARRIER::Transition(mSRVUserBuffer[i].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);

	mCommandList->ResourceBarrier(1, &RenderBarrier);
	for (int i = 0; i < SRV_USER_SIZE; ++i)
		mCommandList->ResourceBarrier(1, &SRVUserBufBarrier[i]);

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(mSwapChainDescriptor[mCurrBackBuffer], (float*)&mMainPassCB.FogColor, 0, nullptr);
	mCommandList->ClearRenderTargetView(mSwapChainDescriptor[APP_NUM_BACK_BUFFERS], (float*)&mMainPassCB.FogColor, 0, nullptr);
	mCommandList->ClearDepthStencilView(mDepthStencilDescriptor, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	D3D12_CPU_DESCRIPTOR_HANDLE rtvs[2];
	rtvs[0] = mSwapChainDescriptor[mCurrBackBuffer];		// 기존
	rtvs[1] = mSwapChainDescriptor[APP_NUM_BACK_BUFFERS];   // 새로 만든 RTV
	mCommandList->OMSetRenderTargets(2, rtvs, false, &mDepthStencilDescriptor);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
	// Bind per-pass constant buffer.  We only need to do this once per-pass.
	auto passCB = mCurrFrameResource->PassCB->Resource();
	UINT passCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	mCommandList->SetGraphicsRootDescriptorTable(4, mCSWaves->DisplacementMap());

	for (int i = 0; i < MAX_LAYER_DEPTH; ++i)
	{
		if (mLayerType[i] == RenderLayer::None)
			continue;
		else if (mLayerType[i] == RenderLayer::AddCS)
		{
			// ApplyBlurFilter(mSwapChainBuffer[mCurrBackBuffer].Get());
		}
		else if (mLayerType[i] == RenderLayer::BlurVerCS || mLayerType[i] == RenderLayer::BlurHorCS)
		{
			mCSBlurFilter->Execute(mCommandList.Get(), mSwapChainBuffer[mCurrBackBuffer].Get(), 4);
		}
		else if (mLayerType[i] == RenderLayer::WaveDisturbCS || mLayerType[i] == RenderLayer::WaveUpdateCS)
		{
			mCSWaves->UpdateWaves(mTimer, mCommandList.Get());
		}
		else if (mLayerType[i] == RenderLayer::WaveVS)
		{
			mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress() + passCBByteSize * mLayerCBIdx[i]);
			mCommandList->OMSetStencilRef(mLayerStencil[i]);
			mCommandList->SetPipelineState(mCSWaves->GetPSO());
			DrawRenderItems(mLayerType[i]);
		}
		else {
			mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress() + passCBByteSize * mLayerCBIdx[i]);
			mCommandList->OMSetStencilRef(mLayerStencil[i]);
			mCommandList->SetPipelineState(mPSOs[mLayerType[i]].Get());
			DrawRenderItems(mLayerType[i]);
		}
	}

	{	// Test Render Copy
		RenderBarrier.Transition.StateBefore = RenderBarrier.Transition.StateAfter;
		RenderBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
		SRVUserBufBarrier[1].Transition.StateBefore = SRVUserBufBarrier[1].Transition.StateAfter;
		SRVUserBufBarrier[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		mCommandList->ResourceBarrier(1, &RenderBarrier);
		mCommandList->ResourceBarrier(1, &SRVUserBufBarrier[1]);

		mCommandList->CopyResource(mSRVUserBuffer[1].Get(), mSwapChainBuffer[mCurrBackBuffer].Get());

		RenderBarrier.Transition.StateBefore = RenderBarrier.Transition.StateAfter;
		RenderBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		SRVUserBufBarrier[1].Transition.StateBefore = SRVUserBufBarrier[1].Transition.StateAfter;
		SRVUserBufBarrier[1].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		mCommandList->ResourceBarrier(1, &RenderBarrier);
		mCommandList->ResourceBarrier(1, &SRVUserBufBarrier[1]);
	}

	// Indicate a state transition on the resource usage.
	RenderBarrier.Transition.StateBefore = RenderBarrier.Transition.StateAfter;
	RenderBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	for (int i = 0; i < SRV_USER_SIZE; ++i)
	{
		SRVUserBufBarrier[i].Transition.StateBefore = SRVUserBufBarrier[i].Transition.StateAfter;
		SRVUserBufBarrier[i].Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
	}

	mCommandList->ResourceBarrier(1, &RenderBarrier);
	for (int i = 0; i < SRV_USER_SIZE; ++i)
		mCommandList->ResourceBarrier(1, &SRVUserBufBarrier[i]);
}

void MyApp::AnimateMaterials()
{
	// Scroll the water material texture coordinates.
	auto waterMat = mMaterials["water1"].get();

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
		= (flag == RenderLayer::Reflected || flag == RenderLayer::ReflectedWireframe)
		? mAllRitems.size()
		: (flag == RenderLayer::Shadow || flag == RenderLayer::ShadowWireframe)
		? mAllRitems.size() * 2
		: 0;
	UINT objCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));
	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto materialCB = mCurrFrameResource->MaterialCB->Resource();

	// For each render item...
	for (auto& ri : mAllRitems)
	{
		if (!(ri->LayerFlag & (1 << (int)flag)))
			continue;

		if (flag == RenderLayer::Normal
			|| flag == RenderLayer::NormalWireframe
			|| flag == RenderLayer::TreeSprites
			|| flag == RenderLayer::TreeSpritesWireframe)
			ri->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
		else
			ri->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		D3D12_VERTEX_BUFFER_VIEW vbv = ri->Geo->VertexBufferView();
		D3D12_INDEX_BUFFER_VIEW ibv = ri->Geo->IndexBufferView();
		mCommandList->IASetVertexBuffers(0, 1, &vbv);
		mCommandList->IASetIndexBuffer(&ibv);
		mCommandList->IASetPrimitiveTopology(ri->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvUavDescriptorSize);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + (ri->ObjCBIndex + offset) * objCBByteSize;
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = materialCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * matCBByteSize;
		if (flag == RenderLayer::Shadow)
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
	//if (GetAsyncKeyState('1') & 0x8000)
	//	mIsWireframe = !mIsWireframe;
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
	if (mShowTextureWindow)
		ShowTextureWindow();
	if (mShowMaterialWindow)
		ShowMaterialWindow();
	if (mShowRenderItemWindow)
		ShowRenderItemWindow();
	if (mShowViewportWindow)
		ShowViewportWindow();
}

void MyApp::ShowMainWindow()
{
	ImGui::Begin("Root");

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Window")) {
		ImGui::Checkbox("Demo Window", &mShowDemoWindow);      // Edit bools storing our window open/close state
		ImGui::Checkbox("Texture", &mShowTextureWindow);
		ImGui::Checkbox("Material", &mShowMaterialWindow);
		ImGui::Checkbox("Render Item", &mShowRenderItemWindow);
		ImGui::Checkbox("Viewport", &mShowViewportWindow);
		ImGui::TreePop();
	}

	static int layerIdx = 0;
	static int item_selected_idx = (int)mLayerType[0];
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Select Layer")) {

		ImGuiSliderFlags flags = ImGuiSliderFlags_None & ~ImGuiSliderFlags_WrapAround;
		if (ImGui::SliderInt((std::string("Render Layer [0, ") + std::to_string(MAX_LAYER_DEPTH - 1) + "]").c_str(), &layerIdx, 0, MAX_LAYER_DEPTH - 1, "%d", flags))
			item_selected_idx = (int)mLayerType[layerIdx];
		{
			ImGui::LabelText("label", "Value");
			// ImGui::SliderInt("Shader Type [0, 1]", &mLayerType[layerIdx], 0, 1, "%d", flags);

			const char* items[] = { 
				"None",
				"Opaque",
				"Mirror",
				"Reflected",
				"AlphaTested",
				"Transparent",
				"Shadow",
				"Subdivision",
				"Normal",
				"TreeSprites",
				"OpaqueWireframe",
				"MirrorWireframe",
				"ReflectedWireframe",
				"AlphaTestedWireframe",
				"TransparentWireframe",
				"ShadowWireframe",
				"SubdivisionWireframe",
				"NormalWireframe",
				"TreeSpritesWireframe",
				"AddCS",
				"BlurHorCS",
				"BlurVerCS",
				"WaveVS",
				"WaveDisturbCS",
				"WaveUpdateCS"
			};
			if (ImGui::BeginListBox("Shader Type"))
			{
				for (int n = 0; n < IM_ARRAYSIZE(items); n++)
				{
					const bool is_selected = (item_selected_idx == n);
					if (ImGui::Selectable(items[n], is_selected))
						item_selected_idx = n;

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				mLayerType[layerIdx] = (RenderLayer) item_selected_idx;
				ImGui::EndListBox();
			}

			ImGui::SliderInt("Stencli [0, 7]", &mLayerStencil[layerIdx], 0, 7, "%d", flags);
			ImGui::SliderInt("Constant Buffer [0, 1]", &mLayerCBIdx[layerIdx], 0, 1, "%d", flags);
		}
		ImGui::TreePop();
	}


	ImGui::End();
}

void MyApp::MakePSOCheckbox(const std::string type, bool& flag, bool& flag2)
{
	std::string temp = "Draw " + type;
	ImGui::TableNextColumn(); ImGui::Checkbox(temp.c_str(), &flag);
	ImGui::TableNextColumn();
	temp = type + "Wireframe";

	if (flag) {
		ImGui::Checkbox(temp.c_str(), &flag2);
	}
	else {
		ImGui::BeginDisabled();
		ImGui::Checkbox(temp.c_str(), &flag2);
		ImGui::EndDisabled();
	}
}

void MyApp::ShowTextureWindow()
{
	ImGui::Begin("texture", &mShowTextureWindow);

	static int texIdx;

	ImGuiSliderFlags flags = ImGuiSliderFlags_None & ~ImGuiSliderFlags_WrapAround;
	ImGui::SliderInt((std::string("Texture [0, ") + std::to_string(SRV_IMGUI_SIZE + TEXTURE_FILENAMES.size() + SRV_USER_SIZE + mCSBlurFilter->DescriptorCount() + mCSWaves->DescriptorCount()) + "]").c_str(), &texIdx, 0, SRV_IMGUI_SIZE + TEXTURE_FILENAMES.size() + SRV_USER_SIZE + mCSBlurFilter->DescriptorCount() + mCSWaves->DescriptorCount() - 1, "%d", flags);
	ImTextureID my_tex_id = (ImTextureID)mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr + mCbvSrvUavDescriptorSize * texIdx;
	ImGui::Text("GPU handle = %p", my_tex_id);
	{
		ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
		ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
		ImVec4 tint_col = true ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // No tint
		ImVec4 border_col = ImGui::GetStyleColorVec4(ImGuiCol_Border);
		ImGui::Image(my_tex_id, ImVec2(mImguiWidth, mImguiHeight), uv_min, uv_max, tint_col, border_col);
	}
	ImGui::End();
}

void MyApp::ShowMaterialWindow()
{
	ImGui::Begin("material", &mShowMaterialWindow);

	static int matIdx;
	static float diff4f[4];
	static float fres3f[3];

	ImGui::LabelText("label", "Value");
	ImGui::SeparatorText("Inputs");
	ImGuiSliderFlags flags = ImGuiSliderFlags_None & ~ImGuiSliderFlags_WrapAround;
	ImGui::SliderInt((std::string("Material [0, ") + std::to_string(MATERIAL_NAMES.size() - 1) + "]").c_str(), &matIdx, 0, MATERIAL_NAMES.size() - 1, "%d", flags);
	{
		int flag = 0;
		Material* mat = mMaterials[MATERIAL_NAMES[matIdx]].get();

		// flag += ImGui::InputFloat4("DiffuseAlbedo R/G/B/A", diff4f);
		flag += ImGui::DragFloat4("DiffuseAlbedo R/G/B/A", diff4f, 0.01f, 0.0f, 1.0f);
		flag += ImGui::DragFloat3("Fresne R/G/B", fres3f, 0.01f, 0.0f, 1.0f);
		flag += ImGui::DragFloat("Roughness", &mat->Roughness, 0.01f, 0.0f, 1.0f);
		flag += ImGui::SliderInt((std::string("Texture Index [0, ") + std::to_string(SRV_IMGUI_SIZE + TEXTURE_FILENAMES.size() + SRV_USER_SIZE + mCSBlurFilter->DescriptorCount() + mCSWaves->DescriptorCount()) + "]").c_str(), &mat->DiffuseSrvHeapIndex, 0, SRV_IMGUI_SIZE + TEXTURE_FILENAMES.size() + SRV_USER_SIZE + +mCSBlurFilter->DescriptorCount() + mCSWaves->DescriptorCount() - 1, "%d", flags);
		if (flag)
		{
			mat->DiffuseAlbedo = { diff4f[0],diff4f[1],diff4f[2],diff4f[3] };
			mat->FresnelR0 = { fres3f[0], fres3f[1], fres3f[2] };
			mat->NumFramesDirty = APP_NUM_FRAME_RESOURCES;
		}

		ImTextureID my_tex_id = (ImTextureID)mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr + mCbvSrvUavDescriptorSize * mat->DiffuseSrvHeapIndex;
		ImVec4 tint_col = true ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // No tint
		ImVec4 border_col = ImGui::GetStyleColorVec4(ImGuiCol_Border);
		ImGui::Image(my_tex_id, ImVec2(mImguiWidth, mImguiHeight), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), tint_col, border_col);
	}
	ImGui::End();
}

void MyApp::ShowRenderItemWindow()
{
	ImGui::Begin("render item", &mShowRenderItemWindow);
	static int ritmIdx;

	static float world3f[3];
	static float scale3f[3];
	static float angle3f[3];

	RenderItem* ritm = mAllRitems[ritmIdx].get();

	ImGui::LabelText("label", "Value");
	ImGui::SeparatorText("Inputs");
	ImGuiSliderFlags flags = ImGuiSliderFlags_None & ~ImGuiSliderFlags_WrapAround;

	int flag = 0;
	flag += ImGui::SliderInt((std::string("Render Items [0, ") + std::to_string(mAllRitems.size() - 1) + "]").c_str(), &ritmIdx, 0, mAllRitems.size() - 1, "%d", flags);

	//const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD", "EEEE", "FFFF", "GGGG", "HHHH", "IIII", "JJJJ", "KKKK", "LLLLLLL", "MMMM", "OOOOOOO" };
	//static int item_selected_idx = 0; // Here we store our selected data as an index.
	//ImGui::Text("Full-width:");
	//if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing())))
	//{
	//	for (int n = 0; n < mAllRitems.size(); n++)
	//	{
	//		bool is_selected = (item_selected_idx == n);
	//		if (ImGui::Selectable(items[n], is_selected))
	//			item_selected_idx = n;

	//		// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
	//		if (is_selected)
	//			ImGui::SetItemDefaultFocus();
	//	}
	//	ImGui::EndListBox();
	//}
	if (flag)
	{
		ritm = mAllRitems[ritmIdx].get();
		world3f[0] = ritm->WorldX;
		world3f[1] = ritm->WorldY;
		world3f[2] = ritm->WorldZ;
		scale3f[0] = ritm->ScaleX;
		scale3f[1] = ritm->ScaleY;
		scale3f[2] = ritm->ScaleZ;
		angle3f[0] = ritm->AngleX;
		angle3f[1] = ritm->AngleY;
		angle3f[2] = ritm->AngleZ;
	};
	{
		flag = 0;

		flag += ImGui::DragFloat3("world x/y/z", world3f, 0.01f, -FLT_MAX / 2, FLT_MAX / 2);
		flag += ImGui::DragFloat3("scale x/y/z", scale3f, 0.01f, -FLT_MAX / 2, FLT_MAX / 2);
		flag += ImGui::DragFloat3("angle x/y/z", angle3f, 0.01f, -FLT_MAX / 2, FLT_MAX / 2);

		if (flag)
		{
			ritm->NumFramesDirty = APP_NUM_FRAME_RESOURCES;
		}
	}

	ImGui::End();
}

void MyApp::ShowViewportWindow()
{
	ImGui::Begin("viewport1", &mShowViewportWindow);

	ImTextureID my_tex_id = (ImTextureID)mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr + mCbvSrvUavDescriptorSize * (SRV_IMGUI_SIZE + TEXTURE_FILENAMES.size());
	{
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
