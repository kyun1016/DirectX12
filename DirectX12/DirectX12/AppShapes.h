#pragma once

#include "AppBase.h"
#include <dxgi1_5.h>
#include "d3dx12.h"
#include <directxtk/SimpleMath.h>
#include "MathHelper.h"
#include "UploadBuffer.h"

class AppShapes : public AppBase
{
	using Super = typename AppBase;

public:
	AppShapes();
	AppShapes(uint32_t width, uint32_t height, std::wstring name);
	virtual ~AppShapes() { };

//	virtual bool Initialize() override;
//	virtual void CleanUp() override;
private:
//	virtual void OnResize()override;
#pragma region Update
	virtual void Update(const GameTimer dt)override;
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);

#pragma endregion Update
	virtual void Render(const GameTimer dt)override;
//
//	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
//	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
//	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;
//	void BuildDescriptorHeaps();
//	void BuildConstantBuffers();
//	void BuildRootSignature();
//	void BuildShadersAndInputLayout();
//	void BuildBoxGeometry();
//	void BuildPSO();
	void BuildFrameResources();
	
private:
	bool mMovable = false;
	

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

	std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;

	Microsoft::WRL::ComPtr<ID3DBlob> mVSByteCode = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> mPSByteCode = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;		//input layout description

	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO = nullptr;
	
	std::unique_ptr<MeshGeometry> mBoxGeo;

	DirectX::XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 mView = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 1.5f * DirectX::XM_PI;
	float mPhi = DirectX::XM_PIDIV4;
	float mRadius = 5.0f;

	POINT mLastMousePos;
};
