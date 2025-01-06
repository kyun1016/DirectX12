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

	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildShapeGeometry();
	BuildSkullGeometry();
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

void MyApp::BuildRootSignature()
{
	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[2];

	// Create root CBVs.
	slotRootParameter[0].InitAsConstantBufferView(0);	// obj CB
	slotRootParameter[1].InitAsConstantBufferView(1);	// pass CB
	slotRootParameter[2].InitAsConstantBufferView(2);	// pass CB

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf()));

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	ThrowIfFailed(mDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void MyApp::BuildShadersAndInputLayout()
{
	mShaders[VS_NAME] = D3DUtil::LoadBinary(L"Shaders\\MainVS.cso");
	mShaders[PS_NAME] = D3DUtil::LoadBinary(L"Shaders\\MainPS.cso");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void MyApp::BuildShapeGeometry()
{
	//=========================================================
	// Part 1. Mesh 생성
	//=========================================================
	std::vector<GeometryGenerator::MeshData> meshes(NUM_MAIN_MESHES);
	meshes[0] = GeometryGenerator::CreateSphere(5.0f, 20, 20);
	meshes[1] = GeometryGenerator::CreateGrid(160.0f, 160.0f, 50, 50);

	//=========================================================
	// Part 2. Sub Mesh 생성 및 통합 vertices & indices 생성
	//=========================================================
	size_t totalVertexCount = 0;
	for (const auto& a : meshes)
		totalVertexCount += a.Vertices.size();

	std::vector<SubmeshGeometry> submeshes(NUM_MAIN_MESHES);
	std::vector<Vertex> vertices(totalVertexCount);
	std::vector<std::uint16_t> indices;
	UINT k = 0;
	for (size_t i = 0; i < NUM_MAIN_MESHES; ++i)
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
		for (size_t j = 0; j < meshes[i].Vertices.size();++j)
		{
			vertices[k].Pos = meshes[i].Vertices[j].Position;
			vertices[k++].Normal = meshes[i].Vertices[j].Normal;
		}
	}

	//=========================================================
	// Part 3. Indices 할당 (16bit)
	//=========================================================
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = gMeshGeometryName[0];
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

	for (size_t i = 0; i < NUM_MAIN_MESHES; ++i)
	{
		geo->DrawArgs[gMainMeshName[i]] = submeshes[i];
	}
	mGeometries[geo->Name] = std::move(geo);
}

void MyApp::BuildSkullGeometry()
{
	std::ifstream fin("Models/skull.txt");

	if (!fin)
	{
		MessageBox(0, L"Models/skull.txt not found.", 0, 0);
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
	geo->Name = gMeshGeometryName[1];

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

	geo->DrawArgs[gSubMeshName[0]] = submesh;

	mGeometries[geo->Name] = std::move(geo);
}

void MyApp::BuildMaterials()
{
	auto bricks0 = std::make_unique<Material>();
	bricks0->Name = gMaterialName[0];
	bricks0->MatCBIndex = 0;
	bricks0->DiffuseSrvHeapIndex = 0;
	bricks0->DiffuseAlbedo = DirectX::XMFLOAT4(DirectX::Colors::ForestGreen);
	bricks0->FresnelR0 = DirectX::XMFLOAT3(0.02f, 0.02f, 0.02f);
	bricks0->Roughness = 0.1f;

	auto stone0 = std::make_unique<Material>();
	stone0->Name = gMaterialName[1];
	stone0->MatCBIndex = 1;
	stone0->DiffuseSrvHeapIndex = 1;
	stone0->DiffuseAlbedo = DirectX::XMFLOAT4(DirectX::Colors::LightSteelBlue);
	stone0->FresnelR0 = DirectX::XMFLOAT3(0.05f, 0.05f, 0.05f);
	stone0->Roughness = 0.3f;

	auto tile0 = std::make_unique<Material>();
	tile0->Name = gMaterialName[2];
	tile0->MatCBIndex = 2;
	tile0->DiffuseSrvHeapIndex = 2;
	tile0->DiffuseAlbedo = DirectX::XMFLOAT4(DirectX::Colors::LightGray);
	tile0->FresnelR0 = DirectX::XMFLOAT3(0.02f, 0.02f, 0.02f);
	tile0->Roughness = 0.2f;

	auto skullMat = std::make_unique<Material>();
	skullMat->Name = gMaterialName[3];
	skullMat->MatCBIndex = 3;
	skullMat->DiffuseSrvHeapIndex = 3;
	skullMat->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	skullMat->FresnelR0 = DirectX::XMFLOAT3(0.05f, 0.05f, 0.05);
	skullMat->Roughness = 0.3f;

	mMaterials[gMaterialName[0]] = std::move(bricks0);
	mMaterials[gMaterialName[1]] = std::move(stone0);
	mMaterials[gMaterialName[2]] = std::move(tile0);
	mMaterials[gMaterialName[3]] = std::move(skullMat);
}


void MyApp::BuildRenderItems()
{
	auto boxRitem = std::make_unique<RenderItem>();
	DirectX::XMStoreFloat4x4(&boxRitem->World, DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f) * DirectX::XMMatrixTranslation(10.0f, 10.0f, 10.0f));
	boxRitem->ObjCBIndex = 0;
	boxRitem->Geo = mGeometries[gMeshGeometryName[0]].get();
	boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs[gSubmeshName[0]].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs[gSubmeshName[0]].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs[gSubmeshName[0]].BaseVertexLocation;
	mAllRitems.push_back(std::move(boxRitem));

	auto gridRitem = std::make_unique<RenderItem>();
	gridRitem->World = MathHelper::Identity4x4();
	gridRitem->ObjCBIndex = 1;
	gridRitem->Geo = mGeometries[gMeshGeometryName[0]].get();
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs[gSubmeshName[1]].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs[gSubmeshName[1]].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs[gSubmeshName[1]].BaseVertexLocation;
	mAllRitems.push_back(std::move(gridRitem));

	auto wavesRitem = std::make_unique<RenderItem>();
	wavesRitem->World = MathHelper::Identity4x4();
	wavesRitem->ObjCBIndex = 2;
	wavesRitem->Geo = mGeometries[gMeshGeometryName[1]].get();
	wavesRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wavesRitem->IndexCount = wavesRitem->Geo->DrawArgs[gSubmeshName[2]].IndexCount;
	wavesRitem->StartIndexLocation = wavesRitem->Geo->DrawArgs[gSubmeshName[2]].StartIndexLocation;
	wavesRitem->BaseVertexLocation = wavesRitem->Geo->DrawArgs[gSubmeshName[2]].BaseVertexLocation;
	mAllRitems.push_back(std::move(wavesRitem));

	// All the render items are opaque.
	for (auto& e : mAllRitems)
		mRitemLayer[(int)RenderLayer::Opaque].push_back(e.get());
}

void MyApp::BuildFrameResources()
{
	for (int i = 0; i < APP_NUM_FRAME_RESOURCES; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(mDevice.Get(), 1, (UINT)mAllRitems.size(), mWaves->VertexCount()));
	}
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
		/* D3D12_RASTERIZER_DESC RasterizerState{							*/.RasterizerState = {
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
		/* D3D12_DEPTH_STENCIL_DESC DepthStencilState						*/.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
		/* D3D12_INPUT_LAYOUT_DESC InputLayout{								*/.InputLayout = {
		/*		const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs			*/		mInputLayout.data(),
		/*		UINT NumElements											*/		(UINT)mInputLayout.size()
		/*	}																*/ },
		/* D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue				*/.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
		/* D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType				*/.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		/* UINT NumRenderTargets											*/.NumRenderTargets = 1,
		/* DXGI_FORMAT RTVFormats[8]										*/.RTVFormats = {mBackBufferFormat, DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN},	// 0
		/* DXGI_FORMAT DSVFormat											*/.DSVFormat = mDepthStencilFormat,
		/* DXGI_SAMPLE_DESC SampleDesc{										*/.SampleDesc = {
		/*		UINT Count;													*/		m4xMsaaState ? 4u : 1u,
		/*		UINT Quality;												*/		m4xMsaaState ? (m4xMsaaQuality - 1) : 0
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
#pragma endregion Initialize

#pragma region Update
float MyApp::GetHillsHeight(float x, float z) const
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}
DirectX::XMFLOAT3 MyApp::GetHillsNormal(float x, float z) const
{
	// n = (-df/dx, 1, -df/dz)
	DirectX::XMFLOAT3 n(
		-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
		1.0f,
		-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

	DirectX::XMVECTOR unitNormal = DirectX::XMVector3Normalize(XMLoadFloat3(&n));
	DirectX::XMStoreFloat3(&n, unitNormal);

	return n;
}
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

	UpdateObjectCBs();
	UpdateMainPassCB();
	UpdateWaves();
}

void MyApp::UpdateObjectCBs()
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : mAllRitems)
	{
		// 상수들이 바뀌었을 때만 cbuffer 자료를 갱신
		// 갱신은 Frame 마다 적용
		if (e->NumFramesDirty > 0)
		{
			DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&e->World);

			ObjectConstants objConstants;
			DirectX::XMStoreFloat4x4(&objConstants.World, DirectX::XMMatrixTranspose(world));

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// 다음 프레임 자원으로 넘어간다.
			e->NumFramesDirty--;
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
		v.Color = DirectX::XMFLOAT4(DirectX::Colors::Blue);

		currWavesVB->CopyData(i, v);
	}

	// Set the dynamic VB of the wave renderitem to the current frame VB.
	mAllRitems[2]->Geo->VertexBufferGPU = currWavesVB->Resource();
}
#pragma endregion Update

