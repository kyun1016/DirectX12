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
	for (int i = 0; i < MAX_LAYER_DEPTH; ++i)
	{
		mLayerType[i] = RenderLayer::None;
		mLayerStencil[i] = 0;
		mLayerCBIdx[i] = 0;
	}

	mLayerType[0] = RenderLayer::Opaque;
	// mLayerType[1] = RenderLayer::SkinnedOpaque;
	// mLayerType[2] = RenderLayer::AlphaTested;
	// mLayerType[3] = RenderLayer::Transparent;
	// mLayerType[4] = RenderLayer::Subdivision;
	// mLayerType[5] = RenderLayer::TreeSprites;
	mLayerType[1] = RenderLayer::CubeMap;
	//mLayerType[7] = RenderLayer::Normal;
	//mLayerType[8] = RenderLayer::SkinnedNormal;
	// mLayerType[17] = RenderLayer::ShadowMap;
	// mLayerType[18] = RenderLayer::WaveCS;
	// mLayerType[19] = RenderLayer::BlurCS;

	{
		// 거울 반사 구현 시
		// 1. 바라보는 시점관련 Constant buffer 구현
		// 2. 스텐실 버퍼 설정
		// 3. 물체의 월드좌표 시점에 맞춰 반전 후 InstanceData 저장 (현재는 Offset을 두어 접근하는 인덱스를 키우는 방식으로 활용 가능)
		// 4. 시점 버퍼 설정 + 스텐실 버퍼 설정 + 물체의 월드 좌표 오프셋 설정 후 렌더링 시 거울 반사 구현 완료
		// mLayerType[2] = RenderLayer::Mirror;
		// mLayerType[3] = RenderLayer::Reflected;
		// mLayerStencil[2] = 1;
		// mLayerStencil[3] = 1;
		// mLayerCBIdx[2] = 1;
		// mLayerCBIdx[3] = 1;
	}
}
MyApp::~MyApp()
{
	if (mDevice != nullptr)
		FlushCommandQueue();
}
#pragma region Initialize
bool MyApp::Initialize()
{
	if (!AppBase::Initialize())
		return false;

	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), nullptr));

	mCamera.SetPosition(0.0f, 2.0f, -15.0f);
	mCSBlurFilter = std::make_unique<BlurFilter>(mDevice.Get(), mClientWidth, mClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
	mCSAdd = std::make_unique<CSAdd>(mDevice.Get(), mCommandList.Get());
	mCSWaves = std::make_unique<GpuWaves>(mDevice.Get(), mCommandList.Get(), 256, 256, 0.25f, 0.03f, 2.0f, 0.2f);
	{
		for (int i = 0; i < MAX_LIGHTS; ++i)
		{
			mShadowMap[i] = std::make_unique<ShadowMap>(mDevice.Get(), 2048, 2048);
		}
		mShadowMap[0]->SetBaseDir({ 0.57735f, -0.57735f, 0.57735f });
		mShadowMap[1]->SetBaseDir({ -0.57735f, -0.57735f, 0.57735f });
		mShadowMap[2]->SetBaseDir({ 0.0f, -0.707f, -0.707f });

		mShadowMap[0]->SetBoxLength({ 100.0f });
		mShadowMap[1]->SetBoxLength({ 100.0f });
		mShadowMap[2]->SetBoxLength({ 100.0f });

		mShadowMap[0]->SetTarget({ 0.0f, 5.0f, 0.0f });
		mShadowMap[1]->SetTarget({ 0.0f, 5.0f, 0.0f });
		mShadowMap[2]->SetTarget({ 0.0f, 5.0f, 0.0f });
	}
	// mSsaoMap = std::make_unique<SsaoMap>(mDevice.Get(), mCommandList.Get(), mClientWidth, mClientHeight);

	BuildMeshes();
	BuildTreeSpritesGeometry();
	LoadTextures();
	BuildMaterials();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSOs();
	// mSsaoMap->SetPSOs();

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
	std::vector<std::wstring> diffuseFilename = {
		// STD TEX
		L"../Data/Textures/bricks.dds",
		L"../Data/Textures/bricks2.dds",
		L"../Data/Textures/bricks3.dds",
		L"../Data/Textures/checkboard.dds",
		L"../Data/Textures/grass.dds",
		L"../Data/Textures/ice.dds",
		L"../Data/Textures/stone.dds",
		L"../Data/Textures/tile.dds",
		L"../Data/Textures/WireFence.dds",
		L"../Data/Textures/WoodCrate01.dds",
		L"../Data/Textures/WoodCrate02.dds",
		L"../Data/Textures/water1.dds",
		L"../Data/Textures/white1x1.dds",
		L"../Data/Textures/tree01S.dds",
		L"../Data/Textures/tree02S.dds",
		L"../Data/Textures/tree35S.dds"
	};
	std::vector<std::wstring> normalFilename = {
		// STD TEX
		L"../Data/Textures/default_nmap.dds",
		L"../Data/Textures/bricks_nmap.dds",
		L"../Data/Textures/bricks2_nmap.dds",
		L"../Data/Textures/tile_nmap.dds"
	};
	std::vector<std::wstring> arrayFilename = {
		// Array Tex (for Billboard Shader)
		L"../Data/Textures/treearray.dds",
		L"../Data/Textures/treeArray2.dds",
	};
	std::vector<std::wstring>	cubeFilename = {
		// Array Tex (for Billboard Shader)
		L"../Data/Textures/desertcube1024.dds",
		L"../Data/Textures/grasscube1024.dds",
		L"../Data/Textures/snowcube1024.dds",
		L"../Data/Textures/sunsetcube1024.dds",
	};

	for (const auto& data : mSkinnedMats)
	{
		std::wstring diffuseFile = L"../Data/Textures/" + StrToWStr(data.DiffuseMapName);
		std::wstring normalFile = L"../Data/Textures/" + StrToWStr(data.NormalMapName);

		diffuseFilename.push_back(diffuseFile);
		normalFilename.push_back(normalFile);
	}

	LoadTextures(diffuseFilename, mDiffuseTex);
	LoadTextures(normalFilename, mNormalTex);
	LoadTextures(arrayFilename, mTreeMapTex);
	LoadTextures(cubeFilename, mCubeMapTex);
}

void MyApp::LoadTextures(const std::vector<std::wstring>& filename, std::unordered_map<std::wstring, std::unique_ptr<Texture>>& texMap)
{
	for (const auto& name : filename)
	{
		if (texMap.find(name) == std::end(texMap))
		{
			auto texture = std::make_unique<Texture>();
			texture->Filename = name;
			ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(mDevice.Get(), mCommandList.Get(), name.c_str(), texture->Resource, texture->UploadHeap));

			texMap[name] = std::move(texture);
		}
	}
}

