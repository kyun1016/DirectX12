#include "pch.h"
#include "ShadowMap.h"

ShadowMap::ShadowMap(ID3D12Device* device, UINT width, UINT height)
{
	md3dDevice = device;

	mWidth = width;
	mHeight = height;

	mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	mScissorRect = { 0, 0, (int)width, (int)height };

	BuildResource();
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

void ShadowMap::SetPosition(const DirectX::SimpleMath::Vector3& pos)
{
	mLightPosW = XMLoadFloat3(&pos);
	Update();
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

void ShadowMap::Update()
{
	UpdateMatrix();
	UpdatePassCB();
}

void ShadowMap::UpdateMatrix()
{
	DirectX::XMVECTOR lightUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	DirectX::XMMATRIX lightView = DirectX::XMMatrixLookAtLH(mLightPosW, mTargetPos, lightUp);

	// Transform bounding sphere to light space.
	DirectX::XMFLOAT3 center;
	DirectX::XMStoreFloat3(&center, XMVector3TransformCoord(mTargetPos, lightView));

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
	DirectX::XMStoreFloat3(&mPassCB.Lights[0].Direction, mLightDir);
	mPassCB.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
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