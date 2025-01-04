#pragma once
#include "AppBase.h"
#include "Waves.h"
#include <dxgi1_5.h>
#include "MathHelper.h"
#include "UploadBuffer.h"
#include "FrameResource.h"
#include <string>

static const size_t NUM_MESHES = 3;
static const std::string gMeshGeometryName = "Geo";
static const std::string VS_NAME = "standardVS";
static const std::string PS_NAME = "opaquePS";
static const std::string gSubmeshName[NUM_MESHES] = { "box", "grid", "wave" };
static const std::string gPSOName[2] = { "opaque", "opaque_wireframe" };

enum class RenderLayer : int
{
	Opaque = 0,
	Count
};

class LandAndWavesApp : public AppBase
{
	using Super = typename AppBase;

public:
	LandAndWavesApp();
	LandAndWavesApp(uint32_t width, uint32_t height, std::wstring name);
	virtual ~LandAndWavesApp() { };

#pragma region Initialize
	virtual bool Initialize() override;
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildGeometry();
	void BuildWavesGeometryBuffers();
	void BuildRenderItems();
	void BuildFrameResources();
	void BuildPSO();
#pragma endregion Initialize

private:
	float GetHillsHeight(float x, float z)const;
	DirectX::XMFLOAT3 GetHillsNormal(float x, float z)const;
#pragma region Update
	virtual void OnResize()override;
	virtual void Update()override;
	void UpdateObjectCBs();
	void UpdateMainPassCB();
	void UpdateWaves();

	virtual void Render()override;
	void DrawRenderItems(const std::vector<RenderItem*>& ritems);

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void OnKeyboardInput();
	void UpdateCamera();
#pragma endregion Update

	//
	//	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	//	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	//	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

private:
	bool mMovable = false;

	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	std::vector<std::unique_ptr<RenderItem>> mAllRitems;
	std::vector<RenderItem*> mOpaqueRitems[(int)RenderLayer::Count];
	std::unique_ptr<Waves> mWaves;

	UINT mPassCbvOffset = 0;
	bool mIsWireframe = false;

	PassConstants mMainPassCB;

	DirectX::XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

	std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;

	Microsoft::WRL::ComPtr<ID3DBlob> mVSByteCode = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> mPSByteCode = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;		//input layout description

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;

	DirectX::XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 mView = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 1.5f * DirectX::XM_PI;
	float mPhi = DirectX::XM_PIDIV4;
	float mRadius = 5.0f;

	POINT mLastMousePos;
};