void MyApp::BuildRootSignature()
{
	// Create root CBVs.
	D3D12_DESCRIPTOR_RANGE TexDiffTable // register t0[16] (Space1)
	{
		/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		/* UINT NumDescriptors						*/.NumDescriptors = (UINT) mDiffuseTex.size() + SRV_USER_SIZE,
		/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		/* UINT RegisterSpace						*/.RegisterSpace = 1,
		/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};
	D3D12_DESCRIPTOR_RANGE DisplacementMapTable // register t0 (Space2)
	{
		/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		/* UINT NumDescriptors						*/.NumDescriptors = 1,
		/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		/* UINT RegisterSpace						*/.RegisterSpace = 2,
		/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};
	D3D12_DESCRIPTOR_RANGE TexNormTable // register t0[16] (Space3)
	{
		/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		/* UINT NumDescriptors						*/.NumDescriptors = (UINT)mNormalTex.size(),
		/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		/* UINT RegisterSpace						*/.RegisterSpace = 3,
		/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};
	D3D12_DESCRIPTOR_RANGE TexAOTable // register t0 (Space4)
	{
		/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		/* UINT NumDescriptors						*/.NumDescriptors = 1,
		/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		/* UINT RegisterSpace						*/.RegisterSpace = 4,
		/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};
	D3D12_DESCRIPTOR_RANGE TexMetallicTable // register t0 (Space5)
	{
		/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		/* UINT NumDescriptors						*/.NumDescriptors = 1,
		/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		/* UINT RegisterSpace						*/.RegisterSpace = 5,
		/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};
	D3D12_DESCRIPTOR_RANGE TexRoughnessTable // register t0 (Space6)
	{
		/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		/* UINT NumDescriptors						*/.NumDescriptors = 1,
		/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		/* UINT RegisterSpace						*/.RegisterSpace = 6,
		/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};
	D3D12_DESCRIPTOR_RANGE TexEmissiveTable // register t0 (Space7)
	{
		/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		/* UINT NumDescriptors						*/.NumDescriptors = 1,
		/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		/* UINT RegisterSpace						*/.RegisterSpace = 7,
		/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};
	D3D12_DESCRIPTOR_RANGE ShadowMapTable // register t0[0] (Space8)
	{
		/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		/* UINT NumDescriptors						*/.NumDescriptors = (UINT)MAX_LIGHTS,
		/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		/* UINT RegisterSpace						*/.RegisterSpace = 8,
		/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};
	D3D12_DESCRIPTOR_RANGE SsaoMapTable // register t0[0] (Space9)
	{
		/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		/* UINT NumDescriptors						*/.NumDescriptors = (UINT)MAX_LIGHTS,
		/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		/* UINT RegisterSpace						*/.RegisterSpace = 9,
		/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};
	D3D12_DESCRIPTOR_RANGE TexArrayTable // register t0[0] (Space10)
	{
		/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		/* UINT NumDescriptors						*/.NumDescriptors = (UINT)mTreeMapTex.size(),
		/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		/* UINT RegisterSpace						*/.RegisterSpace = 10,
		/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};
	D3D12_DESCRIPTOR_RANGE TexCubeTable // register t0[0] (Space11)
	{
		/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		/* UINT NumDescriptors						*/.NumDescriptors = (UINT)mCubeMapTex.size(),
		/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		/* UINT RegisterSpace						*/.RegisterSpace = 11,
		/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};
	
	/*D3D12_SHADER_VISIBILITY
	{
		D3D12_SHADER_VISIBILITY_ALL = 0,
		D3D12_SHADER_VISIBILITY_VERTEX = 1,
		D3D12_SHADER_VISIBILITY_HULL = 2,
		D3D12_SHADER_VISIBILITY_DOMAIN = 3,
		D3D12_SHADER_VISIBILITY_GEOMETRY = 4,
		D3D12_SHADER_VISIBILITY_PIXEL = 5,
		D3D12_SHADER_VISIBILITY_AMPLIFICATION = 6,
		D3D12_SHADER_VISIBILITY_MESH = 7
	} 	D3D12_SHADER_VISIBILITY;*/

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[16];
	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsConstantBufferView(0);		// gBaseInstanceIndex b0
	slotRootParameter[1].InitAsConstantBufferView(1);		// cbPass b1
	slotRootParameter[2].InitAsConstantBufferView(2);		// cbSkinned b2
	slotRootParameter[3].InitAsShaderResourceView(0, 0);	// InstanceData t0 (Space0)
	slotRootParameter[4].InitAsShaderResourceView(1, 0);	// MaterialData t1 (Space0)
	slotRootParameter[5].InitAsDescriptorTable(1, &TexDiffTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[6].InitAsDescriptorTable(1, &DisplacementMapTable, D3D12_SHADER_VISIBILITY_VERTEX);
	slotRootParameter[7].InitAsDescriptorTable(1, &TexNormTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[8].InitAsDescriptorTable(1, &TexAOTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[9].InitAsDescriptorTable(1, &TexMetallicTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[10].InitAsDescriptorTable(1, &TexRoughnessTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[11].InitAsDescriptorTable(1, &TexEmissiveTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[12].InitAsDescriptorTable(1, &ShadowMapTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[13].InitAsDescriptorTable(1, &SsaoMapTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[14].InitAsDescriptorTable(1, &TexArrayTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[15].InitAsDescriptorTable(1, &TexCubeTable, D3D12_SHADER_VISIBILITY_PIXEL);

	auto staticSamplers = D3DUtil::GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(16, slotRootParameter, (UINT)staticSamplers.size(),
		staticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(
		&rootSigDesc, 
		D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(),
		errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	ThrowIfFailed(hr);

	ThrowIfFailed(mDevice->CreateRootSignature(0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void MyApp::BuildDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
	// 현재 SRV Heap은 4가지 데이터를 종합하여 관리하고있다.
	// 1. Imgui Data (SRV_IMGUI_SIZE(64))
	// 2. Texture Data (TEX_DIFF_FILENAMES.size())
	// 3. Viewport Buffer (SRV_USER_SIZE)
	// 4. CS Blar Buffer (SRV_CS_BLAR_SIZE(4))
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc
	{
		/* D3D12_DESCRIPTOR_HEAP_TYPE Type	*/.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		/* UINT NumDescriptors				*/.NumDescriptors 
												= SRV_IMGUI_SIZE 
												+ SRV_USER_SIZE 
												+ (UINT)mDisplacementTex.size()
												+ (UINT)mDiffuseTex.size()
												+ (UINT)mNormalTex.size()
												+ (UINT)mAOTex.size()
												+ (UINT)mMetallicTex.size()
												+ (UINT)mRoughnessTex.size()
												+ (UINT)mEmissiveTex.size()
												+ (UINT)mTreeMapTex.size()
												+ (UINT)mCubeMapTex.size()
												+ (UINT)MAX_LIGHTS
												+ mCSBlurFilter->DescriptorCount() 
												+ mCSWaves->DescriptorCount(),
		/* D3D12_DESCRIPTOR_HEAP_FLAGS Flags*/.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		/* UINT NodeMask					*/.NodeMask = 0
	};
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

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

	CD3DX12_CPU_DESCRIPTOR_HANDLE hCPUDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), SRV_IMGUI_SIZE, mCbvSrvUavDescriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGPUDescriptor(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), SRV_IMGUI_SIZE, mCbvSrvUavDescriptorSize);

	{
		//=========================================
		// User Define - SRV_USER_SIZE (Texture2D) -> 1. Render Target, 2. Copy
		//=========================================
		mhGPUUser = hGPUDescriptor;

		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

		for (int i = 0; i < SRV_USER_SIZE; ++i)
		{
			mDevice->CreateShaderResourceView(mSRVUserBuffer[i].Get(), &srvDesc, hCPUDescriptor);
			hCPUDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
			hGPUDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
		}
	}

	mhGPUDiff = hGPUDescriptor;
	BuildTexture2DSrv(mDiffuseTex, srvDesc, hCPUDescriptor, hGPUDescriptor, mCbvSrvUavDescriptorSize);
	mhGPUDisplacement = hGPUDescriptor;
	BuildTexture2DSrv(mDisplacementTex, srvDesc, hCPUDescriptor, hGPUDescriptor, mCbvSrvUavDescriptorSize);
	mhGPUNorm = hGPUDescriptor;
	BuildTexture2DSrv(mNormalTex, srvDesc, hCPUDescriptor, hGPUDescriptor, mCbvSrvUavDescriptorSize);
	mhGPUAO = hGPUDescriptor;
	BuildTexture2DSrv(mAOTex, srvDesc, hCPUDescriptor, hGPUDescriptor, mCbvSrvUavDescriptorSize);
	mhGPUMetallic = hGPUDescriptor;
	BuildTexture2DSrv(mMetallicTex, srvDesc, hCPUDescriptor, hGPUDescriptor, mCbvSrvUavDescriptorSize);
	mhGPURoughness = hGPUDescriptor;
	BuildTexture2DSrv(mRoughnessTex, srvDesc, hCPUDescriptor, hGPUDescriptor, mCbvSrvUavDescriptorSize);
	mhGPUEmissive = hGPUDescriptor;
	BuildTexture2DSrv(mEmissiveTex, srvDesc, hCPUDescriptor, hGPUDescriptor, mCbvSrvUavDescriptorSize);

	{
		//=========================================
		// gShadowMap - MAX_LIGHTS (Texture2D)
		//=========================================
		mhGPUShadow = hGPUDescriptor;
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCPUDSVDescriptor(mhCPUDSVBuffer[1]);
		for (int i = 0; i < MAX_LIGHTS; ++i)
		{
			mShadowMap[i]->BuildDescriptors(
				hCPUDescriptor,
				hGPUDescriptor,
				hCPUDSVDescriptor);
			hCPUDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
			hGPUDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
			hCPUDSVDescriptor.Offset(1, mDsvDescriptorSize);
		}
	}
	//=========================================
	// TODO make gSSAOMap - (Texture2D)
	//=========================================
	mhGPUSsao = hGPUDescriptor;
	// SSAO

	mhGPUArray = hGPUDescriptor;
	BuildTexture2DArraySrv(mTreeMapTex, srvDesc, hCPUDescriptor, hGPUDescriptor, mCbvSrvUavDescriptorSize);
	mhGPUCube = hGPUDescriptor;
	BuildTextureCubeSrv(mCubeMapTex, srvDesc, hCPUDescriptor, hGPUDescriptor, mCbvSrvUavDescriptorSize);

	{
		//=========================================
		// 가로방향
		// Texture2D gInput : register(t0);				// SRV
		// RWTexture2D<float4> gOutput : register(u0);	// UAV
		// 세로방향
		// Texture2D gInput : register(t0);				// SRV
		// RWTexture2D<float4> gOutput : register(u0);	// UAV
		//=========================================
		mCSBlurFilter->BuildDescriptors(hCPUDescriptor, hGPUDescriptor, mCbvSrvUavDescriptorSize);
		hCPUDescriptor.Offset(mCSBlurFilter->DescriptorCount(), mCbvSrvUavDescriptorSize);
		hGPUDescriptor.Offset(mCSBlurFilter->DescriptorCount(), mCbvSrvUavDescriptorSize);
	}
	
	{
		//=========================================
		// SRV
		// gPrevSolInput (RWTexture2D<float>, u0) 
		// gCurrSolInput (RWTexture2D<float>, u1) 
		// gOutput		 (RWTexture2D<float>, u2) 
		// UAV
		// gPrevSolInput (RWTexture2D<float>, u0) 
		// gCurrSolInput (RWTexture2D<float>, u1) 
		// gOutput		 (RWTexture2D<float>, u2) 
		//=========================================
		mCSWaves->BuildDescriptors(hCPUDescriptor, hGPUDescriptor, mCbvSrvUavDescriptorSize);
		hCPUDescriptor.Offset(mCSWaves->DescriptorCount(), mCbvSrvUavDescriptorSize);
		hGPUDescriptor.Offset(mCSWaves->DescriptorCount(), mCbvSrvUavDescriptorSize);
	}
}

void MyApp::BuildTexture2DSrv(const std::unordered_map<std::wstring, std::unique_ptr<Texture>>& texMap, D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc, CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE& hGpuSrv, UINT descriptorSize)
{
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	for (const auto& tex : texMap)
	{
		auto texture = tex.second.get()->Resource;

		srvDesc.Format = texture->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;
		mDevice->CreateShaderResourceView(texture.Get(), &srvDesc, hCpuSrv);
		hCpuSrv.Offset(1, descriptorSize);
		hGpuSrv.Offset(1, descriptorSize);
	}
}

void MyApp::BuildTexture2DArraySrv(const std::unordered_map<std::wstring, std::unique_ptr<Texture>>& texMap, D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc, CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE& hGpuSrv, UINT descriptorSize)
{
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2DArray.MipLevels = -1;
	srvDesc.Texture2DArray.FirstArraySlice = 0;

	for (const auto& tex : texMap)
	{
		auto texture = tex.second.get()->Resource;

		srvDesc.Format = texture->GetDesc().Format;
		srvDesc.Texture2DArray.ArraySize = texture->GetDesc().DepthOrArraySize;
		// srvDesc.Texture2DArray.MipLevels = texture->GetDesc().MipLevels;
		mDevice->CreateShaderResourceView(texture.Get(), &srvDesc, hCpuSrv);
		hCpuSrv.Offset(1, descriptorSize);
		hGpuSrv.Offset(1, descriptorSize);
	}
}

void MyApp::BuildTextureCubeSrv(const std::unordered_map<std::wstring, std::unique_ptr<Texture>>& texMap, D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc, CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE& hGpuSrv, UINT descriptorSize)
{
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	for (const auto& tex : texMap)
	{
		auto texture = tex.second.get()->Resource;

		srvDesc.Format = texture->GetDesc().Format;
		srvDesc.TextureCube.MipLevels = texture->GetDesc().MipLevels;
		mDevice->CreateShaderResourceView(texture.Get(), &srvDesc, hCpuSrv);
		hCpuSrv.Offset(1, descriptorSize);
		hGpuSrv.Offset(1, descriptorSize);
	}
}



void MyApp::BuildShadersAndInputLayout()
{
	char maxLights[100];
	char texDiffSize[100];
	char texNormSize[100];
	char texArraySize[100];
	char texCubeSize[100];
	strcpy_s(maxLights, std::to_string(MAX_LIGHTS).c_str());
	strcpy_s(texDiffSize, std::to_string(mDiffuseTex.size() + SRV_USER_SIZE).c_str());
	strcpy_s(texNormSize, std::to_string(mNormalTex.size()).c_str());
	strcpy_s(texArraySize, std::to_string(mTreeMapTex.size()).c_str());
	strcpy_s(texCubeSize, std::to_string(mCubeMapTex.size()).c_str());
	const D3D_SHADER_MACRO defines[] =
	{
		"MAX_LIGHTS", maxLights,
		"TEX_DIFF_SIZE", texDiffSize,
		"TEX_NORM_SIZE", texNormSize,
		"TEX_ARRAY_SIZE", texArraySize,
		"TEX_CUBE_SIZE", texCubeSize,
		NULL, NULL
	};
	const D3D_SHADER_MACRO skinnedDefines[] =
	{
		"MAX_LIGHTS", maxLights,
		"TEX_DIFF_SIZE", texDiffSize,
		"TEX_NORM_SIZE", texNormSize,
		"TEX_ARRAY_SIZE", texArraySize,
		"TEX_CUBE_SIZE", texCubeSize,
		"SKINNED", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO fogDefines[] =
	{
		"MAX_LIGHTS", maxLights,
		"TEX_DIFF_SIZE", texDiffSize,
		"TEX_NORM_SIZE", texNormSize,
		"TEX_ARRAY_SIZE", texArraySize,
		"TEX_CUBE_SIZE", texCubeSize,
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"MAX_LIGHTS", maxLights,
		"TEX_DIFF_SIZE", texDiffSize,
		"TEX_NORM_SIZE", texNormSize,
		"TEX_ARRAY_SIZE", texArraySize,
		"TEX_CUBE_SIZE", texCubeSize,
		"ALPHA_TEST", "1",
		NULL, NULL
	};
	const D3D_SHADER_MACRO alphaFogTestDefines[] =
	{
		"MAX_LIGHTS", maxLights,
		"TEX_DIFF_SIZE", texDiffSize,
		"TEX_NORM_SIZE", texNormSize,
		"TEX_ARRAY_SIZE", texArraySize,
		"TEX_CUBE_SIZE", texCubeSize,
		"ALPHA_TEST", "1",
		"FOG", "1",
		NULL, NULL
	};


	// 오직 Release 모드에서만 동작이 가능함 (Timeout 발생)

	mShaders["MainVS"] = D3DUtil::CompileShader(L"Main.hlsl", defines, "VS", "vs_5_1");
	mShaders["SkinnedVS"] = D3DUtil::CompileShader(L"Main.hlsl", skinnedDefines, "VS", "vs_5_1");
	mShaders["MainPS"] = D3DUtil::CompileShader(L"Main.hlsl", defines, "PS", "ps_5_1");
	mShaders["AlphaTestedPS"] = D3DUtil::CompileShader(L"Main.hlsl", alphaTestDefines, "PS", "ps_5_1");
	

	mShaders["NormalVS"] = D3DUtil::CompileShader(L"Normal.hlsl", defines, "VS", "vs_5_1");
	mShaders["SkinnedNormalVS"] = D3DUtil::CompileShader(L"Normal.hlsl", skinnedDefines, "VS", "vs_5_1");
	mShaders["NormalGS"] = D3DUtil::CompileShader(L"Normal.hlsl", defines, "GS", "gs_5_1");
	mShaders["NormalPS"] = D3DUtil::CompileShader(L"Normal.hlsl", defines, "PS", "ps_5_1");

	mShaders["BillboardVS"] = D3DUtil::CompileShader(L"Billboard.hlsl", defines, "VS", "vs_5_1");
	mShaders["BillboardGS"] = D3DUtil::CompileShader(L"Billboard.hlsl", defines, "GS", "gs_5_1");
	mShaders["BillboardPS"] = D3DUtil::CompileShader(L"Billboard.hlsl", alphaTestDefines, "PS", "ps_5_1");

	mShaders["SubdivisionVS"] = D3DUtil::CompileShader(L"Subdivision.hlsl", defines, "VS", "vs_5_1");
	mShaders["SubdivisionGS"] = D3DUtil::CompileShader(L"Subdivision.hlsl", defines, "GS", "gs_5_1");

	mShaders["TessVS"] = D3DUtil::CompileShader(L"Tessellation.hlsl", defines, "VS", "vs_5_1");
	mShaders["TessHS"] = D3DUtil::CompileShader(L"Tessellation.hlsl", defines, "HS", "hs_5_1");
	mShaders["TessDS"] = D3DUtil::CompileShader(L"Tessellation.hlsl", defines, "DS", "ds_5_1");

	mShaders["BasicVS"] = D3DUtil::CompileShader(L"Basic.hlsl", defines, "VS", "vs_5_1");
	mShaders["BasicPS"] = D3DUtil::CompileShader(L"Basic.hlsl", defines, "PS", "ps_5_1");

	mShaders["CubeMapVS"] = D3DUtil::CompileShader(L"CubeMap.hlsl", defines, "VS", "vs_5_1");
	mShaders["CubeMapPS"] = D3DUtil::CompileShader(L"CubeMap.hlsl", defines, "PS", "ps_5_1");

	mShaders["ShadowVS"] = D3DUtil::CompileShader(L"Shadow.hlsl", defines, "VS", "vs_5_1");
	mShaders["SkinnedShadowVS"] = D3DUtil::CompileShader(L"Shadow.hlsl", skinnedDefines, "VS", "vs_5_1");
	mShaders["ShadowPS"] = D3DUtil::CompileShader(L"Shadow.hlsl", defines, "PS", "ps_5_1");

	mShaders["ShadowDebugVS"] = D3DUtil::CompileShader(L"ShadowDebug.hlsl", defines, "VS", "vs_5_1");
	mShaders["ShadowDebugPS"] = D3DUtil::CompileShader(L"ShadowDebug.hlsl", defines, "PS", "ps_5_1");

	mMainInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		
	};

	mTreeSpriteInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	mSkinnedInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "WEIGHTS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, 56, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

GeometryGenerator::MeshData MyApp::LoadModelMesh(std::string dir)
{
	using namespace DirectX;
	GeometryGenerator::MeshData mesh;

	std::ifstream fin(dir);

	if (!fin)
	{
		std::cout << "* Error, Fail Load File, dir: " << dir << std::endl;
		return mesh;
	}

	UINT vcount = 0;
	UINT tcount = 0;
	std::string ignore;

	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;

	mesh.Vertices.resize(vcount);
	for (UINT i = 0; i < vcount; ++i)
	{
		fin >> mesh.Vertices[i].Position.x >> mesh.Vertices[i].Position.y >> mesh.Vertices[i].Position.z;
		fin >> mesh.Vertices[i].Normal.x >> mesh.Vertices[i].Normal.y >> mesh.Vertices[i].Normal.z;

		XMVECTOR P = XMLoadFloat3(&mesh.Vertices[i].Position);

		// Project point onto unit sphere and generate spherical texture coordinates.
		XMFLOAT3 spherePos;
		DirectX::XMStoreFloat3(&spherePos, XMVector3Normalize(P));

		float theta = atan2f(spherePos.z, spherePos.x);

		// Put in [0, 2pi].
		if (theta < 0.0f)
			theta += XM_2PI;

		float phi = acosf(spherePos.y);

		float u = theta / (2.0f * XM_PI);
		float v = phi / XM_PI;

		mesh.Vertices[i].TexC = { u, v };
	}

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	mesh.Indices32.resize(3 * tcount);
	for (UINT i = 0; i < tcount; ++i)
	{
		fin >> mesh.Indices32[i * 3 + 0] >> mesh.Indices32[i * 3 + 1] >> mesh.Indices32[i * 3 + 2];
	}

	fin.close();

	return mesh;
}

void MyApp::LoadSkinnedModelMesh(const std::string& dir)
{
	GeometryGenerator::MeshData mesh;
	std::vector<M3DLoader::Subset> skinnedSubsets;
	SkinnedData skinnedInfo;

	mSkinnedModelInst = std::make_unique<SkinnedModelInstance>();
	M3DLoader::LoadM3d(dir, mesh.SkinnedVertices, mesh.Indices32,
		skinnedSubsets, mSkinnedMats, mSkinnedModelInst->SkinnedInfo);
	mSkinnedModelInst->FinalTransforms.resize(mSkinnedModelInst->SkinnedInfo.BoneCount());
	mSkinnedModelInst->ClipName = "Take1";
	mSkinnedModelInst->TimePos = 0.0f;


	using namespace DirectX;

	//=========================================================
	// Part 2. SubmeshGeometry 생성
	//=========================================================
	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = dir;

	const UINT vbByteSize = (UINT)mesh.SkinnedVertices.size() * sizeof(SkinnedVertex);
	const UINT ibByteSize = (UINT)mesh.Indices32.size() * sizeof(std::uint16_t);

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), mesh.SkinnedVertices.data(), vbByteSize);
	
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), mesh.GetIndices16().data(), ibByteSize);

	geo->VertexBufferGPU = D3DUtil::CreateDefaultBuffer(mDevice.Get(),
		mCommandList.Get(), mesh.SkinnedVertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(mDevice.Get(),
		mCommandList.Get(), mesh.GetIndices16().data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(SkinnedVertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	std::vector<SubmeshGeometry> submeshes(skinnedSubsets.size());
	for (size_t i = 0; i < skinnedSubsets.size(); ++i)
	{
		SubmeshGeometry submesh;
		std::string name = "sm_" + std::to_string(i);

		GeometryGenerator::FindBounding(submesh.BoundingBox, submesh.BoundingSphere, mesh.SkinnedVertices);
		submesh.IndexCount = (UINT)skinnedSubsets[i].FaceCount * 3;
		submesh.StartIndexLocation = skinnedSubsets[i].FaceStart * 3;
		submesh.BaseVertexLocation = 0;

		geo->DrawArgs[name] = submesh;
	}
		
	mGeometries.push_back(std::move(geo));
}

void MyApp::BuildMeshes()
{
	mMeshes.push_back(GeometryGenerator::CreateBox(2.0f, 2.0f, 2.0f, 3));
	mMeshes.push_back(GeometryGenerator::CreateGrid(20.0f, 30.0f, 60, 40));
	mMeshes.push_back(GeometryGenerator::CreateSphere(1.0f, 20, 20));
	mMeshes.push_back(GeometryGenerator::CreateCylinder(0.3f, 0.5f, 3.0f, 20, 20));
	mMeshes.push_back(GeometryGenerator::CreateQuad(0.0f, 0.0f, 1.0f, 1.0f, 0.0f));

	mMeshes.push_back(GeometryGenerator::CreateGrid(160.0f, 160.0f, 50, 50));	// Land
	for (auto& vertice : mMeshes[5].Vertices)
	{
		vertice.Position.y = GetHillsHeight(vertice.Position.x, vertice.Position.z);
		vertice.Normal = GetHillsNormal(vertice.Position.x, vertice.Position.z);
	}

	mMeshes.push_back(GeometryGenerator::CreateGrid(160.0f, 160.0f, mCSWaves->RowCount(), mCSWaves->ColumnCount()));	// Wave
	mMeshes.push_back(LoadModelMesh("../Data/Models/skull.txt"));
	mMeshes.push_back(LoadModelMesh("../Data/Models/car.txt"));

	UpdateTangents();

	BuildGeometry(mMeshes);
	LoadSkinnedModelMesh("../Data/Models/soldier.m3d");
}

void MyApp::BuildGeometry(std::vector<GeometryGenerator::MeshData>& meshes, bool useIndex16, bool useSkinnedMesh)
{
	using namespace DirectX;
	//=========================================================
	// Part 1. vertices & indices 병합
	//=========================================================
	std::vector<GeometryGenerator::SkinnedVertex> skinnedVertices;
	std::vector<GeometryGenerator::Vertex> vertices;
		
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
		? (UINT)skinnedVertices.size() * sizeof(GeometryGenerator::SkinnedVertex)
		: (UINT)vertices.size() * sizeof(GeometryGenerator::Vertex);
	const UINT ibByteSize = useIndex16 
		? (UINT)indices16.size() * sizeof(std::uint16_t)
		: (UINT)indices32.size() * sizeof(std::uint32_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = mGeometries.size();
	geo->VertexByteStride = useSkinnedMesh ? sizeof(GeometryGenerator::SkinnedVertex) : sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = useIndex16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	if (useSkinnedMesh)
	{
		CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), skinnedVertices.data(), vbByteSize);
		geo->VertexBufferGPU = D3DUtil::CreateDefaultBuffer(mDevice.Get(), mCommandList.Get(),
			skinnedVertices.data(), vbByteSize, geo->VertexBufferUploader);
	}
	else
	{
		CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
		geo->VertexBufferGPU = D3DUtil::CreateDefaultBuffer(mDevice.Get(), mCommandList.Get(),
			vertices.data(), vbByteSize, geo->VertexBufferUploader);
	}
		
	if (useIndex16)
	{
		CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices16.data(), ibByteSize);
		geo->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(mDevice.Get(), mCommandList.Get(),
			indices16.data(), ibByteSize, geo->IndexBufferUploader);
	}
	else
	{
		CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices32.data(), ibByteSize);
		geo->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(mDevice.Get(), mCommandList.Get(),
			indices32.data(), ibByteSize, geo->IndexBufferUploader);
	}	

	//=========================================================
	// Part 3. SubmeshGeometry 생성
	//=========================================================
	std::vector<SubmeshGeometry> submeshes(meshes.size());
	for (size_t i = 0; i < meshes.size(); ++i)
	{
		if (useSkinnedMesh)
			GeometryGenerator::FindBounding(submeshes[i].BoundingBox, submeshes[i].BoundingSphere, meshes[i].SkinnedVertices);
		else
			GeometryGenerator::FindBounding(submeshes[i].BoundingBox, submeshes[i].BoundingSphere, meshes[i].Vertices);
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
		geo->DrawArgs[std::to_string(i)] = submeshes[i];
	}

	mGeometries.push_back(std::move(geo));
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
	geo->Name = "TreeSpriteGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
	geo->VertexBufferGPU = D3DUtil::CreateDefaultBuffer(mDevice.Get(), mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(mDevice.Get(), mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(TreeSpriteVertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["0"] = submesh;

	mGeometries.push_back(std::move(geo));
}

void MyApp::BuildMaterials()
{
	UINT idx = 0;
	for (size_t i = 0; i < SRV_USER_SIZE; ++i)
	{
		auto mat = std::make_unique<EXMaterialData>();
		mat->MaterialData.DiffMapIndex = i;
		mat->MaterialData.useAlbedoMap = 1;
		mat->NumFramesDirty = APP_NUM_FRAME_RESOURCES;
		mAllMatItems.push_back(std::move(mat));
	}
	for (size_t i = 0; i < mDiffuseTex.size(); ++i)
	{
		auto mat = std::make_unique<EXMaterialData>();
		mat->MaterialData.DiffMapIndex = SRV_USER_SIZE + i;
		mat->MaterialData.useAlbedoMap = 1;
		mat->NumFramesDirty = APP_NUM_FRAME_RESOURCES;
		mAllMatItems.push_back(std::move(mat));
	}

	for (size_t i = 0; i < mTreeMapTex.size(); ++i)
	{
		auto mat = std::make_unique<EXMaterialData>();
		mat->MaterialData.DiffMapIndex = i;
		mat->MaterialData.useAlbedoMap = 1;
		mat->NumFramesDirty = APP_NUM_FRAME_RESOURCES;
		mAllMatItems.push_back(std::move(mat));
	}
}


void MyApp::BuildRenderItems()
{
	auto boxRitem = std::make_unique<RenderItem>(mGeometries[0].get(), mGeometries[0]->DrawArgs["0"]);
	DirectX::SimpleMath::Vector3 translation;
	DirectX::SimpleMath::Vector3 scale(1.0f, 1.0f, 1.0f);
	DirectX::SimpleMath::Quaternion rot;
	DirectX::SimpleMath::Vector3 texScale(1.0f, 1.0f, 1.0f);
	int repeatCount = 10;

	for (int i = 0; i < mAllMatItems.size() * repeatCount; ++i)
	{
		translation = { (i % 10) * 8.0f, 10.0f, 5.0f + 8.0f * (i / 10) };
		scale = { 2.0f, 2.0f, 2.0f };
		boxRitem->Push(translation, scale, rot, { 2.0f, 1.0f, 2.0f }, mInstanceCount++, i % mAllMatItems.size());
	}
	mAllRitems.push_back(std::move(boxRitem));

	for (int i = 0; i < mAllMatItems.size() * repeatCount; ++i)
	{
		boxRitem = std::make_unique<RenderItem>(mGeometries[0].get(), mGeometries[0]->DrawArgs["0"]);
		translation = { (i % 10) * 8.0f, 20.0f, 5.0f + 8.0f * (i / 10) };
		scale = { 2.0f, 2.0f, 2.0f };
		boxRitem->Push(translation, scale, rot, { 2.0f, 1.0f, 2.0f }, mInstanceCount++, i % mAllMatItems.size());
		mAllRitems.push_back(std::move(boxRitem));
	}

	{
		auto cubeSphereRitem = std::make_unique<RenderItem>(mGeometries[0].get(), mGeometries[0]->DrawArgs["2"]);
		cubeSphereRitem->LayerFlag
			= (1 << (int)RenderLayer::CubeMap);

		translation.x = 0.0f;
		translation.y = 0.0f;
		translation.z = 0.0f;
		scale.x = 5000.0f;
		scale.y = 5000.0f;
		scale.z = 5000.0f;
		cubeSphereRitem->Push(translation, scale, rot, texScale, mInstanceCount++, SRV_USER_SIZE + mDiffuseTex.size() + mTreeMapTex.size(), false);
		cubeSphereRitem->Datas.back().IsPickable = false;
		mAllRitems.push_back(std::move(cubeSphereRitem));
	}
}

void MyApp::BuildFrameResources()
{
	for (int i = 0; i < APP_NUM_FRAME_RESOURCES; ++i)
		mFrameResources.push_back(std::make_unique<FrameResource>(mDevice.Get(), 1 + MAX_LIGHTS, (UINT)mAllRitems.size(), mInstanceCount * 2, (UINT)mAllMatItems.size(), 1));
}

void MyApp::BuildPSOs()
{
	// Mesh Shader(2018) <- Compute Shader (고착화)
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc
	{
		/* ID3D12RootSignature* pRootSignature								*/.pRootSignature = mRootSignature.Get(),
		/* D3D12_SHADER_BYTECODE VS											*/.VS = {reinterpret_cast<BYTE*>(mShaders["MainVS"]->GetBufferPointer()), mShaders["MainVS"]->GetBufferSize()},
		/* D3D12_SHADER_BYTECODE PS											*/.PS = {reinterpret_cast<BYTE*>(mShaders["MainPS"]->GetBufferPointer()), mShaders["MainPS"]->GetBufferSize()},
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
		/* DXGI_FORMAT RTVFormats[8]										*/.RTVFormats = {mBackBufferFormat, mBackBufferFormat,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN},	// 0
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
	alphaTestedPsoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["AlphaTestedPS"]->GetBufferPointer()),	mShaders["AlphaTestedPS"]->GetBufferSize()};
	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	//=====================================================
	// PSO for Subdivision
	//=====================================================
	D3D12_GRAPHICS_PIPELINE_STATE_DESC subdivisionDesc = opaquePsoDesc;
	subdivisionDesc.VS = { reinterpret_cast<BYTE*>(mShaders["SubdivisionVS"]->GetBufferPointer()), mShaders["SubdivisionVS"]->GetBufferSize() };
	subdivisionDesc.GS = { reinterpret_cast<BYTE*>(mShaders["SubdivisionGS"]->GetBufferPointer()), mShaders["SubdivisionGS"]->GetBufferSize() };

	//=====================================================
	// PSO for Normal
	//=====================================================
	D3D12_GRAPHICS_PIPELINE_STATE_DESC normalDesc = opaquePsoDesc;
	normalDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	// normalDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	normalDesc.VS = { reinterpret_cast<BYTE*>(mShaders["NormalVS"]->GetBufferPointer()), mShaders["NormalVS"]->GetBufferSize() };
	normalDesc.GS = { reinterpret_cast<BYTE*>(mShaders["NormalGS"]->GetBufferPointer()), mShaders["NormalGS"]->GetBufferSize() };
	normalDesc.PS = { reinterpret_cast<BYTE*>(mShaders["NormalPS"]->GetBufferPointer()), mShaders["NormalPS"]->GetBufferSize() };

	//=====================================================
	// PSO for tree sprites
	//=====================================================
	D3D12_GRAPHICS_PIPELINE_STATE_DESC treeSpritePsoDesc = opaquePsoDesc;
	treeSpritePsoDesc.VS = { reinterpret_cast<BYTE*>(mShaders["BillboardVS"]->GetBufferPointer()), mShaders["BillboardVS"]->GetBufferSize() };
	treeSpritePsoDesc.GS = { reinterpret_cast<BYTE*>(mShaders["BillboardGS"]->GetBufferPointer()), mShaders["BillboardGS"]->GetBufferSize() };
	treeSpritePsoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["BillboardPS"]->GetBufferPointer()), mShaders["BillboardPS"]->GetBufferSize() };
	treeSpritePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	treeSpritePsoDesc.InputLayout = { mTreeSpriteInputLayout.data(), (UINT)mTreeSpriteInputLayout.size() };
	treeSpritePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	//=====================================================
	// PSO for Tessellation
	//=====================================================
	D3D12_GRAPHICS_PIPELINE_STATE_DESC tessPsoDesc = opaquePsoDesc;
	tessPsoDesc.VS = { reinterpret_cast<BYTE*>(mShaders["TessVS"]->GetBufferPointer()), mShaders["TessVS"]->GetBufferSize() };
	tessPsoDesc.HS = { reinterpret_cast<BYTE*>(mShaders["TessHS"]->GetBufferPointer()), mShaders["TessHS"]->GetBufferSize() };
	tessPsoDesc.DS = { reinterpret_cast<BYTE*>(mShaders["TessDS"]->GetBufferPointer()), mShaders["TessDS"]->GetBufferSize() };
	// tessPsoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["TessPS"]->GetBufferPointer()), mShaders["TessPS"]->GetBufferSize() };
	tessPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;

	//=====================================================
	// PSO for BoundingBox
	//=====================================================
	D3D12_GRAPHICS_PIPELINE_STATE_DESC boundingBoxPsoDesc = opaquePsoDesc;
	boundingBoxPsoDesc.VS = { reinterpret_cast<BYTE*>(mShaders["BasicVS"]->GetBufferPointer()), mShaders["BasicVS"]->GetBufferSize() };
	boundingBoxPsoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["BasicPS"]->GetBufferPointer()), mShaders["BasicPS"]->GetBufferSize() };
	boundingBoxPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;

	//=====================================================
	// PSO for CubeMap
	//=====================================================
	D3D12_GRAPHICS_PIPELINE_STATE_DESC cubeMapPsoDesc = opaquePsoDesc;
	cubeMapPsoDesc.VS = { reinterpret_cast<BYTE*>(mShaders["CubeMapVS"]->GetBufferPointer()), mShaders["CubeMapVS"]->GetBufferSize() };
	cubeMapPsoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["CubeMapPS"]->GetBufferPointer()), mShaders["CubeMapPS"]->GetBufferSize() };
	cubeMapPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	cubeMapPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	//=====================================================
	// PSO for ShadowMap
	//=====================================================
	D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowMapPsoDesc = opaquePsoDesc;
	shadowMapPsoDesc.VS = { reinterpret_cast<BYTE*>(mShaders["ShadowVS"]->GetBufferPointer()), mShaders["ShadowVS"]->GetBufferSize() };
	shadowMapPsoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["ShadowPS"]->GetBufferPointer()), mShaders["ShadowPS"]->GetBufferSize() };
	shadowMapPsoDesc.RasterizerState.DepthBias = 100000;
	shadowMapPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
	shadowMapPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
	shadowMapPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	shadowMapPsoDesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
	shadowMapPsoDesc.NumRenderTargets = 0;
	shadowMapPsoDesc.BlendState.IndependentBlendEnable = false;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC skinnedShadowMapPsoDesc = shadowMapPsoDesc;
	skinnedShadowMapPsoDesc.InputLayout = { mSkinnedInputLayout.data(), (UINT)mSkinnedInputLayout.size() };
	skinnedShadowMapPsoDesc.VS = {reinterpret_cast<BYTE*>(mShaders["SkinnedShadowVS"]->GetBufferPointer()), mShaders["SkinnedShadowVS"]->GetBufferSize()};

	//=====================================================
	// PSO for Debug ShadowMap
	//=====================================================
	D3D12_GRAPHICS_PIPELINE_STATE_DESC debugShadowMapPsoDesc = opaquePsoDesc;
	debugShadowMapPsoDesc.VS = { reinterpret_cast<BYTE*>(mShaders["ShadowDebugVS"]->GetBufferPointer()), mShaders["ShadowDebugVS"]->GetBufferSize() };
	debugShadowMapPsoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["ShadowDebugPS"]->GetBufferPointer()), mShaders["ShadowDebugPS"]->GetBufferSize() };

	//=====================================================
	// PSO for marking skinned mesh.
	//=====================================================
	D3D12_GRAPHICS_PIPELINE_STATE_DESC skinnedOpaquePsoDesc = opaquePsoDesc;
	skinnedOpaquePsoDesc.InputLayout = { mSkinnedInputLayout.data(), (UINT)mSkinnedInputLayout.size() };
	skinnedOpaquePsoDesc.VS = { reinterpret_cast<BYTE*>(mShaders["SkinnedVS"]->GetBufferPointer()), mShaders["SkinnedVS"]->GetBufferSize() };

	D3D12_GRAPHICS_PIPELINE_STATE_DESC skinnedNormalPsoDesc = normalDesc;
	skinnedNormalPsoDesc.InputLayout = { mSkinnedInputLayout.data(), (UINT)mSkinnedInputLayout.size() };
	skinnedNormalPsoDesc.VS = { reinterpret_cast<BYTE*>(mShaders["SkinnedNormalVS"]->GetBufferPointer()), mShaders["SkinnedNormalVS"]->GetBufferSize() };

	

	//=====================================================
	// Create PSO
	//=====================================================
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::Opaque])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&skinnedOpaquePsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::SkinnedOpaque])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&markMirrorsPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::Mirror])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&drawReflectionsPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::Reflected])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::Transparent])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::AlphaTested])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&subdivisionDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::Subdivision])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&normalDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::Normal])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&skinnedNormalPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::SkinnedNormal])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&treeSpritePsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::TreeSprites])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&tessPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::Tessellation])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&boundingBoxPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::BoundingBox])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&boundingBoxPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::BoundingSphere])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&cubeMapPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::CubeMap])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&shadowMapPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::ShadowMap])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&skinnedShadowMapPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::SkinnedShadowMap])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&debugShadowMapPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::DebugShadowMap])));

	//=====================================================
	// PSO for wireframe objects.
	//=====================================================
	opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	markMirrorsPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	drawReflectionsPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	transparentPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	alphaTestedPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	subdivisionDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	normalDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	treeSpritePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	tessPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;

	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::OpaqueWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&markMirrorsPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::MirrorWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&drawReflectionsPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::ReflectedWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::TransparentWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::AlphaTestedWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&subdivisionDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::SubdivisionWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&normalDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::NormalWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&treeSpritePsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::TreeSpritesWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&tessPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::TessellationWireframe])));
}
#pragma endregion Initialize