void MyApp::Render()
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	if (mIsWireframe)
	{
		ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs[gPSOName[1]].Get()));
	}
	else
	{
		ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs[gPSOName[0]].Get()));
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

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	// Bind per-pass constant buffer.  We only need to do this once per-pass.
	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

	DrawRenderItems(mRitemLayer[(int)RenderLayer::Opaque]);

	// Indicate a state transition on the resource usage.
	D3D12_RESOURCE_BARRIER DefaultBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mSwapChainBuffer[mCurrBackBuffer].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	mCommandList->ResourceBarrier(1, &DefaultBarrier);

	mCommandList->SetDescriptorHeaps(1, &mSrvDescHeap);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	//====================================
	// Imgui
	//====================================
	// Update and Render additional Platform Windows
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

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
	auto objectCB = mCurrFrameResource->ObjectCB->Resource();

	// For each render item...
	for (const auto& ri: ritems)
	{
		D3D12_VERTEX_BUFFER_VIEW vbv = ri->Geo->VertexBufferView();
		D3D12_INDEX_BUFFER_VIEW ibv = ri->Geo->IndexBufferView();
		mCommandList->IASetVertexBuffers(0, 1, &vbv);
		mCommandList->IASetIndexBuffer(&ibv);
		mCommandList->IASetPrimitiveTopology(ri->PrimitiveType);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress();
		objCBAddress += ri->ObjCBIndex * objCBByteSize;

		mCommandList->SetGraphicsRootConstantBufferView(0, objCBAddress);

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
