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
	mLayerType[8] = RenderLayer::WaveVS_CS;
	mLayerType[9] = RenderLayer::CubeMap;

	mLayerStencil[0] = 0;
	mLayerStencil[1] = 0;
	mLayerStencil[2] = 1;
	mLayerStencil[3] = 1;
	mLayerStencil[4] = 0;
	mLayerStencil[5] = 0;
	mLayerStencil[6] = 0;
	mLayerStencil[7] = 0;
	mLayerStencil[8] = 0;
	mLayerStencil[9] = 0;

	mLayerCBIdx[0] = 0;
	mLayerCBIdx[1] = 0;
	mLayerCBIdx[2] = 1;
	mLayerCBIdx[3] = 1;
	mLayerCBIdx[4] = 0;
	mLayerCBIdx[5] = 0;
	mLayerCBIdx[6] = 0;
	mLayerCBIdx[7] = 0;
	mLayerCBIdx[8] = 0;
	mLayerCBIdx[9] = 0;

	for (int i = 10; i < MAX_LAYER_DEPTH; ++i)
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

	mCamera.SetPosition(0.0f, 2.0f, -15.0f);
	mCSBlurFilter = std::make_unique<BlurFilter>(mDevice.Get(), mClientWidth, mClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
	mCSAdd = std::make_unique<CSAdd>(mDevice.Get(), mCommandList.Get());
	mCSWaves = std::make_unique<GpuWaves>(mDevice.Get(), mCommandList.Get(), 256, 256, 0.25f, 0.03f, 2.0f, 0.2f, m4xMsaaState, m4xMsaaQuality);

	LoadTextures();
	BuildMaterials();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildMeshes();
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
	static const std::wstring TEXTURE_DIR = L"../Data/Textures/";
	for (const auto& a : TEX_DIFF_FILENAMES)
	{
		auto texture = std::make_unique<Texture>();
		texture->Filename = TEXTURE_DIR + a;
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(mDevice.Get(), mCommandList.Get(), texture->Filename.c_str(), texture->Resource, texture->UploadHeap));

		mTextures[a] = std::move(texture);
	}

	for (const auto& a : TEX_NORM_FILENAMES)
	{
		auto texture = std::make_unique<Texture>();
		texture->Filename = TEXTURE_DIR + a;
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(mDevice.Get(), mCommandList.Get(), texture->Filename.c_str(), texture->Resource, texture->UploadHeap));

		mTextures[a] = std::move(texture);
	}
	for (const auto& a : TEX_ARRAY_FILENAMES)
	{
		auto texture = std::make_unique<Texture>();
		texture->Filename = TEXTURE_DIR + a;
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(mDevice.Get(), mCommandList.Get(), texture->Filename.c_str(), texture->Resource, texture->UploadHeap));

		mTextures[a] = std::move(texture);
	}

	for (const auto& a : TEX_CUBE_FILENAMES)
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
	D3D12_DESCRIPTOR_RANGE TexDiffTable // register t0[16] (Space0)
	{
		/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		/* UINT NumDescriptors						*/.NumDescriptors = (UINT)TEX_DIFF_FILENAMES.size() + SRV_USER_SIZE,
		/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		/* UINT RegisterSpace						*/.RegisterSpace = 0,
		/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};
	D3D12_DESCRIPTOR_RANGE TexNormTable // register t0[16] (Space1)
	{
		/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		/* UINT NumDescriptors						*/.NumDescriptors = (UINT)TEX_DIFF_FILENAMES.size() + SRV_USER_SIZE,
		/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		/* UINT RegisterSpace						*/.RegisterSpace = 1,
		/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};
	D3D12_DESCRIPTOR_RANGE TexArrayTable // register t0[0] (Space2)
	{
		/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		/* UINT NumDescriptors						*/.NumDescriptors = (UINT)TEX_ARRAY_FILENAMES.size(),
		/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		/* UINT RegisterSpace						*/.RegisterSpace = 2,
		/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};
	D3D12_DESCRIPTOR_RANGE TexCubeTable // register t0[0] (Space3)
	{
		/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		/* UINT NumDescriptors						*/.NumDescriptors = (UINT)TEX_CUBE_FILENAMES.size(),
		/* UINT BaseShaderRegister					*/.BaseShaderRegister = 0,
		/* UINT RegisterSpace						*/.RegisterSpace = 3,
		/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};
	D3D12_DESCRIPTOR_RANGE DisplacementMapTable // register t2 (Space4)
	{
		/* D3D12_DESCRIPTOR_RANGE_TYPE RangeType	*/.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		/* UINT NumDescriptors						*/.NumDescriptors = 1,
		/* UINT BaseShaderRegister					*/.BaseShaderRegister = 2,
		/* UINT RegisterSpace						*/.RegisterSpace = 4,
		/* UINT OffsetInDescriptorsFromTableStart	*/.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
	};
	

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[9];

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

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsConstantBufferView(1);		// gBaseInstanceIndex b1
	slotRootParameter[1].InitAsShaderResourceView(0, 4);	// InstanceData t0 (Space4)
	slotRootParameter[2].InitAsShaderResourceView(1, 4);	// MaterialData t1 (Space4)
	slotRootParameter[3].InitAsConstantBufferView(0);		// cbPass b0
	slotRootParameter[4].InitAsDescriptorTable(1, &TexDiffTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[5].InitAsDescriptorTable(1, &TexNormTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[6].InitAsDescriptorTable(1, &TexArrayTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[7].InitAsDescriptorTable(1, &TexCubeTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[8].InitAsDescriptorTable(1, &DisplacementMapTable, D3D12_SHADER_VISIBILITY_ALL);
	

	auto staticSamplers = D3DUtil::GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(9, slotRootParameter, (UINT)staticSamplers.size(), staticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
	// 2. Texture Data (TEX_DIFF_FILENAMES.size())
	// 3. Viewport Buffer (SRV_USER_SIZE)
	// 4. CS Blar Buffer (SRV_CS_BLAR_SIZE(4))
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc
	{
		/* D3D12_DESCRIPTOR_HEAP_TYPE Type	*/.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		/* UINT NumDescriptors				*/.NumDescriptors 
												= SRV_IMGUI_SIZE 
												+ SRV_USER_SIZE 
												+ (UINT)TEX_DIFF_FILENAMES.size() 
												+ (UINT)TEX_NORM_FILENAMES.size()
												+ (UINT)TEX_ARRAY_FILENAMES.size() 
												+ (UINT)TEX_CUBE_FILENAMES.size() 
												+ mCSBlurFilter->DescriptorCount() 
												+ mCSWaves->DescriptorCount(),
		/* D3D12_DESCRIPTOR_HEAP_FLAGS Flags*/.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		/* UINT NodeMask					*/.NodeMask = 0
	};
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	BuildShaderResourceViews();
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

	CD3DX12_CPU_DESCRIPTOR_HANDLE hCPUDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), SRV_IMGUI_SIZE, mCbvSrvUavDescriptorSize);
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGPUDescriptor(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), SRV_IMGUI_SIZE, mCbvSrvUavDescriptorSize);
	mhGPUUser = hGPUDescriptor;
	mhGPUDiff = hGPUDescriptor;

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

	for (int i = 0; i < TEX_DIFF_FILENAMES.size(); ++i)
	{
		auto texture = mTextures[TEX_DIFF_FILENAMES[i]]->Resource;

		srvDesc.Format = texture->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;
		mDevice->CreateShaderResourceView(texture.Get(), &srvDesc, hCPUDescriptor);
		hCPUDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
		hGPUDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
	}

	mhGPUNorm = hGPUDescriptor;
	for (int i = 0; i < TEX_NORM_FILENAMES.size(); ++i)
	{
		auto texture = mTextures[TEX_NORM_FILENAMES[i]]->Resource;

		srvDesc.Format = texture->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;
		mDevice->CreateShaderResourceView(texture.Get(), &srvDesc, hCPUDescriptor);
		hCPUDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
		hGPUDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
	}

	mhGPUArray = hGPUDescriptor;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2DArray.MipLevels = -1;
	srvDesc.Texture2DArray.FirstArraySlice = 0;
	for (int i = 0; i < TEX_ARRAY_FILENAMES.size(); ++i)
	{
		auto texture = mTextures[TEX_ARRAY_FILENAMES[i]]->Resource;

		srvDesc.Format = texture->GetDesc().Format;
		srvDesc.Texture2DArray.ArraySize = texture->GetDesc().DepthOrArraySize;
		mDevice->CreateShaderResourceView(texture.Get(), &srvDesc, hCPUDescriptor);
		hCPUDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
		hGPUDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
	}

	mCSBlurFilter->BuildDescriptors(hCPUDescriptor, hGPUDescriptor, mCbvSrvUavDescriptorSize);
	hCPUDescriptor.Offset(mCSBlurFilter->DescriptorCount(), mCbvSrvUavDescriptorSize);
	hGPUDescriptor.Offset(mCSBlurFilter->DescriptorCount(), mCbvSrvUavDescriptorSize);

	mCSWaves->BuildDescriptors(hCPUDescriptor, hGPUDescriptor, mCbvSrvUavDescriptorSize);
	hCPUDescriptor.Offset(mCSWaves->DescriptorCount(), mCbvSrvUavDescriptorSize);
	hGPUDescriptor.Offset(mCSWaves->DescriptorCount(), mCbvSrvUavDescriptorSize);

	mhGPUCube = hGPUDescriptor;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	for (int i = 0; i < TEX_CUBE_FILENAMES.size(); ++i)
	{
		auto texture = mTextures[TEX_CUBE_FILENAMES[i]]->Resource;

		srvDesc.TextureCube.MipLevels = texture->GetDesc().MipLevels;
		srvDesc.Format = texture->GetDesc().Format;
		mDevice->CreateShaderResourceView(texture.Get(), &srvDesc, hCPUDescriptor);
		hCPUDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
		hGPUDescriptor.Offset(1, mCbvSrvUavDescriptorSize);
	}
}

void MyApp::BuildShadersAndInputLayout()
{
	char texDiffSize[100];
	char texNormSize[100];
	char texArraySize[100];
	char texCubeSize[100];
	strcpy_s(texDiffSize, std::to_string(TEX_DIFF_FILENAMES.size() + SRV_USER_SIZE).c_str());
	strcpy_s(texNormSize, std::to_string(TEX_NORM_FILENAMES.size()).c_str());
	strcpy_s(texArraySize, std::to_string(TEX_ARRAY_FILENAMES.size()).c_str());
	strcpy_s(texCubeSize, std::to_string(TEX_CUBE_FILENAMES.size()).c_str());
	const D3D_SHADER_MACRO defines[] =
	{
		"TEX_DIFF_SIZE", texDiffSize,
		"TEX_NORM_SIZE", texNormSize,
		"TEX_ARRAY_SIZE", texArraySize,
		"TEX_CUBE_SIZE", texCubeSize,
		NULL, NULL
	};

	const D3D_SHADER_MACRO fogDefines[] =
	{
		"TEX_DIFF_SIZE", texDiffSize,
		"TEX_NORM_SIZE", texNormSize,
		"TEX_ARRAY_SIZE", texArraySize,
		"TEX_CUBE_SIZE", texCubeSize,
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"TEX_DIFF_SIZE", texDiffSize,
		"TEX_NORM_SIZE", texNormSize,
		"TEX_ARRAY_SIZE", texArraySize,
		"TEX_CUBE_SIZE", texCubeSize,
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO waveDefines[] =
	{
		"TEX_DIFF_SIZE", texDiffSize,
		"TEX_NORM_SIZE", texNormSize,
		"TEX_ARRAY_SIZE", texArraySize,
		"TEX_CUBE_SIZE", texCubeSize,
		"DISPLACEMENT_MAP", "1",
		NULL, NULL
	};

	// 오직 Release 모드에서만 동작이 가능함 (Timeout 발생)

	mShaders["MainVS"] = D3DUtil::CompileShader(L"Main.hlsl", defines, "VS", "vs_5_1");
	mShaders["DisplacementVS"] = D3DUtil::CompileShader(L"Main.hlsl", waveDefines, "VS", "vs_5_1");
	mShaders["MainPS"] = D3DUtil::CompileShader(L"Main.hlsl", fogDefines, "PS", "ps_5_1");
	mShaders["AlphaTestedPS"] = D3DUtil::CompileShader(L"Main.hlsl", alphaTestDefines, "PS", "ps_5_1");
	mShaders["DisplacementPS"] = D3DUtil::CompileShader(L"Main.hlsl", waveDefines, "PS", "ps_5_1");

	mShaders["NormalVS"] = D3DUtil::CompileShader(L"Normal.hlsl", defines, "VS", "vs_5_1");
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
	mShaders["TessPS"] = D3DUtil::CompileShader(L"Tessellation.hlsl", fogDefines, "PS", "ps_5_1");

	mShaders["BasicVS"] = D3DUtil::CompileShader(L"Basic.hlsl", defines, "VS", "vs_5_1");
	mShaders["BasicPS"] = D3DUtil::CompileShader(L"Basic.hlsl", defines, "PS", "ps_5_1");

	mShaders["CubeMapVS"] = D3DUtil::CompileShader(L"CubeMap.hlsl", defines, "VS", "vs_5_1");
	mShaders["CubeMapPS"] = D3DUtil::CompileShader(L"CubeMap.hlsl", defines, "PS", "ps_5_1");

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
}

GeometryGenerator::MeshData MyApp::LoadModelMesh(std::wstring dir)
{
	using namespace DirectX;
	GeometryGenerator::MeshData mesh;

	std::ifstream fin(dir);

	if (!fin)
	{
		MessageBox(0, dir.c_str(), 0, 0);
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

void MyApp::BuildMeshes()
{
	//static const std::wstring MESH_MODEL_DIR = L"../Data/Models/";
	//static const std::vector<std::wstring> MESH_MODEL_FILE_NAMES = {
	//	L"skull.txt", L""
	//};

	mMeshes.push_back(GeometryGenerator::CreateBox(1.0f, 1.0f, 1.0f, 3));
	mMeshes.push_back(GeometryGenerator::CreateGrid(20.0f, 30.0f, 60, 40));
	mMeshes.push_back(GeometryGenerator::CreateSphere(0.5f, 20, 20));
	mMeshes.push_back(GeometryGenerator::CreateCylinder(0.3f, 0.5f, 3.0f, 20, 20));
	mMeshes.push_back(GeometryGenerator::CreateGrid(160.0f, 160.0f, 50, 50));
	for (size_t i = 0; i < mMeshes[4].Vertices.size(); ++i)
	{
		mMeshes[4].Vertices[i].Position.y = GetHillsHeight(mMeshes[4].Vertices[i].Position.x, mMeshes[4].Vertices[i].Position.z);
		mMeshes[4].Vertices[i].Normal = GetHillsNormal(mMeshes[4].Vertices[i].Position.x, mMeshes[4].Vertices[i].Position.z);
	}

	mMeshes.push_back(GeometryGenerator::CreateGrid(160.0f, 160.0f, mCSWaves->RowCount(), mCSWaves->ColumnCount()));
	mMeshes.push_back(LoadModelMesh(L"../Data/Models/skull.txt"));
	mMeshes.push_back(LoadModelMesh(L"../Data/Models/car.txt"));

	UpdateTangents();

	BuildGeometry(mMeshes);
}

void MyApp::BuildGeometry(std::vector<GeometryGenerator::MeshData>& meshes, const DXGI_FORMAT indexFormat)
{
	using namespace DirectX;
	//=========================================================
	// Part 1-1. Culling을 위한 Bounding Box 생성
	//=========================================================
	std::vector<DirectX::BoundingBox> BoundingBox(meshes.size());
	std::vector<DirectX::BoundingSphere> BoundingSphere(meshes.size());
	DirectX::XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
	DirectX::XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

	for (size_t i = 0; i < BoundingBox.size(); ++i)
	{
		DirectX::XMVECTOR vMin = XMLoadFloat3(&vMinf3);
		DirectX::XMVECTOR vMax = XMLoadFloat3(&vMaxf3);
		for (size_t j = 0; j < meshes[i].Vertices.size(); ++j)
		{
			DirectX::XMVECTOR P = XMLoadFloat3(&meshes[i].Vertices[j].Position);
			vMin = DirectX::XMVectorMin(vMin, P);
			vMax = DirectX::XMVectorMax(vMax, P);
		}
		DirectX::SimpleMath::Vector3 center = (vMin + vMax) * 0.5f;

		BoundingBox[i].Center = center;
		DirectX::XMStoreFloat3(&BoundingBox[i].Extents, 0.5f * (vMax - vMin));

		float maxRadius = 0.0f;
		for (size_t j = 0; j < meshes[i].Vertices.size(); ++j)
		{
			maxRadius = max(maxRadius, (center - meshes[i].Vertices[j].Position).Length());
		}
		maxRadius += 1e-2f;
		BoundingSphere[i].Center = center;
		BoundingSphere[i].Radius = maxRadius;
	}
	//=========================================================
	// Part 2. vertices & indices 생성
	//=========================================================
	size_t totalVertexCount = 0;
	for (const auto& a : meshes)
		totalVertexCount += a.Vertices.size();
	UINT k = 0;
	std::vector<Vertex> vertices(totalVertexCount);
	std::vector<std::uint16_t> indices;
	for (size_t i = 0; i < meshes.size(); ++i)
	{
		indices.insert(indices.end(), meshes[i].GetIndices16().begin(), meshes[i].GetIndices16().end());
		for (const auto& ver : meshes[i].Vertices)
		{
			vertices[k].Position = ver.Position;
			vertices[k].Normal = ver.Normal;
			vertices[k].TangentU = ver.TangentU;
			vertices[k++].TexC = ver.TexC;
		}
	}

	//=========================================================
	// Part 2. SubmeshGeometry 생성
	//=========================================================
	std::vector<SubmeshGeometry> submeshes(meshes.size());
	for (size_t i = 0; i < meshes.size(); ++i)
	{
		submeshes[i].BoundingBox = BoundingBox[i];
		submeshes[i].BoundingSphere = BoundingSphere[i];
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
	}

	//=========================================================
	// Part 3. GPU 할당 (16bit)
	//=========================================================
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = mGeometries.size();
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
	geo->VertexBufferGPU = D3DUtil::CreateDefaultBuffer(mDevice.Get(), mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = D3DUtil::CreateDefaultBuffer(mDevice.Get(), mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = indexFormat;
	geo->IndexBufferByteSize = ibByteSize;

	for (size_t i = 0; i < submeshes.size(); ++i)
	{
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
	for (size_t i = 0; i < TEX_DIFF_FILENAMES.size(); ++i)
	{
		auto mat = std::make_unique<Material>();
		mat->Name = MATERIAL_NAMES[idx];
		mat->MatCBIndex = idx;
		mat->DiffuseSrvHeapIndex = SRV_USER_SIZE + idx++;
		mat->NormalSrvHeapIndex = 0;
		mat->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mat->FresnelR0 = DirectX::XMFLOAT3(0.05f, 0.05f, 0.05f);
		mat->Roughness = 0.1f;
		mMaterials[mat->Name] = std::move(mat);
	}
	for (size_t i = 0; i < TEX_ARRAY_FILENAMES.size(); ++i)
	{
		auto mat = std::make_unique<Material>();
		mat->Name = MATERIAL_NAMES[idx];
		mat->MatCBIndex = idx++;
		mat->DiffuseSrvHeapIndex = 0;
		mat->NormalSrvHeapIndex = 0;
		mat->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		mat->FresnelR0 = DirectX::XMFLOAT3(0.02f, 0.02f, 0.02f);
		mat->Roughness = 0.1f;
		mMaterials[mat->Name] = std::move(mat);
	}
	{
		mMaterials["bricks3"]->DiffuseAlbedo = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		mMaterials["bricks3"]->FresnelR0 = DirectX::XMFLOAT3(0.97f, 0.97f, 0.97f);
		mMaterials["water1"]->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
		mMaterials["ice"]->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.3f);

		mMaterials["bricks"]->NormalSrvHeapIndex = 1;
		mMaterials["bricks2"]->NormalSrvHeapIndex = 2;
		mMaterials["tile"]->NormalSrvHeapIndex = 3;
	}
	

	auto skullMat = std::make_unique<Material>();
	skullMat->Name = MATERIAL_NAMES[idx];
	skullMat->MatCBIndex = idx++;
	skullMat->DiffuseSrvHeapIndex = SRV_USER_SIZE;
	skullMat->NormalSrvHeapIndex = 0;
	skullMat->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	skullMat->FresnelR0 = DirectX::XMFLOAT3(0.05f, 0.05f, 0.05f);
	skullMat->Roughness = 0.3f;
	mMaterials[skullMat->Name] = std::move(skullMat);

	auto shadowMat = std::make_unique<Material>();
	shadowMat->Name = MATERIAL_NAMES[idx];
	shadowMat->MatCBIndex = idx++;
	shadowMat->DiffuseSrvHeapIndex = SRV_USER_SIZE;
	shadowMat->NormalSrvHeapIndex = 0;
	shadowMat->DiffuseAlbedo = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
	shadowMat->FresnelR0 = DirectX::XMFLOAT3(0.001f, 0.001f, 0.001f);
	shadowMat->Roughness = 0.0f;
	mMaterials[shadowMat->Name] = std::move(shadowMat);

	auto viewportMat = std::make_unique<Material>();
	viewportMat->Name = MATERIAL_NAMES[idx];
	viewportMat->MatCBIndex = idx++;
	viewportMat->DiffuseSrvHeapIndex = 0;
	viewportMat->NormalSrvHeapIndex = 0;
	viewportMat->DiffuseAlbedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	viewportMat->FresnelR0 = DirectX::XMFLOAT3(0.001f, 0.001f, 0.001f);
	viewportMat->Roughness = 0.0f;
	mMaterials[viewportMat->Name] = std::move(viewportMat);
}


void MyApp::BuildRenderItems()
{
	auto boxRitem				= std::make_unique<RenderItem>(mGeometries[0].get(), mGeometries[0]->DrawArgs["0"]);
	auto alphaBoxRitem			= std::make_unique<RenderItem>(mGeometries[0].get(), mGeometries[0]->DrawArgs["0"]);
	auto subBoxRitem			= std::make_unique<RenderItem>(mGeometries[0].get(), mGeometries[0]->DrawArgs["0"]);
	auto gridRitem				= std::make_unique<RenderItem>(mGeometries[0].get(), mGeometries[0]->DrawArgs["1"]);
	auto mirrorGridRitem		= std::make_unique<RenderItem>(mGeometries[0].get(), mGeometries[0]->DrawArgs["1"]);
	auto tessGridRitem			= std::make_unique<RenderItem>(mGeometries[0].get(), mGeometries[0]->DrawArgs["1"]);
	auto sphereRitem			= std::make_unique<RenderItem>(mGeometries[0].get(), mGeometries[0]->DrawArgs["2"]);
	auto subSphereRitem			= std::make_unique<RenderItem>(mGeometries[0].get(), mGeometries[0]->DrawArgs["2"]);
	auto cubeSphereRitem		= std::make_unique<RenderItem>(mGeometries[0].get(), mGeometries[0]->DrawArgs["2"]);
	auto cylinderRitem			= std::make_unique<RenderItem>(mGeometries[0].get(), mGeometries[0]->DrawArgs["3"]);
	auto landRitem				= std::make_unique<RenderItem>(mGeometries[0].get(), mGeometries[0]->DrawArgs["4"], false);
	auto wavesRitem				= std::make_unique<RenderItem>(mGeometries[0].get(), mGeometries[0]->DrawArgs["5"], false);
	auto skullRitem				= std::make_unique<RenderItem>(mGeometries[0].get(), mGeometries[0]->DrawArgs["6"]);
	auto carRitem				= std::make_unique<RenderItem>(mGeometries[0].get(), mGeometries[0]->DrawArgs["7"]);
	auto boundingBoxRitem		= std::make_unique<RenderItem>(mGeometries[0].get(), mGeometries[0]->DrawArgs["0"]);
	auto boundingSphereRitem	= std::make_unique<RenderItem>(mGeometries[0].get(), mGeometries[0]->DrawArgs["2"]);
	auto treeSpritesRitem		= std::make_unique<RenderItem>(mGeometries[1].get(), mGeometries[1]->DrawArgs["0"], false);

	subBoxRitem->LayerFlag
		= (1 << (int)RenderLayer::Subdivision)
		| (1 << (int)RenderLayer::Normal)
		| (1 << (int)RenderLayer::SubdivisionWireframe)
		| (1 << (int)RenderLayer::NormalWireframe);

	subSphereRitem->LayerFlag
		= (1 << (int)RenderLayer::Subdivision)
		| (1 << (int)RenderLayer::Normal)
		| (1 << (int)RenderLayer::SubdivisionWireframe)
		| (1 << (int)RenderLayer::NormalWireframe);

	alphaBoxRitem->LayerFlag
		= (1 << (int)RenderLayer::AlphaTested)
		| (1 << (int)RenderLayer::AlphaTestedWireframe)
		| (1 << (int)RenderLayer::Normal)
		| (1 << (int)RenderLayer::NormalWireframe);

	mirrorGridRitem->LayerFlag
		= (1 << (int)RenderLayer::Mirror)
		| (1 << (int)RenderLayer::Transparent);

	wavesRitem->LayerFlag
		= (1 << (int)RenderLayer::WaveVS_CS);

	treeSpritesRitem->LayerFlag
		= (1 << (int)RenderLayer::TreeSprites)
		| (1 << (int)RenderLayer::TreeSpritesWireframe)
		| (1 << (int)RenderLayer::Normal);

	tessGridRitem->LayerFlag
		= (1 << (int)RenderLayer::Tessellation)
		| (1 << (int)RenderLayer::TessellationWireframe)
		| (1 << (int)RenderLayer::Normal);

	boundingBoxRitem->LayerFlag
		= (1 << (int)RenderLayer::BoundingBox);

	boundingSphereRitem->LayerFlag
		= (1 << (int)RenderLayer::BoundingSphere);

	cubeSphereRitem->LayerFlag
		= (1 << (int)RenderLayer::CubeMap);

	//=========================================================
	// GEO_MESH_NAMES[0]: ShapeGeo
	//=========================================================
	DirectX::SimpleMath::Vector3 translation;
	DirectX::SimpleMath::Vector3 scale(1.0f, 1.0f, 1.0f);
	DirectX::SimpleMath::Quaternion rot;
	DirectX::SimpleMath::Vector3 texScale(1.0f, 1.0f, 1.0f);

	for (int i = 0; i < MATERIAL_NAMES.size() * 10; ++i)
	{
		translation.x = (i % 10) * 8.0f;
		translation.y = 10.0f;
		translation.z = 5.0f + 5.0f * (i / 5);
		scale.x = 2.0f;
		scale.y = 2.0f;
		scale.z = 2.0f;

		boxRitem->Push(translation, scale, rot, { 2.0f, 1.0f, 2.0f }, mInstanceCount++, i % MATERIAL_NAMES.size());
	}
	
	for (int i = 0; i < MATERIAL_NAMES.size() * 10; ++i)
	{
		translation.x = (i % 10) * 8.0f;
		translation.y = 20.0f;
		translation.z = 5.0f + 5.0f * (i / 5);
		scale.x = 3.0f;
		scale.y = 3.0f;
		scale.z = 3.0f;

		sphereRitem->Push(translation, scale, rot, texScale, mInstanceCount++, i % MATERIAL_NAMES.size());
	}

	for (int i = 0; i < MATERIAL_NAMES.size() * 10; ++i)
	{
		translation.x = (i % 10) * 8.0f;
		translation.y = 30.0f;
		translation.z = 5.0f + 5.0f * (i / 5);
		scale.x = 3.0f;
		scale.y = 3.0f;
		scale.z = 3.0f;

		subBoxRitem->Push(translation, scale, rot, texScale, mInstanceCount++, i % MATERIAL_NAMES.size());
	}
	
	for (int i = 0; i < MATERIAL_NAMES.size() * 10; ++i)
	{
		translation.x = (i % 10) * 8.0f;
		translation.y = 40.0f;
		translation.z = 5.0f + 5.0f * (i / 5);
		scale.x = 3.0f;
		scale.y = 3.0f;
		scale.z = 3.0f;

		subSphereRitem->Push(translation, scale, rot, texScale, mInstanceCount++, i % MATERIAL_NAMES.size());
	}

	for (int i = 0; i < MATERIAL_NAMES.size() * 10; ++i)
	{
		translation.x = (i % 10) * 8.0f;
		translation.y = 50.0f;
		translation.z = 5.0f + 5.0f * (i / 5);
		scale.x = 3.0f;
		scale.y = 3.0f;
		scale.z = 3.0f;

		alphaBoxRitem->Push(translation, scale, rot, texScale, mInstanceCount++, i % MATERIAL_NAMES.size());
	}

	{
		translation.x = 0.0f;
		translation.y = 5.0f;
		translation.z = 0.0f;
		scale.x = 1.0f;
		scale.y = 1.0f;
		scale.z = 1.0f;
		gridRitem->Push(translation, scale, rot, { 8.0f, 8.0f, 8.0f }, mInstanceCount++, 1);
	}

	for (int i = 0; i < 5; ++i)
	{
		translation.x = 5.0f;
		translation.y = 6.5f;
		translation.z = -10.0f + i * 5.0f;
		scale.x = 1.0f;
		scale.y = 1.0f;
		scale.z = 1.0f;

		cylinderRitem->Push(translation, scale, rot, texScale, mInstanceCount++, i);
		translation.y = 8.5f;
		sphereRitem->Push(translation, scale, rot, texScale, mInstanceCount++, i);

		translation.x = -5.0f;
		translation.y = 6.5f;
		cylinderRitem->Push(translation, scale, rot, texScale, mInstanceCount++, i);
		translation.y = 8.5f;
		sphereRitem->Push(translation, scale, rot, texScale, mInstanceCount++, i);
	}

	{
		translation.x = 0.0f;
		translation.y = 8.0f;
		translation.z = 0.0f;
		scale.x = 3.0f;
		scale.y = 3.0f;
		scale.z = 3.0f;
		sphereRitem->Push(translation, scale, rot, texScale, mInstanceCount++, 2);
	}

	{
		translation.x = 0.0f;
		translation.y = 100.0f;
		translation.z = 0.0f;
		scale.x = 1.0f;
		scale.y = 1.0f;
		scale.z = 1.0f;
		mirrorGridRitem->Push(translation, scale, rot, texScale, mInstanceCount++, 13);
	}
	
	
	////=========================================================
	//// GEO_MESH_NAMES[1]:ModelGeo
	////=========================================================
	for (int i = 0; i < MATERIAL_NAMES.size() * 20; ++i)
	{
		translation.x = (i % 10) * 8.0f;
		translation.y = 60.0f;
		translation.z = 5.0f + 5.0f * (i / 5);
		scale.x = 1.0f;
		scale.y = 1.0f;
		scale.z = 1.0f;

		skullRitem->Push(translation, scale, rot, texScale, mInstanceCount++, i % MATERIAL_NAMES.size());
	}

	for (int i = 0; i < MATERIAL_NAMES.size() * 20; ++i)
	{
		translation.x = (i % 10) * 8.0f;
		translation.y = 70.0f;
		translation.z = 5.0f + 5.0f * (i / 5);
		scale.x = 1.0f;
		scale.y = 1.0f;
		scale.z = 1.0f;

		carRitem->Push(translation, scale, rot, texScale, mInstanceCount++, i % MATERIAL_NAMES.size());
	}

	////=========================================================
	//// GEO_MESH_NAMES[2]:LandGeo
	////=========================================================
	{
		translation.x = 0.0f;
		translation.y = 0.0f;
		translation.z = 0.0f;
		scale.x = 1.0f;
		scale.y = 1.0f;
		scale.z = 1.0f;
		landRitem->Push(translation, scale, rot, texScale, mInstanceCount++, 4);
	}

	////=========================================================
	//// GEO_MESH_NAMES[3]:WaterGeo
	////=========================================================
	{
		translation.x = 0.0f;
		translation.y = 0.0f;
		translation.z = 0.0f;
		scale.x = 1.5f;
		scale.y = 1.5f;
		scale.z = 1.5f;
		wavesRitem->Push(translation, scale, rot, texScale, mInstanceCount++, 11);
		wavesRitem->Datas.back().InstanceData.DisplacementMapTexelSize.x = 1.0f / mCSWaves->ColumnCount();
		wavesRitem->Datas.back().InstanceData.DisplacementMapTexelSize.y = 1.0f / mCSWaves->RowCount();
		wavesRitem->Datas.back().InstanceData.GridSpatialStep = mCSWaves->SpatialStep();
	}

	////=========================================================
	//// GEO_MESH_NAMES[5]: TreeSpritesGeo
	////=========================================================
	{
		translation.x = 0.0f;
		translation.y = 0.0f;
		translation.z = 0.0f;
		scale.x = 1.0f;
		scale.y = 1.0f;
		scale.z = 1.0f;
		treeSpritesRitem->Push(translation, scale, rot, texScale, mInstanceCount++, TEX_DIFF_FILENAMES.size());
	}

	////=========================================================
	//// Tessellation
	////=========================================================
	{
		translation.x = 0.0f;
		translation.y = 10.0f;
		translation.z = 0.0f;
		scale.x = 1.0f;
		scale.y = 1.0f;
		scale.z = 1.0f;
		tessGridRitem->Push(translation, scale, rot, texScale, mInstanceCount++, SRV_USER_SIZE);
	}

	mAllRitems.push_back(std::move(boxRitem));
	mAllRitems.push_back(std::move(gridRitem));
	mAllRitems.push_back(std::move(sphereRitem));
	mAllRitems.push_back(std::move(cylinderRitem));
	mAllRitems.push_back(std::move(alphaBoxRitem));
	mAllRitems.push_back(std::move(mirrorGridRitem));
	mAllRitems.push_back(std::move(subBoxRitem));
	mAllRitems.push_back(std::move(subSphereRitem));
	mAllRitems.push_back(std::move(skullRitem));
	mAllRitems.push_back(std::move(carRitem));
	mAllRitems.push_back(std::move(landRitem));
	mAllRitems.push_back(std::move(wavesRitem));
	mAllRitems.push_back(std::move(treeSpritesRitem));
	mAllRitems.push_back(std::move(tessGridRitem));

	for (size_t i = 0; i < mAllRitems.size(); ++i)
	{
		if (mAllRitems[i]->mFrustumCullingEnabled)
		{
			for (const auto& a : mAllRitems[i]->Datas)
			{
				boundingBoxRitem->Push(a);
				boundingSphereRitem->Push(a);

				++mInstanceCount;
				++mInstanceCount;
			}
		}
	}
	mAllRitems.push_back(std::move(boundingBoxRitem));
	mAllRitems.push_back(std::move(boundingSphereRitem));

	{
		translation.x = 0.0f;
		translation.y = 0.0f;
		translation.z = 0.0f;
		scale.x = 5000.0f;
		scale.y = 5000.0f;
		scale.z = 5000.0f;
		cubeSphereRitem->Push(translation, scale, rot, texScale, mInstanceCount++, SRV_USER_SIZE + TEX_DIFF_FILENAMES.size() + TEX_ARRAY_FILENAMES.size(), false);
	}
	mAllRitems.push_back(std::move(cubeSphereRitem));

	mUpdateBoundingMesh = true;
}

void MyApp::BuildFrameResources()
{
	for (int i = 0; i < APP_NUM_FRAME_RESOURCES; ++i)
		mFrameResources.push_back(std::make_unique<FrameResource>(mDevice.Get(), 2, (UINT)mAllRitems.size(), mInstanceCount * 2, (UINT)mMaterials.size()));
}

void MyApp::BuildPSO()
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
	// PSO for CS Wave
	//=====================================================
	D3D12_GRAPHICS_PIPELINE_STATE_DESC wavesRenderPSO = transparentPsoDesc;
	wavesRenderPSO.VS = { reinterpret_cast<BYTE*>(mShaders["DisplacementVS"]->GetBufferPointer()), mShaders["DisplacementVS"]->GetBufferSize() };
	wavesRenderPSO.PS = { reinterpret_cast<BYTE*>(mShaders["DisplacementPS"]->GetBufferPointer()), mShaders["DisplacementPS"]->GetBufferSize() };

	//=====================================================
	// PSO for Tessellation
	//=====================================================
	D3D12_GRAPHICS_PIPELINE_STATE_DESC tessPsoDesc = opaquePsoDesc;
	tessPsoDesc.VS = { reinterpret_cast<BYTE*>(mShaders["TessVS"]->GetBufferPointer()), mShaders["TessVS"]->GetBufferSize() };
	tessPsoDesc.HS = { reinterpret_cast<BYTE*>(mShaders["TessHS"]->GetBufferPointer()), mShaders["TessHS"]->GetBufferSize() };
	tessPsoDesc.DS = { reinterpret_cast<BYTE*>(mShaders["TessDS"]->GetBufferPointer()), mShaders["TessDS"]->GetBufferSize() };
	tessPsoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["TessPS"]->GetBufferPointer()), mShaders["TessPS"]->GetBufferSize() };
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
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&tessPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::Tessellation])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&boundingBoxPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::BoundingBox])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&boundingBoxPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::BoundingSphere])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&wavesRenderPSO, IID_PPV_ARGS(&mPSOs[RenderLayer::WaveVS_CS])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&cubeMapPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::CubeMap])));

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
	tessPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	wavesRenderPSO.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;

	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::OpaqueWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&markMirrorsPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::MirrorWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&drawReflectionsPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::ReflectedWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::TransparentWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::AlphaTestedWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::ShadowWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&subdivisionDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::SubdivisionWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&normalDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::NormalWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&treeSpritePsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::TreeSpritesWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&tessPsoDesc, IID_PPV_ARGS(&mPSOs[RenderLayer::TessellationWireframe])));
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&wavesRenderPSO, IID_PPV_ARGS(&mPSOs[RenderLayer::WaveVS_CS_Wireframe])));
}
#pragma endregion Initialize

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
	OnKeyboardInput();

	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % APP_NUM_FRAME_RESOURCES;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, mFenceEvent));
		WaitForSingleObject(mFenceEvent, INFINITE);
	}

	AnimateMaterials();
	UpdateInstanceBuffer();
	UpdateMaterialBuffer();
	UpdateMainPassCB();
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

	// Bind per-pass constant buffer.  We only need to do this once per-pass.
	auto passCB = mCurrFrameResource->PassCB->Resource();
	UINT passCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	auto matBuffer = mCurrFrameResource->MaterialBuffer->Resource();
	auto instanceBuffer = mCurrFrameResource->InstanceBuffer->Resource();
	{
		//====================================================
		// GPU Memory setting
		//====================================================
		ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
		mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

		// slotRootParameter[0].InitAsConstantBufferView(1);	// gBaseInstanceIndex b1
		// slotRootParameter[1].InitAsShaderResourceView(0, 4);	// InstanceData t0 (Space4)
		// slotRootParameter[2].InitAsShaderResourceView(1, 4);	// MaterialData t1 (Space4)
		// slotRootParameter[3].InitAsConstantBufferView(0);	// cbPass b0
		// slotRootParameter[4].InitAsDescriptorTable(1, &TexDiffTable, D3D12_SHADER_VISIBILITY_PIXEL);
		// slotRootParameter[5].InitAsDescriptorTable(1, &TexNormTable, D3D12_SHADER_VISIBILITY_PIXEL);
		// slotRootParameter[6].InitAsDescriptorTable(1, &TexArrayTable, D3D12_SHADER_VISIBILITY_PIXEL);
		// slotRootParameter[7].InitAsDescriptorTable(1, &TexCubeTable, D3D12_SHADER_VISIBILITY_PIXEL);
		// slotRootParameter[8].InitAsDescriptorTable(1, &DisplacementMapTable, D3D12_SHADER_VISIBILITY_ALL);
		mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
		mCommandList->SetGraphicsRootShaderResourceView(1, instanceBuffer->GetGPUVirtualAddress());
		mCommandList->SetGraphicsRootShaderResourceView(2, matBuffer->GetGPUVirtualAddress());
		mCommandList->SetGraphicsRootConstantBufferView(3, passCB->GetGPUVirtualAddress());
		mCommandList->SetGraphicsRootDescriptorTable(4, mhGPUDiff);
		mCommandList->SetGraphicsRootDescriptorTable(5, mhGPUNorm);
		mCommandList->SetGraphicsRootDescriptorTable(6, mhGPUArray);
		mCommandList->SetGraphicsRootDescriptorTable(7, mhGPUCube);
		mCommandList->SetGraphicsRootDescriptorTable(8, mCSWaves->DisplacementMap());
		
	}
	
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

	for (int i = 0; i < MAX_LAYER_DEPTH; ++i)
	{
		if (mLayerType[i] == RenderLayer::None)
			continue;
		else if (mLayerType[i] == RenderLayer::AddCS)
		{
			mCSAdd->DoComputeWork(mCommandList.Get(), mCurrFrameResource->CmdListAlloc.Get());
			mLayerType[i] = RenderLayer::None;
			mSyncEn = true;
		}
		else if (mLayerType[i] == RenderLayer::BlurCS)
		{
			mCSBlurFilter->Execute(mCommandList.Get(), mSwapChainBuffer[mCurrBackBuffer].Get(), 4);
		}
		else {
			if(mLayerType[i] == RenderLayer::WaveVS_CS)
				mCSWaves->UpdateWaves(mTimer, mCommandList.Get());
			mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
			mCommandList->SetGraphicsRootConstantBufferView(3, passCB->GetGPUVirtualAddress() + passCBByteSize * mLayerCBIdx[i]);
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

void MyApp::Sync()
{
	mCSAdd->PrintOutput();
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

void MyApp::UpdateTangents()
{
	using namespace DirectX;

	// https://github.com/microsoft/DirectXMesh/wiki/ComputeTangentFrame

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

		//if (m.skinnedVertices.size() > 0) {
		//	for (size_t i = 0; i < m.skinnedVertices.size(); i++) {
		//		m.skinnedVertices[i].tangentModel = tangents[i];
		//	}
		//}
	}
}

void MyApp::UpdateInstanceBuffer()
{
	mCamFrustum.Origin = mCamera.GetPosition3f();
	mCamFrustum.Orientation = mCamera.GetQuaternion();

	auto currInstanceBuffer = mCurrFrameResource->InstanceBuffer.get();
	auto currInstanceCB = mCurrFrameResource->InstanceCB.get();
	int visibleInstanceCount = 0;
	// mAllRitems[0]->Datas[0].Translation.y += 0.01f;
	for (size_t i = 0; i < mAllRitems.size(); ++i)
	{
		auto& e = mAllRitems[i];
		
		InstanceConstants insCB;
		insCB.BaseInstanceIndex = visibleInstanceCount;
		e->StartInstanceLocation = visibleInstanceCount;
		currInstanceCB->CopyData(i, insCB);

		for (size_t i = 0; i < e->Datas.size(); ++i)
		{
			if ((mCamFrustum.Contains(e->Datas[i].BoundingBox) != DirectX::DISJOINT) || (e->Datas[i].FrustumCullingEnabled == false))
			{
				currInstanceBuffer->CopyData(visibleInstanceCount++, e->Datas[i].InstanceData);
			}
		}
		e->InstanceCount = visibleInstanceCount - e->StartInstanceLocation;
	}
	std::wostringstream outs;
	outs.precision(6);
	outs << L"Instancing and Culling Demo" <<
		L"    " << visibleInstanceCount <<
		L" objects visible out of " << mInstanceCount;
	mWndCaption = outs.str();

//	for (auto& e : mAllRitems)
//	{
//		if (e->NumFramesDirty > 0)
//		{
//			
//			//DirectX::XMMATRIX rotate = DirectX::XMMatrixRotationY(e->AngleY * MathHelper::Pi);
//			//DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(e->ScaleX, e->ScaleY, e->ScaleZ);
//			//DirectX::XMMATRIX offset = DirectX::XMMatrixTranslation(e->OffsetX, e->OffsetY, e->OffsetZ);
//			//DirectX::XMMATRIX world = rotate * scale * offset;
//
//			// Update reflection world matrix.
//			if (e->LayerFlag & (1 << (int)RenderLayer::Reflected))
//			{
//				DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&e->Instances.World);
//				DirectX::XMMATRIX texTransform = DirectX::XMLoadFloat4x4(&e->Instances.TexTransform);
//				DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(world);
//
//				DirectX::XMVECTOR mirrorPlane = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // xy plane
//				DirectX::XMMATRIX R = DirectX::XMMatrixReflect(mirrorPlane);
//				InstanceData data;
//				DirectX::XMStoreFloat4x4(&data.World, DirectX::XMMatrixTranspose(world * R));
//				DirectX::XMStoreFloat4x4(&data.TexTransform, XMMatrixTranspose(texTransform));
//				DirectX::XMStoreFloat4x4(&data.WorldInvTranspose, DirectX::XMMatrixInverse(&det, world));
//				data.MaterialIndex = e->Mat->MatCBIndex;
//				data.DisplacementMapTexelSize = e->Instances.DisplacementMapTexelSize;
//				data.GridSpatialStep = e->Instances.GridSpatialStep;
//
//				currInstanceBuffer->CopyData(mAllRitems.size() + e->InsIndex, data);
//			}
//		}
//	}
}

void MyApp::UpdateMaterialBuffer()
{
	auto currMaterialCB = mCurrFrameResource->MaterialBuffer.get();
	for (auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			DirectX::XMMATRIX matTransform = DirectX::XMLoadFloat4x4(&mat->MatTransform);

			MaterialData matData;
			matData.DiffuseAlbedo = mat->DiffuseAlbedo;
			matData.FresnelR0 = mat->FresnelR0;
			matData.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(matTransform));
			matData.DiffMapIndex = mat->DiffuseSrvHeapIndex;
			matData.NormMapIndex = mat->NormalSrvHeapIndex;

			currMaterialCB->CopyData(mat->MatCBIndex, matData);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void MyApp::UpdateMainPassCB()
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

#pragma endregion Update

void MyApp::DrawRenderItems(const RenderLayer flag)
{
	UINT offset
		= (flag == RenderLayer::Reflected || flag == RenderLayer::ReflectedWireframe)
		? mAllRitems.size()
		: 0;
	UINT objCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(InstanceConstants));
	auto currInstanceCB = mCurrFrameResource->InstanceCB->Resource();

	// For each render item...
	for (size_t i = 0; i < mAllRitems.size(); ++i)
	{
		auto& ri = mAllRitems[i];
		if (!(ri->LayerFlag & (1 << (int)flag)))
			continue;

		if (flag == RenderLayer::Normal
			|| flag == RenderLayer::NormalWireframe
			|| flag == RenderLayer::TreeSprites
			|| flag == RenderLayer::TreeSpritesWireframe)
			ri->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
		else if (flag == RenderLayer::Tessellation
			|| flag == RenderLayer::TessellationWireframe)
			ri->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST;
		else
			ri->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		D3D12_VERTEX_BUFFER_VIEW vbv = ri->Geo->VertexBufferView();
		D3D12_INDEX_BUFFER_VIEW ibv = ri->Geo->IndexBufferView();
		mCommandList->IASetVertexBuffers(0, 1, &vbv);
		mCommandList->IASetIndexBuffer(&ibv);
		mCommandList->IASetPrimitiveTopology(ri->PrimitiveType);

		// StartInstanceLocation 적용 불가 버그를 해결하기 위해 Constant buffer를 활용함
		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = currInstanceCB->GetGPUVirtualAddress() + i * objCBByteSize;
		mCommandList->SetGraphicsRootConstantBufferView(0, objCBAddress);

		mCommandList->DrawIndexedInstanced(ri->IndexCount, ri->InstanceCount, ri->StartIndexLocation, ri->BaseVertexLocation, ri->StartInstanceLocation);
		// mCommandList->DrawIndexedInstanced(ri->IndexCount, ri->InstanceCount, ri->StartIndexLocation, ri->BaseVertexLocation, 0);	// DX12 버그로 위의 코드도 Start InstanceLocation이 반영되지 않는다.
	}
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

	// if (mPickModel.first == -1)
	{
		mPickModel = PickClosest(curRay, dist);
		if (mPickModel.first != -1)
		{
			std::cout << "Newly selected model: " << mPickModel.first << " / " << mPickModel.second << std::endl;
		}
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
		ImGui::Checkbox("Material", &mShowMaterialWindow);
		ImGui::Checkbox("Render Item", &mShowRenderItemWindow);
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
				"Opaque",
				"Mirror",
				"Reflected",
				"AlphaTested",
				"Transparent",
				"Shadow",
				"Subdivision",
				"Normal",
				"TreeSprites",
				"Tessellation",
				"BoundingBox",
				"BoundingSphere",
				"CubeMap",
				"OpaqueWireframe",
				"MirrorWireframe",
				"ReflectedWireframe",
				"AlphaTestedWireframe",
				"TransparentWireframe",
				"ShadowWireframe",
				"SubdivisionWireframe",
				"NormalWireframe",
				"TreeSpritesWireframe",
				"TessellationWireframe",
				"AddCS",
				"BlurCS",
				"WaveVS_CS"
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

	int size
		= (int) SRV_USER_SIZE
		+ (int) TEX_DIFF_FILENAMES.size()
		+ (int) TEX_NORM_FILENAMES.size()
		+ (int) TEX_ARRAY_FILENAMES.size()
		+ (int) mCSBlurFilter->DescriptorCount()
		+ (int) mCSWaves->DescriptorCount()
		+ (int) TEX_CUBE_FILENAMES.size();
		
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

	int dataSize
		= SRV_USER_SIZE
		+ (int) TEX_DIFF_FILENAMES.size()
		+ (int) TEX_NORM_FILENAMES.size()
		+ (int) TEX_ARRAY_FILENAMES.size()
		+ (int) TEX_CUBE_FILENAMES.size()
		+ (int) mCSBlurFilter->DescriptorCount()
		+ (int) mCSWaves->DescriptorCount();

	int texDiffSize
		= (int) SRV_USER_SIZE
		+ (int) TEX_DIFF_FILENAMES.size();

	int texNormSize
		= (int) TEX_NORM_FILENAMES.size();

	ImGui::LabelText("label", "Value");
	ImGui::SeparatorText("Inputs");
	ImGuiSliderFlags flags = ImGuiSliderFlags_None & ~ImGuiSliderFlags_WrapAround;
	ImGui::SliderInt((std::string("Material [0, ") + std::to_string(mMaterials.size() - 1) + "]").c_str(), &matIdx, 0, mMaterials.size() - 1, "%d", flags);
	{
		int flag = 0;
		Material* mat = mMaterials[MATERIAL_NAMES[matIdx]].get();

		flag += ImGui::DragFloat4("DiffuseAlbedo R/G/B/A", diff4f, 0.01f, 0.0f, 1.0f);
		flag += ImGui::DragFloat3("Fresne R/G/B", fres3f, 0.01f, 0.0f, 1.0f);
		flag += ImGui::DragFloat("Roughness", &mat->Roughness, 0.01f, 0.0f, 1.0f);
		flag += ImGui::SliderInt((std::string("Tex Diffuse Index [0, ") + std::to_string(texDiffSize - 1) + "]").c_str(), &mat->DiffuseSrvHeapIndex, 0, texDiffSize-1, "%d", flags);
		flag += ImGui::SliderInt((std::string("Tex Normal Index [0, ") + std::to_string(texNormSize - 1) + "]").c_str(), &mat->NormalSrvHeapIndex, 0, texNormSize-1, "%d", flags);
		if (flag)
		{
			mat->DiffuseAlbedo = { diff4f[0],diff4f[1],diff4f[2],diff4f[3] };
			mat->FresnelR0 = { fres3f[0], fres3f[1], fres3f[2] };
			mat->NumFramesDirty = APP_NUM_FRAME_RESOURCES;
		}

		ImTextureID my_tex_id = (ImTextureID)mhGPUDiff.ptr + mCbvSrvUavDescriptorSize * mat->DiffuseSrvHeapIndex;
		ImVec4 tint_col = true ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // No tint
		ImVec4 border_col = ImGui::GetStyleColorVec4(ImGuiCol_Border);

		ImGui::Image(my_tex_id, ImVec2(mImguiWidth, mImguiHeight), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), tint_col, border_col);
		my_tex_id = (ImTextureID)mhGPUNorm.ptr + mCbvSrvUavDescriptorSize * mat->NormalSrvHeapIndex;
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
	};
	{
		flag = 0;

		flag += ImGui::DragFloat3("world x/y/z", world3f, 0.01f, -FLT_MAX / 2, FLT_MAX / 2);
		flag += ImGui::DragFloat3("scale x/y/z", scale3f, 0.01f, -FLT_MAX / 2, FLT_MAX / 2);
		flag += ImGui::DragFloat3("angle x/y/z", angle3f, 0.01f, -FLT_MAX / 2, FLT_MAX / 2);

		if (flag)
		{
			// ritm->NumFramesDirty = 1;
			// TODO: 즉시 instance 데이터를 업데이트 하는 로직으로 수정
		}
	}

	ImGui::End();
}

void MyApp::ShowViewportWindow()
{
	ImGui::Begin("viewport1", &mShowViewportWindow);

	ImTextureID my_tex_id = (ImTextureID)mhGPUUser.ptr + mCbvSrvUavDescriptorSize;
	{
		ImVec2 uv_min = ImVec2(0.0f, 0.0f);                 // Top-left
		ImVec2 uv_max = ImVec2(1.0f, 1.0f);                 // Lower-right
		ImVec4 tint_col = true ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // No tint
		ImVec4 border_col = ImGui::GetStyleColorVec4(ImGuiCol_Border);
		ImGui::Image(my_tex_id, ImVec2(mImguiWidth, mImguiHeight), uv_min, uv_max, tint_col, border_col);
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
	flag += ImGui::SliderInt((std::string("Cubemap [0, ") + std::to_string((UINT)TEX_CUBE_FILENAMES.size() - 1) + "]").c_str(), &idx, 0, (UINT)TEX_CUBE_FILENAMES.size() - 1, "%d", flags);
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