void MyApp::CreateRtvAndDsvDescriptorHeaps(UINT numRTV, UINT numDSV)
{
	AppBase::CreateRtvAndDsvDescriptorHeaps(APP_NUM_BACK_BUFFERS + 1, 1 + MAX_LIGHTS);
}

#pragma region Update
void MyApp::OnResize()
{
	AppBase::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	mCamera.SetLens(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);

	DirectX::BoundingFrustum::CreateFromMatrix(mCamFrustum, mCamera.GetProj());
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

		CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), SRV_IMGUI_SIZE, mCbvSrvUavDescriptorSize);
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
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % APP_NUM_FRAME_RESOURCES;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, mFenceEvent));
		WaitForSingleObject(mFenceEvent, INFINITE);
	}

	OnKeyboardInput();	
	UpdateMaterials();
	UpdateShadowMap();
	UpdateInstanceBuffer();
	UpdateMaterialBuffer();
	UpdatePassCB();
	UpdateSkinnedCB();
}

void MyApp::Render()
{
	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(mCurrFrameResource->CmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(mCurrFrameResource->CmdListAlloc.Get(), nullptr));

	// Bind per-pass constant buffer.  We only need to do this once per-pass.
	auto passCB = mCurrFrameResource->PassCB->Resource();
	UINT passCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	auto matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
	auto instanceBuffer = mCurrFrameResource->InstanceBuffer->Resource();

	// GPU Memory setting
	{
		ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
		mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

		mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
		mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());
		mCommandList->SetGraphicsRootShaderResourceView(3, instanceBuffer->GetGPUVirtualAddress());
		mCommandList->SetGraphicsRootShaderResourceView(4, matBuffer->GetGPUVirtualAddress());
		mCommandList->SetGraphicsRootDescriptorTable(5, mhGPUUser);
		mCommandList->SetGraphicsRootDescriptorTable(6, mCSWaves->DisplacementMap());
		mCommandList->SetGraphicsRootDescriptorTable(7, mhGPUNorm); 
		mCommandList->SetGraphicsRootDescriptorTable(12, mhGPUShadow);
		mCommandList->SetGraphicsRootDescriptorTable(14, mhGPUArray);
		mCommandList->SetGraphicsRootDescriptorTable(15, mhGPUCube);

		mLastVertexBufferView = mAllRitems[0]->Geo->VertexBufferView();
		mLastIndexBufferView = mAllRitems[0]->Geo->IndexBufferView();
		mLastPrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		mCommandList->IASetVertexBuffers(0, 1, &mLastVertexBufferView);
		mCommandList->IASetIndexBuffer(&mLastIndexBufferView);
		mCommandList->IASetPrimitiveTopology(mLastPrimitiveType);
	}

	D3D12_VERTEX_BUFFER_VIEW mLastVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW mLastIndexBufferView;
	D3D12_PRIMITIVE_TOPOLOGY mLastPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	for (int i = 0; i < MAX_LAYER_DEPTH; ++i)
	{
		if (mLayerType[i] == RenderLayer::ShadowMap
			|| mLayerType[i] == RenderLayer::SkinnedShadowMap)
			for (int i = 0; i < MAX_LIGHTS; ++i)
			{
				if(mUseShadowMap[i])
					DrawSceneToShadowMap(i);
			}
		else
			continue;
	}

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	D3D12_RESOURCE_BARRIER RenderBarrier = 
		CD3DX12_RESOURCE_BARRIER::Transition(mSwapChainBuffer[mCurrBackBuffer].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	D3D12_RESOURCE_BARRIER SRVUserBufBarrier[SRV_USER_SIZE];
	for(int i=0; i<SRV_USER_SIZE; ++i)
		SRVUserBufBarrier[i] = CD3DX12_RESOURCE_BARRIER::Transition(mSRVUserBuffer[i].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);

	mCommandList->ResourceBarrier(1, &RenderBarrier);
	mCommandList->ResourceBarrier(SRV_USER_SIZE, &SRVUserBufBarrier[0]);

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(mhCPUSwapChainBuffer[mCurrBackBuffer], (float*)&mMainPassCB.FogColor, 0, nullptr);
	mCommandList->ClearRenderTargetView(mhCPUSwapChainBuffer[APP_NUM_BACK_BUFFERS], (float*)&mMainPassCB.FogColor, 0, nullptr);
	mCommandList->ClearDepthStencilView(mhCPUDSVBuffer[0], D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	D3D12_CPU_DESCRIPTOR_HANDLE rtvs[2];
	rtvs[0] = mhCPUSwapChainBuffer[mCurrBackBuffer];		// 기존
	rtvs[1] = mhCPUSwapChainBuffer[APP_NUM_BACK_BUFFERS];   // 새로 만든 RTV
	mCommandList->OMSetRenderTargets(2, rtvs, false, &mhCPUDSVBuffer[0]);

	for (int i = 0; i < MAX_LAYER_DEPTH; ++i)
	{
		if (mLayerType[i] == RenderLayer::None)
			continue;
		else if (
			mLayerType[i] == RenderLayer::ShadowMap
			|| mLayerType[i] == RenderLayer::SkinnedShadowMap
			|| mLayerType[i] == RenderLayer::AddCS
			|| mLayerType[i] == RenderLayer::BlurCS			|| mLayerType[i] == RenderLayer::WaveCS)
			continue;
		else {
			mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress() + passCBByteSize * mLayerCBIdx[i]);
			mCommandList->OMSetStencilRef(mLayerStencil[i]);
			mCommandList->SetPipelineState(mPSOs[mLayerType[i]].Get());
			DrawRenderItems(mLayerType[i]);
		}
	}

	// Post process
	for (int i = 0; i < MAX_LAYER_DEPTH; ++i)
	{
		if (mLayerType[i] == RenderLayer::AddCS)
		{
			mCSAdd->DoComputeWork(mCommandList.Get(), mCurrFrameResource->CmdListAlloc.Get());
			mLayerType[i] = RenderLayer::None;
			mSyncEn = true;
		}
		else if (mLayerType[i] == RenderLayer::BlurCS)
		{
			mCSBlurFilter->Execute(mCommandList.Get(), mSwapChainBuffer[mCurrBackBuffer].Get(), 4);
		}
		else if (mLayerType[i] == RenderLayer::WaveCS)
		{
			mCSWaves->UpdateWaves(mTimer, mCommandList.Get());
		}
		else
			continue;
	}
	// Copy Back Buffer
	{	
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
	mCommandList->ResourceBarrier(SRV_USER_SIZE, &SRVUserBufBarrier[0]);
}

void MyApp::Sync()
{
	mCSAdd->PrintOutput();
}

void MyApp::UpdateMaterials()
{
	// Scroll the water material texture coordinates.
	auto waterMat = mAllMatItems[6].get();

	float& tu = waterMat->MaterialData.MatTransform(3, 0);
	float& tv = waterMat->MaterialData.MatTransform(3, 1);

	tu += 0.1f * mTimer.DeltaTime();
	tv += 0.02f * mTimer.DeltaTime();

	if (tu >= 1.0f)
		tu -= 1.0f;

	if (tv >= 1.0f)
		tv -= 1.0f;

	waterMat->MaterialData.MatTransform(3, 0) = tu;
	waterMat->MaterialData.MatTransform(3, 1) = tv;

	// Material has changed, so need to update cbuffer.
	waterMat->NumFramesDirty = APP_NUM_FRAME_RESOURCES;
}

void MyApp::UpdateTangents()
{
	using namespace DirectX;

	for (auto& m : mMeshes) {
		std::vector<XMFLOAT3> positions(m.Vertices.size());
		std::vector<XMFLOAT3> normals(m.Vertices.size());
		std::vector<XMFLOAT2> texcoords(m.Vertices.size());
		std::vector<XMFLOAT3> tangents(m.Vertices.size());
		std::vector<XMFLOAT3> bitangents(m.Vertices.size());

		for (size_t i = 0; i < m.Vertices.size(); i++) {
			auto& v = m.Vertices[i];
			positions[i] = v.Position;
			normals[i] = v.Normal;
			texcoords[i] = v.TexC;
		}

		DirectX::ComputeTangentFrame(m.Indices32.data(), m.Indices32.size() / 3,
			positions.data(), normals.data(), texcoords.data(),
			m.Vertices.size(), tangents.data(),
			bitangents.data());

		for (size_t i = 0; i < m.Vertices.size(); i++) {
			m.Vertices[i].TangentU = tangents[i];
		}
	}
}

void MyApp::UpdateShadowMap()
{
	static float rotationAngle[MAX_LIGHTS];
	for (int i = 1; i < MAX_LIGHTS; ++i)	// 0번은 고정 위치에서 성능 비교
	{
		rotationAngle[i] += 0.1f * mTimer.DeltaTime();
		mShadowMap[i]->SetRotate(rotationAngle[i]);
	}
}

void MyApp::UpdateInstanceBuffer()
{
	auto currInstanceBuffer = mCurrFrameResource->InstanceBuffer.get();
	auto currInstanceCB = mCurrFrameResource->InstanceCB.get();
	int visibleInstanceCount = 0;
	for (size_t i = 0; i < mAllRitems.size(); ++i)
	{
		auto& e = mAllRitems[i];
		if (mCullingEnable)
		{
			InstanceConstants insCB;
			insCB.BaseInstanceIndex = visibleInstanceCount;
			e->StartInstanceLocation = visibleInstanceCount;
			currInstanceCB->CopyData(i, insCB);

			// 컬링을 활용하며, 매 Frame 간 데이터를 복사하는 로직으로 구현
			for (const auto& d : e->Datas)
			{
				// Cam Frustum에 포함되는 Instance만 복사
				// if ((mCamFrustum.Contains(d.BoundingBox) != DirectX::DISJOINT) || (d.FrustumCullingEnabled == false))
				if ((mCamFrustum.Contains(d.BoundingSphere) != DirectX::DISJOINT) || (d.FrustumCullingEnabled == false))
				{
					currInstanceBuffer->CopyData(visibleInstanceCount++, d.InstanceData);
				}
			}

			// Instance 개수 업데이트
			e->InstanceCount = visibleInstanceCount - e->StartInstanceLocation;
		}
		else
		{
			// Dirty Flag가 존재하는 경우 업데이트 (이동 등 instance 변경 발생 시 NumFramesDirty = APP_NUM_FRAME_RESOURCES)
			if (e->NumFramesDirty > 0)
			{
				InstanceConstants insCB;
				insCB.BaseInstanceIndex = visibleInstanceCount;
				e->StartInstanceLocation = visibleInstanceCount;
				currInstanceCB->CopyData(i, insCB);

				// Instance 전체 복사
				for (const auto& d : e->Datas)
				{
					currInstanceBuffer->CopyData(visibleInstanceCount++, d.InstanceData);
				}
				e->InstanceCount = visibleInstanceCount - e->StartInstanceLocation;

				e->NumFramesDirty--;
			}
		}
		
	}
	std::wostringstream outs;
	outs.precision(6);
	outs 
		<< L"Update objects " << visibleInstanceCount 
		<< L" / All objects " << mInstanceCount;
	mWndCaption = outs.str();
}

void MyApp::UpdateMaterialBuffer()
{
	auto currMaterialCB = mCurrFrameResource->MaterialBuffer.get();
	for (size_t i=0; i < mAllMatItems.size(); ++i)
	{
		auto& e = mAllMatItems[i];
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		if (e->NumFramesDirty > 0)
		{
			currMaterialCB->CopyData(i, e->MaterialData);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}
}

void MyApp::UpdatePassCB()
{
	DirectX::XMMATRIX view = mCamera.GetView();
	DirectX::XMMATRIX proj = mCamera.GetProj();
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
	mMainPassCB.EyePosW = mCamera.GetPosition3f();
	mMainPassCB.RenderTargetSize = DirectX::XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = DirectX::XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = mTimer.TotalTime();
	mMainPassCB.DeltaTime = mTimer.DeltaTime();
	mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	for (int i = 0; i < MAX_LIGHTS; ++i)
	{
		PassConstants shadowCB = mShadowMap[i]->GetPassCB();
		mMainPassCB.Lights[i] = shadowCB.Lights[0];
	}

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
	for (int i = 0; i < MAX_LIGHTS; ++i)
	{
		currPassCB->CopyData(1 + i, mShadowMap[i]->GetPassCB());
	}
}

void MyApp::UpdateSkinnedCB()
{
	auto currSkinnedCB = mCurrFrameResource->SkinnedCB.get();

	// We only have one skinned model being animated.
	mSkinnedModelInst->UpdateSkinnedAnimation(mTimer.DeltaTime());

	SkinnedConstants skinnedConstants;
	std::copy(
		std::begin(mSkinnedModelInst->FinalTransforms),
		std::end(mSkinnedModelInst->FinalTransforms),
		&skinnedConstants.BoneTransforms[0]);

	currSkinnedCB->CopyData(0, skinnedConstants);
}

#pragma endregion Update

void MyApp::DrawRenderItems(const RenderLayer flag)
{
	UINT objCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(InstanceConstants));
	UINT skinnedCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(SkinnedConstants));

	auto currInstanceCB = mCurrFrameResource->InstanceCB->Resource();
	auto skinnedCB = mCurrFrameResource->SkinnedCB->Resource();
	// For each render item...
	for (size_t i = 0; i < mAllRitems.size(); ++i)
	{
		auto& ri = mAllRitems[i];
		if (!(ri->LayerFlag & (1 << (int)flag)))
			continue;

		D3D12_PRIMITIVE_TOPOLOGY PrimitiveType;
		if (flag == RenderLayer::Normal
			|| flag == RenderLayer::NormalWireframe
			|| flag == RenderLayer::TreeSprites
			|| flag == RenderLayer::TreeSpritesWireframe)
			PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
		else if (flag == RenderLayer::Tessellation
			|| flag == RenderLayer::TessellationWireframe)
			PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;
		else
			PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		if (mLastVertexBufferView.BufferLocation != ri->Geo->VertexBufferView().BufferLocation)
		{
			mLastVertexBufferView = ri->Geo->VertexBufferView();
			mCommandList->IASetVertexBuffers(0, 1, &mLastVertexBufferView);
		}

		if (mLastIndexBufferView.BufferLocation != ri->Geo->IndexBufferView().BufferLocation)
		{
			mLastIndexBufferView = ri->Geo->IndexBufferView();
			mCommandList->IASetIndexBuffer(&mLastIndexBufferView);
		}
		if (mLastPrimitiveType != PrimitiveType)
		{
			mLastPrimitiveType = PrimitiveType;
			mCommandList->IASetPrimitiveTopology(mLastPrimitiveType);
		}

		// StartInstanceLocation 적용 불가 버그를 해결하기 위해 Constant buffer를 활용함
		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = currInstanceCB->GetGPUVirtualAddress() + i * objCBByteSize;
		mCommandList->SetGraphicsRootConstantBufferView(0, objCBAddress);
		
		if (ri->SkinnedModelInst != nullptr)
		{
			D3D12_GPU_VIRTUAL_ADDRESS skinnedCBAddress = skinnedCB->GetGPUVirtualAddress() + ri->SkinnedCBIndex * skinnedCBByteSize;
			mCommandList->SetGraphicsRootConstantBufferView(2, skinnedCBAddress);
		}
		// DX12 버그로 코드의 Start InstanceLocation이 반영되지 않는다.
		// mCommandList->DrawIndexedInstanced(ri->IndexCount, ri->InstanceCount, ri->StartIndexLocation, ri->BaseVertexLocation, ri->StartInstanceLocation);
		mCommandList->DrawIndexedInstanced(ri->IndexCount, ri->InstanceCount, ri->StartIndexLocation, ri->BaseVertexLocation, 99999);
		// mCommandList->DrawIndexedInstanced(ri->IndexCount, ri->InstanceCount, ri->StartIndexLocation, ri->BaseVertexLocation, 0);	
	}
}

