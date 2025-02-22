#include "pch.h"
#include "ShadowMap.h"

ShadowMap::ShadowMap(
	ID3D12Device* device,
	UINT width, 
	UINT height, 
	const DirectX::SimpleMath::Vector3 lightDirection,
	const DirectX::SimpleMath::Vector3 lightStrength,
	const DirectX::SimpleMath::Vector3 targetPosition,
	const DirectX::SimpleMath::Vector4 ambientLight,
	float orthoBoxLength)
	: md3dDevice(device)
	, mWidth(width)
	, mHeight(height)
	, mViewport({ 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f })
	, mScissorRect({ 0, 0, (int)width, (int)height })
	, mFormat(DXGI_FORMAT_R24G8_TYPELESS)
	, mOrthoBoxLength(orthoBoxLength)
{
	mBaseLightDir = XMLoadFloat3(&lightDirection);
	mLightDir = XMLoadFloat3(&lightDirection);
	mTargetPos = XMLoadFloat3(&targetPosition);
	mPassCB.AmbientLight = ambientLight;
	mLight.Strength = lightStrength;
	Update();

	BuildResource();
}

PassConstants ShadowMap::GetPassCB() const
{
	return mPassCB;
}

UINT ShadowMap::Width()const
{
	return mWidth;
}

UINT ShadowMap::Height()const
{
	return mHeight;
}

