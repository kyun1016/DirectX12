#pragma once

#include "../EngineCore/D3DUtil.h"
#include "FrameResource.h"

class ShadowMap
{
public:
	ShadowMap(
		ID3D12Device* device,
		UINT width,
		UINT height,
		const DirectX::SimpleMath::Vector3 lightDirection = { 0.57735f, -0.57735f, 0.57735f },
		const DirectX::SimpleMath::Vector3 lightStrength = { 0.6f, 0.6f, 0.6f },
		const DirectX::SimpleMath::Vector3 targetPosition = {0.0f, 0.0f, 0.0f},
		const DirectX::SimpleMath::Vector4 ambientLight = { 0.25f, 0.25f, 0.35f, 1.0f },
		float orthoBoxLength = 100.0f);

	ShadowMap(const ShadowMap& rhs) = delete;
	ShadowMap& operator=(const ShadowMap& rhs) = delete;
	~ShadowMap() = default;

	PassConstants GetPassCB() const;
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
	void SetLightStrength(const DirectX::SimpleMath::Vector3& strength);
	void SetBaseDir(const DirectX::SimpleMath::Vector3& dir);
	void SetRotate(const float angle);
	void SetTarget(const DirectX::SimpleMath::Vector3& targetPos);

private:
	void Update();
	void UpdateMatrix();
	void UpdatePassCB();

	void BuildDescriptors();
	void BuildResource();

private:
	ID3D12Device* md3dDevice;

	D3D12_VIEWPORT mViewport;
	D3D12_RECT mScissorRect;

	UINT mWidth;
	UINT mHeight;
	DXGI_FORMAT mFormat;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuDsv;

	Microsoft::WRL::ComPtr<ID3D12Resource> mShadowMap = nullptr;

	// Light
	DirectX::XMVECTOR mBaseLightDir;
	DirectX::XMVECTOR mLightPosW;
	DirectX::XMVECTOR mLightDir;
	DirectX::XMMATRIX mLightView;
	DirectX::XMMATRIX mLightProj;

	// Shadow
	float mLightNearZ;
	float mLightFarZ;
	DirectX::XMVECTOR mTargetPos;
	float mOrthoBoxLength;
	DirectX::XMMATRIX mShadowTransform;
	PassConstants mPassCB;
	LightData mLight;
};