void MyApp::DrawSceneToShadowMap(int index)
{
	D3D12_VIEWPORT viewport = mShadowMap[index]->Viewport();
	D3D12_RECT scissorRect = mShadowMap[index]->ScissorRect();
	mCommandList->RSSetViewports(1, &viewport);
	mCommandList->RSSetScissorRects(1, &scissorRect);

	D3D12_RESOURCE_BARRIER barrier 
		= CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap[index]->Resource(),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			D3D12_RESOURCE_STATE_DEPTH_WRITE);

	mCommandList->ResourceBarrier(1, &barrier);

	// Clear the back buffer and depth buffer.
	auto dsv = mShadowMap[index]->Dsv();
	mCommandList->ClearDepthStencilView(dsv,
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Set null render target because we are only going to draw to
	// depth buffer.  Setting a null render target will disable color writes.
	// Note the active PSO also must specify a render target count of 0.
	mCommandList->OMSetRenderTargets(0, nullptr, false, &dsv);

	// Bind the pass constant buffer for the shadow map pass.
	auto passCB = mCurrFrameResource->PassCB->Resource();
	UINT passCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + (1 + index) * passCBByteSize;
	mCommandList->SetGraphicsRootConstantBufferView(1, passCBAddress);

	mCommandList->SetPipelineState(mPSOs[RenderLayer::ShadowMap].Get());
	DrawRenderItems(RenderLayer::Opaque);
	DrawRenderItems(RenderLayer::AlphaTested);
	DrawRenderItems(RenderLayer::Subdivision);
	mCommandList->SetPipelineState(mPSOs[RenderLayer::SkinnedShadowMap].Get());
	DrawRenderItems(RenderLayer::SkinnedOpaque);

	barrier.Transition.StateBefore = barrier.Transition.StateAfter;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	mCommandList->ResourceBarrier(1, &barrier);

	// mCommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress()); // passCB 복구 로직은 구현하지 않음
}