ID3D12Resource* ShadowMap::Resource()
{
	return mShadowMap.Get();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE ShadowMap::Srv()const
{
	return mhGpuSrv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE ShadowMap::Dsv()const
{
	return mhCpuDsv;
}

D3D12_VIEWPORT ShadowMap::Viewport()const
{
	return mViewport;
}

D3D12_RECT ShadowMap::ScissorRect()const
{
	return mScissorRect;
}

void ShadowMap::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv)
{
	// Save references to the descriptors. 
	mhCpuSrv = hCpuSrv;
	mhGpuSrv = hGpuSrv;
	mhCpuDsv = hCpuDsv;

	//  Create the descriptors
	BuildDescriptors();
}

void ShadowMap::OnResize(UINT newWidth, UINT newHeight)
{
	if ((mWidth != newWidth) || (mHeight != newHeight))
	{
		mWidth = newWidth;
		mHeight = newHeight;

		BuildResource();

		// New resource, so we need new descriptors to that resource.
		BuildDescriptors();
	}
}

void ShadowMap::SetBoxLength(float orthoBoxLength)
{
	mOrthoBoxLength = orthoBoxLength;
	Update();
}

void ShadowMap::SetAmbientLight(const DirectX::SimpleMath::Vector4& ambientLight)
{
	mPassCB.AmbientLight = ambientLight;
}

void ShadowMap::SetLightStrength(const DirectX::SimpleMath::Vector3& strength)
{
	mLight.Strength = strength;
}

void ShadowMap::SetBaseDir(const DirectX::SimpleMath::Vector3& dir)
{
	mBaseLightDir = XMLoadFloat3(&dir);
}

void ShadowMap::SetRotate(const float angle)
{
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationY(angle);
	mLightDir = DirectX::XMVector3TransformNormal(mBaseLightDir, R);
	Update();
}

void ShadowMap::SetTarget(const DirectX::SimpleMath::Vector3& targetPos)
{
	mTargetPos = XMLoadFloat3(&targetPos);
	Update();
}

float ShadowMap::GetBoxLength() const
{
	return mOrthoBoxLength;
}

DirectX::SimpleMath::Vector4 ShadowMap::GetAmbientLight() const
{
	return mPassCB.AmbientLight;
}

DirectX::SimpleMath::Vector3 ShadowMap::GetLightStrength() const
{
	return mPassCB.Lights[0].Strength;
}

DirectX::SimpleMath::Vector3 ShadowMap::GetBaseDir() const
{
	DirectX::SimpleMath::Vector3 ret;
	DirectX::XMStoreFloat3(&ret, mBaseLightDir);
	return ret;
}

DirectX::SimpleMath::Vector3 ShadowMap::GetRotate() const
{
	DirectX::SimpleMath::Vector3 ret;
	DirectX::XMStoreFloat3(&ret, mLightDir);
	return ret;
}

DirectX::SimpleMath::Vector3 ShadowMap::GetTarget() const
{
	DirectX::SimpleMath::Vector3 ret;
	DirectX::XMStoreFloat3(&ret, mTargetPos);
	return ret;
}

// void ShadowMap::ExecuteBegin(ID3D12GraphicsCommandList* cmdList, UINT rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS passCBAddress)
// {
// 	cmdList->RSSetViewports(1, &mViewport);
// 	cmdList->RSSetScissorRects(1, &mScissorRect);
// 
// 	D3D12_RESOURCE_BARRIER barrier
// 		= CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap.Get(),
// 			D3D12_RESOURCE_STATE_GENERIC_READ,
// 			D3D12_RESOURCE_STATE_DEPTH_WRITE);
// 
// 	// Change to DEPTH_WRITE.
// 	cmdList->ResourceBarrier(1, &barrier);
// 
// 	cmdList->ClearDepthStencilView(mhCpuDsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
// 
// 	cmdList->OMSetRenderTargets(0, nullptr, false, &mhCpuDsv);
// 
// 	cmdList->SetGraphicsRootConstantBufferView(rootParameterIndex, passCBAddress);
// }
// 
// void ShadowMap::SetDefaultPSO(ID3D12GraphicsCommandList* cmdList)
// {
// 	cmdList->SetPipelineState(mShadowPSO.Get());
// }
// 
// void ShadowMap::SetSkinnedPSO(ID3D12GraphicsCommandList* cmdList)
// {
// 	cmdList->SetPipelineState(mSkinnedShadowPSO.Get());
// }
// 
// void ShadowMap::ExecuteFinish(ID3D12GraphicsCommandList* cmdList)
// {
// 	D3D12_RESOURCE_BARRIER barrier
// 		= CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap.Get(),
// 			D3D12_RESOURCE_STATE_DEPTH_WRITE,
// 			D3D12_RESOURCE_STATE_GENERIC_READ);
// 	cmdList->ResourceBarrier(1, &barrier);
// }
// 
// void ShadowMap::BuildPSO()
// {
// 	D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowMapPsoDesc
// 	{
// 		/* ID3D12RootSignature* pRootSignature								*/.pRootSignature = mRootSignature.Get(),
// 		/* D3D12_SHADER_BYTECODE VS											*/.VS = {reinterpret_cast<BYTE*>(mVSShadowShader->GetBufferPointer()), mVSShadowShader->GetBufferSize()},
// 		/* D3D12_SHADER_BYTECODE PS											*/.PS = {reinterpret_cast<BYTE*>(mPSShadowShader->GetBufferPointer()), mPSShadowShader->GetBufferSize()},
// 		/* D3D12_SHADER_BYTECODE DS											*/.DS = {NULL, 0},
// 		/* D3D12_SHADER_BYTECODE HS											*/.HS = {NULL, 0},
// 		/* D3D12_SHADER_BYTECODE GS											*/.GS = {NULL, 0},
// 		/* D3D12_STREAM_OUTPUT_DESC StreamOutput{							*/.StreamOutput = {
// 		/*		const D3D12_SO_DECLARATION_ENTRY* pSODeclaration{			*/	NULL,
// 		/*			UINT Stream;											*/
// 		/*			LPCSTR SemanticName;									*/
// 		/*			UINT SemanticIndex;										*/
// 		/*			BYTE StartComponent;									*/
// 		/*			BYTE ComponentCount;									*/
// 		/*			BYTE OutputSlot;										*/
// 		/*		}															*/
// 		/*		UINT NumEntries;											*/	0,
// 		/*		const UINT* pBufferStrides;									*/	0,
// 		/*		UINT NumStrides;											*/	0,
// 		/*		UINT RasterizedStream;										*/	0
// 		/* }																*/},
// 		/* D3D12_BLEND_DESC BlendState{										*/.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
// 		/*		BOOL AlphaToCoverageEnable									*/
// 		/*		BOOL IndependentBlendEnable									*/
// 		/*		D3D12_RENDER_TARGET_BLEND_DESC RenderTarget[8]				*/
// 		/* }																*/
// 		/* UINT SampleMask													*/.SampleMask = UINT_MAX,
// 		/* D3D12_RASTERIZER_DESC RasterizerState{							*/.RasterizerState = { // CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
// 		/*		D3D12_FILL_MODE FillMode									*/		D3D12_FILL_MODE_SOLID, // D3D12_FILL_MODE_WIREFRAME,
// 		/*		D3D12_CULL_MODE CullMode									*/		D3D12_CULL_MODE_BACK,
// 		/*		BOOL FrontCounterClockwise									*/		false,
// 		/*		INT DepthBias												*/		100000,
// 		/*		FLOAT DepthBiasClamp										*/		0.0f,
// 		/*		FLOAT SlopeScaledDepthBias									*/		1.0f,
// 		/*		BOOL DepthClipEnable										*/		true,
// 		/*		BOOL MultisampleEnable										*/		false,
// 		/*		BOOL AntialiasedLineEnable									*/		false,
// 		/*		UINT ForcedSampleCount										*/		0,
// 		/*		D3D12_CONSERVATIVE_RASTERIZATION_MODE ConservativeRaster	*/		D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
// 		/* }																*/},
// 		/* D3D12_DEPTH_STENCIL_DESC DepthStencilState {						*/.DepthStencilState = { // CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
// 		/*		BOOL DepthEnable											*/		.DepthEnable = true,
// 		/*		D3D12_DEPTH_WRITE_MASK DepthWriteMask						*/		.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
// 		/*		D3D12_COMPARISON_FUNC DepthFunc								*/		.DepthFunc = D3D12_COMPARISON_FUNC_LESS,
// 		/*		BOOL StencilEnable											*/		.StencilEnable = false,
// 		/*		UINT8 StencilReadMask										*/		.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
// 		/*		UINT8 StencilWriteMask										*/		.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
// 		/*		D3D12_DEPTH_STENCILOP_DESC FrontFace {						*/		.FrontFace = {
// 		/*			D3D12_STENCIL_OP StencilFailOp							*/			.StencilFailOp = D3D12_STENCIL_OP_KEEP,
// 		/*			D3D12_STENCIL_OP StencilDepthFailOp						*/			.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
// 		/*			D3D12_STENCIL_OP StencilPassOp							*/			.StencilPassOp = D3D12_STENCIL_OP_KEEP,
// 		/*			D3D12_COMPARISON_FUNC StencilFunc						*/			.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
// 		/*		}															*/		},
// 		/*		D3D12_DEPTH_STENCILOP_DESC BackFace							*/		.BackFace = {
// 		/*			D3D12_STENCIL_OP StencilFailOp							*/			.StencilFailOp = D3D12_STENCIL_OP_KEEP,
// 		/*			D3D12_STENCIL_OP StencilDepthFailOp						*/			.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
// 		/*			D3D12_STENCIL_OP StencilPassOp							*/			.StencilPassOp = D3D12_STENCIL_OP_KEEP,
// 		/*			D3D12_COMPARISON_FUNC StencilFunc						*/			.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
// 		/*		}															*/		},
// 		/* }																*/ },
// 		/* D3D12_INPUT_LAYOUT_DESC InputLayout{								*/.InputLayout = {
// 		/*		const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs			*/		.pInputElementDescs = mInputLayout.data(),
// 		/*		UINT NumElements											*/		.NumElements = (UINT)mInputLayout.size()
// 		/*	}																*/ },
// 		/* D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue				*/.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
// 		/* D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType				*/.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
// 		/* UINT NumRenderTargets											*/.NumRenderTargets = 0,
// 		/* DXGI_FORMAT RTVFormats[8]										*/.RTVFormats = {DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_UNKNOWN},	// 0
// 		/* DXGI_FORMAT DSVFormat											*/.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT,
// 		/* DXGI_SAMPLE_DESC SampleDesc{										*/.SampleDesc = {
// 		/*		UINT Count;													*/		.Count = m4xMsaaState ? 4u : 1u,
// 		/*		UINT Quality;												*/		.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0
// 		/*	}																*/},
// 		/* UINT NodeMask													*/.NodeMask = 0,
// 		/* D3D12_CACHED_PIPELINE_STATE CachedPSO							*/.CachedPSO = {NULL, 0},
// 		/* D3D12_PIPELINE_STATE_FLAGS Flags									*/.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
// 	};
// 	shadowMapPsoDesc.BlendState.IndependentBlendEnable = true;
// 	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&shadowMapPsoDesc, IID_PPV_ARGS(&mShadowPSO)));
// 
// 	D3D12_GRAPHICS_PIPELINE_STATE_DESC skinnedShadowMapPsoDesc = shadowMapPsoDesc;
// 	skinnedShadowMapPsoDesc.InputLayout = { mSkinnedInputLayout.data(), (UINT)mSkinnedInputLayout.size() };
// 	skinnedShadowMapPsoDesc.VS = { reinterpret_cast<BYTE*>(mVSSkinnedShadowShader->GetBufferPointer()), mVSSkinnedShadowShader->GetBufferSize() };
// 	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&skinnedShadowMapPsoDesc, IID_PPV_ARGS(&mSkinnedShadowPSO)));
// }

void ShadowMap::Update()
{
	UpdateMatrix();
	UpdatePassCB();
}

void ShadowMap::UpdateMatrix()
{
	using namespace DirectX;
	mLightPosW = -2.0f * mOrthoBoxLength * mLightDir;

	DirectX::XMVECTOR lightUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	mLightView = DirectX::XMMatrixLookAtLH(mLightPosW, mTargetPos, lightUp);

	// Transform bounding sphere to light space.
	DirectX::XMFLOAT3 center;
	DirectX::XMStoreFloat3(&center, XMVector3TransformCoord(mTargetPos, mLightView));

	// Ortho frustum in light space encloses scene.
	float l = center.x - mOrthoBoxLength;
	float b = center.y - mOrthoBoxLength;
	float n = center.z - mOrthoBoxLength;
	float r = center.x + mOrthoBoxLength;
	float t = center.y + mOrthoBoxLength;
	float f = center.z + mOrthoBoxLength;

	mLightNearZ = n;
	mLightFarZ = f;
	mLightProj = DirectX::XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	DirectX::XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	mShadowTransform = mLightView * mLightProj * T;
}

void ShadowMap::UpdatePassCB()
{
	DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply(mLightView, mLightProj);

	DirectX::XMVECTOR viewDeterminant = DirectX::XMMatrixDeterminant(mLightView);
	DirectX::XMVECTOR projDeterminant = DirectX::XMMatrixDeterminant(mLightProj);
	DirectX::XMVECTOR viewProjDeterminant = DirectX::XMMatrixDeterminant(viewProj);

	DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(&viewDeterminant, mLightView);
	DirectX::XMMATRIX invProj = DirectX::XMMatrixInverse(&projDeterminant, mLightProj);
	DirectX::XMMATRIX invViewProj = DirectX::XMMatrixInverse(&viewProjDeterminant, viewProj);

	DirectX::XMStoreFloat4x4(&mPassCB.View, DirectX::XMMatrixTranspose(mLightView));
	DirectX::XMStoreFloat4x4(&mPassCB.InvView, DirectX::XMMatrixTranspose(invView));
	DirectX::XMStoreFloat4x4(&mPassCB.Proj, DirectX::XMMatrixTranspose(mLightProj));
	DirectX::XMStoreFloat4x4(&mPassCB.InvProj, DirectX::XMMatrixTranspose(invProj));
	DirectX::XMStoreFloat4x4(&mPassCB.ViewProj, DirectX::XMMatrixTranspose(viewProj));
	DirectX::XMStoreFloat4x4(&mPassCB.InvViewProj, DirectX::XMMatrixTranspose(invViewProj));
	DirectX::XMStoreFloat3(&mPassCB.EyePosW, mLightPosW);
	mPassCB.RenderTargetSize = DirectX::XMFLOAT2((float)mWidth, (float)mHeight);
	mPassCB.InvRenderTargetSize = DirectX::XMFLOAT2(1.0f / mWidth, 1.0f / mHeight);
	mPassCB.NearZ = mLightNearZ;
	mPassCB.FarZ = mLightFarZ;

	mLight.FalloffStart = mLightNearZ;
	mLight.FalloffEnd = mLightFarZ;
	mLight.SpotPower = 64.0f;
	mLight.type = 1;
	mLight.radius = 1.0f;
	mLight.haloRadius = 1.0f;
	mLight.haloStrength = 1.0f;
	DirectX::XMStoreFloat3(&mLight.Direction, mLightDir);
	DirectX::XMStoreFloat3(&mLight.Position, mLightPosW);
	DirectX::XMStoreFloat4x4(&mLight.viewProj, DirectX::XMMatrixTranspose(mLightView));
	DirectX::XMStoreFloat4x4(&mLight.invProj, DirectX::XMMatrixTranspose(invProj));
	DirectX::XMStoreFloat4x4(&mLight.shadowTransform, DirectX::XMMatrixTranspose(mShadowTransform));

	mPassCB.Lights[0] = mLight;
}

void ShadowMap::BuildDescriptors()
{
	// Create SRV to resource so we can sample the shadow map in a shader program.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.PlaneSlice = 0;
	md3dDevice->CreateShaderResourceView(mShadowMap.Get(), &srvDesc, mhCpuSrv);

	// Create DSV to resource so we can render to the shadow map.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.Texture2D.MipSlice = 0;
	md3dDevice->CreateDepthStencilView(mShadowMap.Get(), &dsvDesc, mhCpuDsv);
}

void ShadowMap::BuildResource()
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	CD3DX12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&mShadowMap)));
}