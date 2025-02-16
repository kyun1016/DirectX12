#pragma once

#include "../EngineCore/D3DUtil.h"
#include "FrameResource.h"

class ShadowMap
{
public:
	ShadowMap(ID3D12Device* device,
		UINT width, UINT height);

	ShadowMap(const ShadowMap& rhs) = delete;
	ShadowMap& operator=(const ShadowMap& rhs) = delete;
	~ShadowMap() = default;

	UINT Width()const;
	UINT Height()const;
	ID3D12Resource* Resource();
	CD3DX12_GPU_DESCRIPTOR_HANDLE Srv()const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE Dsv()const;

	D3D12_VIEWPORT Viewport()const;
	D3D12_RECT ScissorRect()const;

	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv);

	// Shadow
	void OnResize(UINT newWidth, UINT newHeight);
	void SetBoxLength(float orthoBoxLength);
	// Light
	void SetAmbientLight(const DirectX::SimpleMath::Vector4& ambientLight);
	void SetPosition(const DirectX::SimpleMath::Vector3& pos);
	void SetBaseDir(const DirectX::SimpleMath::Vector3& dir);
	void SetRotate(const float angle);
	void SetTarget(const DirectX::SimpleMath::Vector3& targetPos);
	
	
	void Update();
	void UpdateMatrix();
	void UpdatePassCB();

private:
	void BuildDescriptors();
	void BuildResource();

private:

	ID3D12Device* md3dDevice = nullptr;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R24G8_TYPELESS;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuDsv;

	Microsoft::WRL::ComPtr<ID3D12Resource> mShadowMap = nullptr;

	DirectX::BoundingSphere mSceneBounds;

	// Light
	DirectX::XMVECTOR mBaseLightDir;
	DirectX::XMVECTOR mLightPosW;
	DirectX::XMVECTOR mLightDir;
	DirectX::XMMATRIX mLightView;
	DirectX::XMMATRIX mLightProj;

	// Shadow
	float mLightNearZ = 0.0f;
	float mLightFarZ = 0.0f;
	DirectX::XMVECTOR mTargetPos;
	float mOrthoBoxLength = 100.0f;
	DirectX::XMMATRIX mShadowTransform;

	PassConstants mPassCB;
};