void MyApp::Pick()
{
	using DirectX::SimpleMath::Ray;
	using DirectX::SimpleMath::Vector3;
	using DirectX::SimpleMath::Matrix;

	float dist = 0.0f;

	const Matrix projRow = mCamera.GetProj4x4f();
	const Matrix viewRow = mCamera.GetView4x4f();
	const Vector3 ndcNear = Vector3(mMouseNdcX, mMouseNdcY, 0.0f);
	const Vector3 ndcFar = Vector3(mMouseNdcX, mMouseNdcY, 1.0f);
	const Matrix invProjView = (viewRow * projRow).Invert();
	const Vector3 worldNear = Vector3::Transform(ndcNear, invProjView);
	const Vector3 worldFar = Vector3::Transform(ndcFar, invProjView);
	Vector3 dir = worldFar - worldNear;
	dir.Normalize();
	const Ray curRay = Ray(worldNear, dir);

	{
		mPickModel = PickClosest(curRay, dist);
		std::cout << "Newly selected model: " << mPickModel.first << " / " << mPickModel.second << std::endl;
	}
}

std::pair<int,int> MyApp::PickClosest(const DirectX::SimpleMath::Ray& pickingRay, float& minDist)
{
	std::pair<int, int> ret(-1,-1);
	minDist = 1e5f;
	for (int i = 0; i < mAllRitems.size(); ++i) {
		for (int j = 0; j < mAllRitems[i]->Datas.size(); ++j) {
			auto& data = mAllRitems[i]->Datas[j];
			float dist = 0.0f;
			if (data.IsPickable &&
				pickingRay.Intersects(data.BoundingSphere, dist) &&
				dist < minDist) {
				ret = { i,j };
				minDist = dist;
			}
		}
	}
	return ret;
}

void MyApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mMouseX = x;
	mMouseY = y;

	// 마우스 커서의 위치를 NDC로 변환
	// 마우스 커서는 좌측 상단 (0, 0), 우측 하단(width-1, height-1)
	// NDC는 좌측 하단이 (-1, -1), 우측 상단(1, 1)
	mMouseNdcX = x * 2.0f / mClientWidth - 1.0f;
	mMouseNdcY = -y * 2.0f / mClientHeight + 1.0f;

	// 커서가 화면 밖으로 나갔을 경우 범위 조절
	// 게임에서는 클램프를 안할 수도 있습니다.
	mMouseNdcX = std::clamp(mMouseNdcX, -1.0f, 1.0f);
	mMouseNdcY = std::clamp(mMouseNdcY, -1.0f, 1.0f);

	if ((btnState & MK_LBUTTON) != 0)
	{
		SetCapture(mHwndWindow);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		Pick();
	}
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
		float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(x - mMouseX));
		float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(y - mMouseY));

		mCamera.Pitch(dy);
		mCamera.RotateY(dx);
	}

	mMouseX = x;
	mMouseY = y;
}

void MyApp::OnKeyboardInput()
{
	const float dt = mTimer.DeltaTime();

	float speed = 30.0f;
	if (GetAsyncKeyState('Q') & 0x8000)
		mCamera.Up(speed * dt);

	if (GetAsyncKeyState('E') & 0x8000)
		mCamera.Up(-speed * dt);

	if (GetAsyncKeyState('W') & 0x8000)
		mCamera.Walk(speed * dt);

	if (GetAsyncKeyState('S') & 0x8000)
		mCamera.Walk(-speed * dt);

	if (GetAsyncKeyState('A') & 0x8000)
		mCamera.Strafe(-speed * dt);

	if (GetAsyncKeyState('D') & 0x8000)
		mCamera.Strafe(speed * dt);

	mCamera.UpdateViewMatrix();

	mCamFrustum.Origin = mCamera.GetPosition3f();
	mCamFrustum.Orientation = mCamera.GetQuaternion();
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
	if (mShowInstanceWindow)
		ShowInstanceWindow();
	if (mShowLightWindow)
		ShowLightWindow();
	if (mShowViewportWindow)
		ShowViewportWindow();
	if (mShowCubeMapWindow)
		ShowCubeMapWindow();
}

