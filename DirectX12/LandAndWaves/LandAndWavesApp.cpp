#include "pch.h"
#include "LandAndWavesApp.h"

LandAndWavesApp::LandAndWavesApp()
	: AppBase()
{
}

LandAndWavesApp::LandAndWavesApp(uint32_t width, uint32_t height, std::wstring name)
	: AppBase(width, height, name)
{
}
#pragma region Initialize
bool LandAndWavesApp::Initialize()
{
	if (!AppBase::Initialize())
		return false;

	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildShapeGeometry();
	BuildRenderItems();
	BuildFrameResources();
	BuildDescriptorHeaps();
	BuildConstantBufferViews();
	BuildPSO();

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	return true;
}

void LandAndWavesApp::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE cbvTable0;
	cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE cbvTable1;
	cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[2];

	// Create root CBVs.
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
	slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(mDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void LandAndWavesApp::BuildShadersAndInputLayout()
{
	mShaders[VS_NAME] = D3DUtil::LoadBinary(L"Shaders\\Shapes_VS.cso");
	mShaders[PS_NAME] = D3DUtil::LoadBinary(L"Shaders\\Shapes_PS.cso");

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void LandAndWavesApp::BuildShapeGeometry()
{
	// 기존
	//GeometryGenerator::MeshData box = GeometryGenerator::CreateBox(1.5f, 0.5f, 1.5f, 3);
	//GeometryGenerator::MeshData grid = GeometryGenerator::CreateGrid(20.0f, 30.0f, 60, 40);
	//GeometryGenerator::MeshData sphere = GeometryGenerator::CreateSphere(0.5f, 20, 20);
	//GeometryGenerator::MeshData cylinder = GeometryGenerator::CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

	//UINT boxVertexOffset = 0;
	//UINT gridVertexOffset = (UINT)box.Indices32.size();
	//UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Indices32.size();
	//UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Indices32.size();

	// 기존
	//UINT vertexOffsets[4];
	//vertexOffsets[0] = 0;
	//vertexOffsets[1] = vertexOffsets[0] + (UINT)meshes[0].Vertices.size();
	//vertexOffsets[2] = vertexOffsets[1] + (UINT)meshes[1].Vertices.size();
	//vertexOffsets[3] = vertexOffsets[2] + (UINT)meshes[2].Vertices.size();

	//UINT indexOffsets[4];
	//indexOffsets[0] = 0;
	//indexOffsets[1] = indexOffsets[0] + (UINT)meshes[0].Indices32.size();
	//indexOffsets[2] = indexOffsets[1] + (UINT)meshes[1].Indices32.size();
	//indexOffsets[3] = indexOffsets[2] + (UINT)meshes[2].Indices32.size();

	//SubmeshGeometry submeshes[4];
	//submeshes[0].IndexCount = (UINT)meshes[0].Indices32.size();
	//submeshes[1].IndexCount = (UINT)meshes[1].Indices32.size();
	//submeshes[2].IndexCount = (UINT)meshes[2].Indices32.size();
	//submeshes[3].IndexCount = (UINT)meshes[3].Indices32.size();

	//submeshes[0].StartIndexLocation = indexOffsets[0];
	//submeshes[1].StartIndexLocation = indexOffsets[1];
	//submeshes[2].StartIndexLocation = indexOffsets[2];
	//submeshes[3].StartIndexLocation = indexOffsets[3];

	//submeshes[0].BaseVertexLocation = vertexOffsets[0];
	//submeshes[1].BaseVertexLocation = vertexOffsets[1];
	//submeshes[2].BaseVertexLocation = vertexOffsets[2];
	//submeshes[3].BaseVertexLocation = vertexOffsets[3];

	//=========================================================
	// Part 1. Mesh 생성
	//=========================================================
	std::vector<GeometryGenerator::MeshData> meshes(NUM_MESHES);
	meshes[0] = GeometryGenerator::CreateBox(1.5f, 0.5f, 1.5f, 3);
	meshes[1] = GeometryGenerator::CreateGrid(20.0f, 30.0f, 60, 40);
	meshes[2] = GeometryGenerator::CreateSphere(0.5f, 20, 20);
	meshes[3] = GeometryGenerator::CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

	//=========================================================
	// Part 2. Sub Mesh 생성 및 통합 vertices & indices 생성
	//=========================================================
	std::vector<SubmeshGeometry> submeshes(NUM_MESHES);
	std::vector<GeometryGenerator::Vertex> vertices;
	std::vector<std::uint16_t> indices;

	for (size_t i = 0; i < NUM_MESHES; ++i)
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
		//for (size_t j = 0; j < meshes[i].Vertices.size(); ++j)
		//{
		//	vertices.push_back(meshes[i].Vertices[j]);
		//}
		vertices.insert(vertices.end(), meshes[i].Vertices.begin(), meshes[i].Vertices.end());
		indices.insert(indices.end(), meshes[i].GetIndices16().begin(), meshes[i].GetIndices16().end());
	}

	//=========================================================
	// Part 3. Indices 할당 (16bit)
	//=========================================================
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(GeometryGenerator::Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = gMeshGeometryName;
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
	geo->VertexBufferGPU = D3DUtil::CreateDefaultBuffer(mDevice.Get(), mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(mDevice.Get(), mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(GeometryGenerator::Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	for (size_t i = 0; i < NUM_MESHES; ++i)
	{
		geo->DrawArgs[gSubmeshName[i]] = submeshes[i];
	}
	mGeometries[geo->Name] = std::move(geo);
}

void LandAndWavesApp::BuildRenderItems()
{
	auto boxRitem = std::make_unique<RenderItem>();
	DirectX::XMStoreFloat4x4(&boxRitem->World, DirectX::XMMatrixScaling(2.0f, 2.0f, 2.0f) * DirectX::XMMatrixTranslation(0.0f, 0.5f, 0.0f));
	boxRitem->ObjCBIndex = 0;
	boxRitem->Geo = mGeometries[gMeshGeometryName].get();
	boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs[gSubmeshName[0]].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs[gSubmeshName[0]].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs[gSubmeshName[0]].BaseVertexLocation;
	mAllRitems.push_back(std::move(boxRitem));

	auto gridRitem = std::make_unique<RenderItem>();
	gridRitem->World = MathHelper::Identity4x4();
	gridRitem->ObjCBIndex = 1;
	gridRitem->Geo = mGeometries[gMeshGeometryName].get();
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs[gSubmeshName[1]].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs[gSubmeshName[1]].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs[gSubmeshName[1]].BaseVertexLocation;
	mAllRitems.push_back(std::move(gridRitem));

	UINT objCBIndex = 2;
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

		XMStoreFloat4x4(&leftCylRitem->World, rightCylWorld);
		leftCylRitem->ObjCBIndex = objCBIndex++;
		leftCylRitem->Geo = mGeometries[gMeshGeometryName].get();
		leftCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs[gSubmeshName[3]].IndexCount;
		leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs[gSubmeshName[3]].StartIndexLocation;
		leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs[gSubmeshName[3]].BaseVertexLocation;

		XMStoreFloat4x4(&rightCylRitem->World, leftCylWorld);
		rightCylRitem->ObjCBIndex = objCBIndex++;
		rightCylRitem->Geo = mGeometries[gMeshGeometryName].get();
		rightCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs[gSubmeshName[3]].IndexCount;
		rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs[gSubmeshName[3]].StartIndexLocation;
		rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs[gSubmeshName[3]].BaseVertexLocation;

		XMStoreFloat4x4(&leftSphereRitem->World, leftSphereWorld);
		leftSphereRitem->ObjCBIndex = objCBIndex++;
		leftSphereRitem->Geo = mGeometries[gMeshGeometryName].get();
		leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs[gSubmeshName[2]].IndexCount;
		leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs[gSubmeshName[2]].StartIndexLocation;
		leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs[gSubmeshName[2]].BaseVertexLocation;

		XMStoreFloat4x4(&rightSphereRitem->World, rightSphereWorld);
		rightSphereRitem->ObjCBIndex = objCBIndex++;
		rightSphereRitem->Geo = mGeometries[gMeshGeometryName].get();
		rightSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRitem->IndexCount = rightSphereRitem->Geo->DrawArgs[gSubmeshName[2]].IndexCount;
		rightSphereRitem->StartIndexLocation = rightSphereRitem->Geo->DrawArgs[gSubmeshName[2]].StartIndexLocation;
		rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geo->DrawArgs[gSubmeshName[2]].BaseVertexLocation;

		mAllRitems.push_back(std::move(leftCylRitem));
		mAllRitems.push_back(std::move(rightCylRitem));
		mAllRitems.push_back(std::move(leftSphereRitem));
		mAllRitems.push_back(std::move(rightSphereRitem));
	}

	// All the render items are opaque.
	for (auto& e : mAllRitems)
		mOpaqueRitems.push_back(e.get());
}

void LandAndWavesApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(mDevice.Get(), 1, (UINT)mAllRitems.size()));
	}
}

void LandAndWavesApp::BuildDescriptorHeaps()
{
	UINT objCount = (UINT)mOpaqueRitems.size();

	// Need a CBV descriptor for each object for each frame resource,
	// +1 for the perPass CBV for each frame resource.
	UINT numDescriptors = (objCount + 1) * gNumFrameResources;

	// Save an offset to the start of the pass CBVs.  These are the last 3 descriptors.
	mPassCbvOffset = objCount * gNumFrameResources;

	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc
	{
		/* D3D12_DESCRIPTOR_HEAP_TYPE Type	*/D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		/* UINT NumDescriptors				*/numDescriptors,
		/* D3D12_DESCRIPTOR_HEAP_FLAGS Flags*/D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		/* UINT NodeMask					*/0
	};
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));
}

void LandAndWavesApp::BuildConstantBufferViews()
{
	UINT objCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	UINT objCount = (UINT)mOpaqueRitems.size();

	// Need a CBV descriptor for each object for each frame resource.
	int heapIndex = 0;
	for (int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
	{
		auto objectCB = mFrameResources[frameIndex]->ObjectCB->Resource();
		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = objectCB->GetGPUVirtualAddress();
		for (UINT i = 0; i < objCount; ++i)
		{
			auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(heapIndex++, mCbvSrvUavDescriptorSize);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc
			{
				/* D3D12_GPU_VIRTUAL_ADDRESS BufferLocation	*/cbAddress,
				/* UINT SizeInBytes							*/objCBByteSize
			};
			mDevice->CreateConstantBufferView(&cbvDesc, handle);

			// Offset to the ith object constant buffer in the buffer.
			cbAddress += objCBByteSize;
		}
	}

	UINT passCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	// Last three descriptors are the pass CBVs for each frame resource.
	for (int frameIndex = 0; frameIndex < gNumFrameResources; ++frameIndex)
	{
		auto passCB = mFrameResources[frameIndex]->PassCB->Resource();
		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();

		// Offset to the pass cbv in the descriptor heap.
		int heapIndex = mPassCbvOffset + frameIndex;
		auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc
		{
			/* D3D12_GPU_VIRTUAL_ADDRESS BufferLocation	*/cbAddress,
			/* UINT SizeInBytes							*/passCBByteSize
		};

		mDevice->CreateConstantBufferView(&cbvDesc, handle);
	}
}

void LandAndWavesApp::BuildPSO()
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
void LandAndWavesApp::OnResize()
{
	AppBase::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, mAspectRatio, 1.0f, 1000.0f);
	DirectX::XMStoreFloat4x4(&mProj, P);
}
void LandAndWavesApp::Update()
{
	OnKeyboardInput();
	UpdateCamera();

	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, mFenceEvent));
		WaitForSingleObject(mFenceEvent, INFINITE);
	}

	UpdateObjectCBs();
	UpdateMainPassCB();
}