void MyApp::ShowMainWindow()
{
	ImGui::Begin("Root");

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Window")) {
		ImGui::Checkbox("Demo Window", &mShowDemoWindow);      // Edit bools storing our window open/close state
		ImGui::Checkbox("Texture", &mShowTextureWindow);
		// ImGui::Checkbox("Material", &mShowMaterialWindow);
		ImGui::Checkbox("Intance", &mShowInstanceWindow);
		ImGui::Checkbox("Light", &mShowLightWindow);
		ImGui::Checkbox("Viewport", &mShowViewportWindow);
		ImGui::Checkbox("Cubemap", &mShowCubeMapWindow);
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
				"1. Opaque",
				"2. SkinnedOpaque",
				"3. Mirror",
				"4. Reflected",
				"5. AlphaTested",
				"6. Transparent",
				"7. Subdivision",
				"8. Normal",
				"9. SkinnedNormal",
				"10. TreeSprites",
				"11. Tessellation",
				"12. BoundingBox",
				"13. BoundingSphere",
				"14. CubeMap",
				"15. DebugShadowMap",
				"16. OpaqueWireframe",
				"17. MirrorWireframe",
				"18. ReflectedWireframe",
				"19. AlphaTestedWireframe",
				"20. TransparentWireframe",
				"21. SubdivisionWireframe",
				"22. NormalWireframe",
				"23. TreeSpritesWireframe",
				"24. TessellationWireframe",
				"25. ShadowMap",
				"26. SkinnedShadowMap",
				"27. AddCS",
				"28. BlurCS",
				"29. WaveCS"
			};
			ImVec2 size(0, 400);
			if (ImGui::BeginListBox("Shader Type", size))
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
			ImGui::SliderInt("Constant Buffer [0, 3]", &mLayerCBIdx[layerIdx], 0, 1 + MAX_LIGHTS - 1, "%d", flags);
		}
		ImGui::TreePop();
	}
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Culling")) {
		int flag = 0;
		flag += ImGui::Checkbox("Enable Culling", &mCullingEnable);
		if (flag)
		{
			for (auto& e : mAllRitems)
			{
				e->NumFramesDirty = APP_NUM_FRAME_RESOURCES;
			}
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
	float widthSize = ImGui::GetColumnWidth();
	float heightSize = widthSize / mAspectRatio;

	int size
		= (int) SRV_USER_SIZE
		+ (int) mDiffuseTex.size()
		+ (int) mNormalTex.size()
		+ (int) MAX_LIGHTS
		+ (int) mTreeMapTex.size()
		+ (int) mCSBlurFilter->DescriptorCount()
		+ (int) mCSWaves->DescriptorCount()
		+ (int) mCubeMapTex.size();
		
	ImGuiSliderFlags flags = ImGuiSliderFlags_None & ~ImGuiSliderFlags_WrapAround;
	ImGui::SliderInt(
		(std::string("Texture [0, ") + std::to_string(size - 1) + "]").c_str(),
		&texIdx,
		0,
		size - 1, "%d", flags);
	ImTextureID my_tex_id = (ImTextureID)mhGPUUser.ptr + mCbvSrvUavDescriptorSize * texIdx;
	ImGui::Text("GPU handle = %p", my_tex_id);
	{
		ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
		ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
		ImVec4 tint_col = true ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // No tint
		ImVec4 border_col = ImGui::GetStyleColorVec4(ImGuiCol_Border);
		ImGui::Image(my_tex_id, ImVec2(widthSize, heightSize), uv_min, uv_max, tint_col, border_col);
	}
	ImGui::End();
}

void MyApp::ShowMaterialWindow()
{
	ImGui::Begin("material", &mShowMaterialWindow);

	static int matIdx;
	static float diff4f[4];
	static float fres3f[3];

	int texDiffSize
		= (int) SRV_USER_SIZE
		+ (int) mDiffuseTex.size();

	int texNormSize
		= (int) mNormalTex.size();


	ImGuiSliderFlags flags = ImGuiSliderFlags_None & ~ImGuiSliderFlags_WrapAround;

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("Select Material")) {
		ImGui::SliderInt(
			(std::string("Material [0, ") + std::to_string(mAllMatItems.size() - 1) + "]").c_str(),
			&matIdx,
			0,
			mAllMatItems.size() - 1,
			"%d",
			flags);

		ImGui::TreePop();
	}
	{
		int flag = 0;
		auto mat = mAllMatItems[matIdx].get();
		{
			// 초기 값 반영
			diff4f[0] = mat->MaterialData.DiffuseAlbedo.x;
			diff4f[1] = mat->MaterialData.DiffuseAlbedo.y;
			diff4f[2] = mat->MaterialData.DiffuseAlbedo.z;
			diff4f[3] = mat->MaterialData.DiffuseAlbedo.w;

			fres3f[0] = mat->MaterialData.FresnelR0.x;
			fres3f[1] = mat->MaterialData.FresnelR0.y;
			fres3f[2] = mat->MaterialData.FresnelR0.z;
		}
		
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("Default Factor")) {
			flag += ImGui::DragFloat4("DiffuseAlbedo R/G/B/A", diff4f, 0.01f, 0.0f, 1.0f);
			flag += ImGui::DragFloat3("Fresne R/G/B", fres3f, 0.01f, 0.0f, 1.0f);
			flag += ImGui::DragFloat("Roughness", &mat->MaterialData.Roughness, 0.01f, 0.0f, 1.0f);
			ImGui::TreePop();
		}

		ImTextureID my_tex_id;
		ImVec4 tint_col = true ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // No tint
		ImVec4 border_col = ImGui::GetStyleColorVec4(ImGuiCol_Border);

		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("Diffuse Map")) {
			flag += ImGui::CheckboxFlags("Use Diffuse(Albedo) Texture", &mat->MaterialData.useAlbedoMap, 1);
			flag += ImGui::SliderInt((std::string("Tex Diffuse Index [0, ") + std::to_string(texDiffSize - 1) + "]").c_str(), &mat->MaterialData.DiffMapIndex, 0, texDiffSize - 1, "%d", flags);
			my_tex_id = (ImTextureID)mhGPUDiff.ptr + mCbvSrvUavDescriptorSize * mat->MaterialData.DiffMapIndex;
			ImGui::Image(my_tex_id, ImVec2(mImguiWidth, mImguiHeight), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), tint_col, border_col);

			ImGui::TreePop();
		}

		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("Normal Map")) {
			flag += ImGui::CheckboxFlags("Use Normal Texture", &mat->MaterialData.useNormalMap, 1);
			flag += ImGui::SliderInt((std::string("Tex Normal Index [0, ") + std::to_string(texNormSize - 1) + "]").c_str(), &mat->MaterialData.NormMapIndex, 0, texNormSize - 1, "%d", flags);
			my_tex_id = (ImTextureID)mhGPUNorm.ptr + mCbvSrvUavDescriptorSize * mat->MaterialData.NormMapIndex;
			ImGui::Image(my_tex_id, ImVec2(mImguiWidth, mImguiHeight), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), tint_col, border_col);

			ImGui::TreePop();
		}

		//ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		//if (ImGui::TreeNode("Normal Map")) {
		//	flag += ImGui::CheckboxFlags("Use Normal Texture", &mat->MaterialData.useNormalMap, 1);
		//	flag += ImGui::SliderInt((std::string("Tex Normal Index [0, ") + std::to_string(texNormSize - 1) + "]").c_str(), &mat->MaterialData.NormMapIndex, 0, texNormSize - 1, "%d", flags);
		//	my_tex_id = (ImTextureID)mhGPUNorm.ptr + mCbvSrvUavDescriptorSize * mat->MaterialData.NormMapIndex;
		//	ImGui::Image(my_tex_id, ImVec2(mImguiWidth, mImguiHeight), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), tint_col, border_col);

		//	ImGui::TreePop();
		//}
		

		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("Alpha Test")) {
			flag += ImGui::CheckboxFlags("Use Alpha Test", &mat->MaterialData.useAlphaTest, 1);

			ImGui::TreePop();
		}

		if (flag)
		{
			mat->MaterialData.DiffuseAlbedo = { diff4f[0],diff4f[1],diff4f[2],diff4f[3] };
			mat->MaterialData.FresnelR0 = { fres3f[0], fres3f[1], fres3f[2] };
			mat->NumFramesDirty = APP_NUM_FRAME_RESOURCES;
		}
	}
	ImGui::End();
}