void LandAndWavesApp::UpdateObjectCBs()
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

void LandAndWavesApp::UpdateMainPassCB()
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
#pragma endregion Update

void LandAndWavesApp::Render()
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

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	int passCbvIndex = mPassCbvOffset + mCurrFrameResourceIndex;
	auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	passCbvHandle.Offset(passCbvIndex, mCbvSrvUavDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);

	DrawRenderItems(mOpaqueRitems);

	// Indicate a state transition on the resource usage.
	D3D12_RESOURCE_BARRIER DefaultBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mSwapChainBuffer[mCurrBackBuffer].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	mCommandList->ResourceBarrier(1, &DefaultBarrier);

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

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

void LandAndWavesApp::DrawRenderItems(const std::vector<RenderItem*>& ritems)
{
	// UINT objCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	// auto objectCB = mCurrFrameResource->ObjectCB->Resource();

	// For each render item...
	for (size_t i = 0; i < ritems.size(); ++i)
	{
		auto ri = ritems[i];

		D3D12_VERTEX_BUFFER_VIEW vbv = ri->Geo->VertexBufferView();
		D3D12_INDEX_BUFFER_VIEW ibv = ri->Geo->IndexBufferView();
		mCommandList->IASetVertexBuffers(0, 1, &vbv);
		mCommandList->IASetIndexBuffer(&ibv);
		mCommandList->IASetPrimitiveTopology(ri->PrimitiveType);

		// Offset to the CBV in the descriptor heap for this object and for this frame resource.
		UINT cbvIndex = mCurrFrameResourceIndex * (UINT)mOpaqueRitems.size() + ri->ObjCBIndex;
		auto cbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
		cbvHandle.Offset(cbvIndex, mCbvSrvUavDescriptorSize);

		mCommandList->SetGraphicsRootDescriptorTable(0, cbvHandle);

		mCommandList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

void LandAndWavesApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mHwndWindow);
}

void LandAndWavesApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void LandAndWavesApp::OnMouseMove(WPARAM btnState, int x, int y)
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

void LandAndWavesApp::OnKeyboardInput()
{
	if (GetAsyncKeyState('1') & 0x8000)
		mIsWireframe = true;
	else
		mIsWireframe = false;
}

void LandAndWavesApp::UpdateCamera()
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