void MyApp::ShowInstanceWindow()
{
	ImGui::Begin("render item", &mShowInstanceWindow);

	static float world3f[3];
	static float scale3f[3];
	static float angle3f[3];
	static float texScale3f[3];
	static float displacementSize2f[2];
	static float diff4f[4];
	static float fres3f[3];

	int texDiffSize
		= (int)SRV_USER_SIZE
		+ (int)mDiffuseTex.size();

	int texNormSize
		= (int)mNormalTex.size();

	ImGuiSliderFlags flags = ImGuiSliderFlags_None & ~ImGuiSliderFlags_WrapAround;

	ImTextureID my_tex_id;
	ImVec4 tint_col = true ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // No tint
	ImVec4 border_col = ImGui::GetStyleColorVec4(ImGuiCol_Border);

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Select Instance")) {
		if (ImGui::SliderInt((std::string("Render Items [0, ") + std::to_string(mAllRitems.size() - 1) + "]").c_str(), &mPickModel.first, 0, mAllRitems.size() - 1, "%d", flags))
		{
			mPickModel.second = 0;
		}
			
		ImGui::SliderInt((std::string("Instance [0, ") + std::to_string(mAllRitems[mPickModel.first]->Datas.size() - 1) + "]").c_str(),
			&mPickModel.second,
			0,
			mAllRitems[mPickModel.first]->Datas.size() - 1,
			"%d",
			flags);

		ImGui::TreePop();
	}

	RenderItem* ritm = mAllRitems[mPickModel.first].get();

	if(ritm->Datas.size())
	{
		EXInstanceData& inst = ritm->Datas[mPickModel.second];
		// 초기 값 반영
		world3f[0] = inst.Translation.x;
		world3f[1] = inst.Translation.y;
		world3f[2] = inst.Translation.z;
		scale3f[0] = inst.Scale.x;
		scale3f[1] = inst.Scale.y;
		scale3f[2] = inst.Scale.z;
		angle3f[0] = inst.Rotate.x;
		angle3f[1] = inst.Rotate.y;
		angle3f[2] = inst.Rotate.z;
		texScale3f[0] = inst.TexScale.x;
		texScale3f[1] = inst.TexScale.y;
		texScale3f[2] = inst.TexScale.z;
		float widthSize = ImGui::GetColumnWidth();
		float heightSize = widthSize / mAspectRatio;
		int flag = 0;

		
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("Scale")) {
			flag += ImGui::DragFloat3("World x/y/z", world3f, 0.1f, -FLT_MAX / 2, FLT_MAX / 2);
			flag += ImGui::DragFloat3("Scale x/y/z", scale3f, 0.1f, -FLT_MAX / 2, FLT_MAX / 2);
			flag += ImGui::DragFloat3("angle x/y/z", angle3f, 5.0f, -FLT_MAX / 2, FLT_MAX / 2);
			flag += ImGui::DragFloat3("Texture Scale x/y/z", texScale3f, 0.01f, -FLT_MAX / 2, FLT_MAX / 2);
			
			flag += ImGui::Checkbox("Use Quaternion", &inst.useQuat);

			ImGui::TreePop();
		}
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("Displacement Map")) {
			int dummy = 0;
			flag += ImGui::CheckboxFlags("Use DisplacementMap Test", &inst.InstanceData.useDisplacementMap, 1);
			flag += ImGui::SliderInt(std::string("DisplacementMap [0, 0]").c_str(), &dummy, 0, 0, "%d", flags);
			flag += ImGui::DragFloat2("Displacement Size", displacementSize2f, 0.01f, 0.0f, 10.0f);
			flag += ImGui::DragFloat("Displacement Scale", &inst.InstanceData.GridSpatialStep, 0.01f, 0.0f, 10.0f);

			my_tex_id = (ImTextureID) mCSWaves->DisplacementMap().ptr;
			ImGui::Image(my_tex_id, ImVec2(widthSize, heightSize), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), tint_col, border_col);

			ImGui::TreePop();
		}
		if (flag)
		{
			inst.UpdateTranslation({ world3f[0], world3f[1], world3f[2] });
			inst.UpdateScale({ scale3f[0], scale3f[1], scale3f[2] });
			inst.UpdateRotate({ angle3f[0], angle3f[1], angle3f[2] });
			inst.UpdateTexScale({ texScale3f[0], texScale3f[1], texScale3f[2] });
			for (auto& d : mAllRitems)
				d->NumFramesDirty = APP_NUM_FRAME_RESOURCES;
		}

		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode("Material")) {
			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Select Material")) {
				flag = 0;
				flag += ImGui::SliderInt(
					(std::string("Material [0, ") + std::to_string(mAllMatItems.size() - 1) + "]").c_str(),
					&inst.InstanceData.MaterialIndex,
					0,
					mAllMatItems.size() - 1,
					"%d",
					flags);
				ImGui::TreePop();
			}
			auto mat = mAllMatItems[inst.InstanceData.MaterialIndex].get();
			{
				// 초기 값 반영
				diff4f[0] = mat->MaterialData.DiffuseAlbedo.x;
				diff4f[1] = mat->MaterialData.DiffuseAlbedo.y;
				diff4f[2] = mat->MaterialData.DiffuseAlbedo.z;
				diff4f[3] = mat->MaterialData.DiffuseAlbedo.w;

				fres3f[0] = mat->MaterialData.FresnelR0.x;
				fres3f[1] = mat->MaterialData.FresnelR0.y;
				fres3f[2] = mat->MaterialData.FresnelR0.z;
			}

			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Default Factor")) {
				flag += ImGui::DragFloat4("DiffuseAlbedo R/G/B/A", diff4f, 0.01f, 0.0f, 1.0f);
				flag += ImGui::DragFloat3("Fresne R/G/B", fres3f, 0.01f, 0.0f, 1.0f);
				flag += ImGui::DragFloat("Roughness", &mat->MaterialData.Roughness, 0.01f, 0.0f, 1.0f);
				ImGui::TreePop();
			}
			;
			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Diffuse Map")) {
				flag += ImGui::CheckboxFlags("Use Diffuse(Albedo) Texture", &mat->MaterialData.useAlbedoMap, 1);
				flag += ImGui::SliderInt((std::string("Tex Diffuse Index [0, ") + std::to_string(texDiffSize - 1) + "]").c_str(), &mat->MaterialData.DiffMapIndex, 0, texDiffSize - 1, "%d", flags);
				my_tex_id = (ImTextureID)mhGPUUser.ptr + mCbvSrvUavDescriptorSize * mat->MaterialData.DiffMapIndex;
				ImGui::Image(my_tex_id, ImVec2(widthSize, heightSize), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), tint_col, border_col);

				ImGui::TreePop();
			}

			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Normal Map")) {
				flag += ImGui::CheckboxFlags("Use Normal Texture", &mat->MaterialData.useNormalMap, 1);
				flag += ImGui::SliderInt((std::string("Tex Normal Index [0, ") + std::to_string(texNormSize - 1) + "]").c_str(), &mat->MaterialData.NormMapIndex, 0, texNormSize - 1, "%d", flags);
				my_tex_id = (ImTextureID)mhGPUNorm.ptr + mCbvSrvUavDescriptorSize * mat->MaterialData.NormMapIndex;
				ImGui::Image(my_tex_id, ImVec2(widthSize, heightSize), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), tint_col, border_col);

				ImGui::TreePop();
			}

			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::TreeNode("Alpha Test")) {
				flag += ImGui::CheckboxFlags("Use Alpha Test", &mat->MaterialData.useAlphaTest, 1);

				ImGui::TreePop();
			}
			ImGui::TreePop();
			if (flag)
			{
				for (auto& d : mAllRitems)
				 	d->NumFramesDirty = APP_NUM_FRAME_RESOURCES;

				mat->MaterialData.DiffuseAlbedo = { diff4f[0],diff4f[1],diff4f[2],diff4f[3] };
				mat->MaterialData.FresnelR0 = { fres3f[0], fres3f[1], fres3f[2] };
				mat->NumFramesDirty = APP_NUM_FRAME_RESOURCES;
			}
		}
	}
	else
	{
		ImGui::Text("have no item");
	}

	

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

	ImGui::End();
}

void MyApp::ShowLightWindow()
{
	ImGui::Begin("light", &mShowLightWindow);

	static int mapIdx = 0;
	ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
	ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
	ImVec4 tint_col = true ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // No tint
	ImVec4 border_col = ImGui::GetStyleColorVec4(ImGuiCol_Border);
	ImTextureID my_tex_id;
	float widthSize = ImGui::GetColumnWidth();
	float heightSize = widthSize / mAspectRatio;
	
	int mapSize = MAX_LIGHTS;

	ImGuiSliderFlags flags = ImGuiSliderFlags_None & ~ImGuiSliderFlags_WrapAround;

	static float ambientLight4f[4];
	static float strength3f[3];
	static float baseDir3f[3];
	static float targetPos3f[3];
	static float rotate;


	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Light")) {
		ImGui::SliderInt((std::string("Light [0, ") + std::to_string(mapSize - 1) + "]").c_str(), &mapIdx, 0, mapSize - 1, "%d", flags);

		

		if (mapIdx < MAX_LIGHTS && mShadowMap[mapIdx])
		{
			ImGui::Checkbox("Use Light", &mUseShadowMap[mapIdx]);

			float orthoBoxLength = mShadowMap[mapIdx]->GetBoxLength();
			DirectX::SimpleMath::Vector4 ambientLight = mShadowMap[mapIdx]->GetAmbientLight();
			DirectX::SimpleMath::Vector3 strength = mShadowMap[mapIdx]->GetLightStrength();
			DirectX::SimpleMath::Vector3 baseDir = mShadowMap[mapIdx]->GetBaseDir();
			DirectX::SimpleMath::Vector3 targetPos = mShadowMap[mapIdx]->GetTarget();

			{
				ambientLight4f[0] = ambientLight.x;
				ambientLight4f[1] = ambientLight.y;
				ambientLight4f[2] = ambientLight.z;
				ambientLight4f[3] = ambientLight.w;
				strength3f[0] = strength.x;
				baseDir3f[0] = baseDir.x;
				targetPos3f[0] = targetPos.x;
				strength3f[1] = strength.y;
				baseDir3f[1] = baseDir.y;
				targetPos3f[1] = targetPos.y;
				strength3f[2] = strength.z;
				baseDir3f[2] = baseDir.z;
				targetPos3f[2] = targetPos.z;
			}

			if (ImGui::DragFloat("Light Length", &orthoBoxLength, 0.1f, -FLT_MAX / 2, FLT_MAX / 2))
				mShadowMap[mapIdx]->SetBoxLength(orthoBoxLength);
			if (ImGui::DragFloat4("ambient light r/g/b", ambientLight4f, 0.01f, 0.0f, 1.0f))
				mShadowMap[mapIdx]->SetAmbientLight({ ambientLight4f[0], ambientLight4f[1], ambientLight4f[2], ambientLight4f[3] });
			if (ImGui::DragFloat3("strength r/g/b", strength3f, 0.01f, 0.0f, 1.0f))
				mShadowMap[mapIdx]->SetLightStrength({ strength3f[0], strength3f[1], strength3f[2] });
			if (ImGui::DragFloat3("base direction x/y/z", baseDir3f, 0.1f, -FLT_MAX / 2, FLT_MAX / 2))
				mShadowMap[mapIdx]->SetBaseDir({ baseDir3f[0], baseDir3f[1], baseDir3f[2] });
			if (ImGui::DragFloat("Rotate", &rotate, 0.1f, -FLT_MAX / 2, FLT_MAX / 2))
				mShadowMap[mapIdx]->SetRotate(rotate);
			if (ImGui::DragFloat3("target position x/y/z", targetPos3f, 0.01f, -FLT_MAX / 2, FLT_MAX / 2))
				mShadowMap[mapIdx]->SetTarget({ targetPos3f[0], targetPos3f[1], targetPos3f[2] });
		}
		ImGui::TreePop();
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Shadow Map")) {
		my_tex_id = (ImTextureID)mhGPUShadow.ptr + mCbvSrvUavDescriptorSize * mapIdx;
		ImGui::Image(my_tex_id, ImVec2(widthSize, heightSize), uv_min, uv_max, tint_col, border_col);
		ImGui::TreePop();
	}

	ImGui::End();
}

void MyApp::ShowViewportWindow()
{
	ImGui::Begin("viewport1", &mShowViewportWindow);

	float widthSize = ImGui::GetColumnWidth();
	float heightSize = widthSize / mAspectRatio;

	ImTextureID my_tex_id = (ImTextureID)mhGPUUser.ptr + mCbvSrvUavDescriptorSize;
	{
		ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
		ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
		ImVec4 tint_col = true ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // No tint
		ImVec4 border_col = ImGui::GetStyleColorVec4(ImGuiCol_Border);
		ImGui::Image(my_tex_id, ImVec2(widthSize, heightSize), uv_min, uv_max, tint_col, border_col);
	}
	ImGui::End();
}

void MyApp::ShowCubeMapWindow()
{
	ImGui::Begin("cubemap", &mShowCubeMapWindow);

	static int idx;

	ImGui::LabelText("label", "Value");
	ImGui::SeparatorText("Inputs");
	ImGuiSliderFlags flags = ImGuiSliderFlags_None & ~ImGuiSliderFlags_WrapAround;
	int flag = 0;
	flag += ImGui::SliderInt((std::string("Cubemap [0, ") + std::to_string((UINT)mCubeMapTex.size() - 1) + "]").c_str(), &idx, 0, (UINT)mCubeMapTex.size() - 1, "%d", flags);
	if (flag)
	{
		mMainPassCB.gCubeMapIndex = idx;
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
