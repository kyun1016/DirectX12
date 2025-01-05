#pragma once
#include "AppBase.h"
#include "Waves.h"
#include <dxgi1_5.h>
#include "MathHelper.h"
#include "UploadBuffer.h"
#include "FrameResource.h"
#include <string>

static const size_t NUM_MESHES = 3;
static const std::string gMeshGeometryName[2] = { "LandGeo", "WaterGeo" };
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
	struct RenderItem
	{
		RenderItem() = default;

		// World matrix of the shape that describes the object's local space
		// relative to the world space, which defines the position, orientation,
		// and scale of the object in the world.
		DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();

		// Dirty flag indicating the object data has changed and we need to update the constant buffer.
		// Because we have an object cbuffer for each FrameResource, we have to apply the
		// update to each FrameResource.  Thus, when we modify obect data we should set 
		// NumFramesDirty = APP_NUM_FRAME_RESOURCES so that each frame resource gets the update.
		int NumFramesDirty = APP_NUM_FRAME_RESOURCES;

		// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
		UINT ObjCBIndex = -1;

		MeshGeometry* Geo = nullptr;

		// Primitive topology.
		D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		// DrawIndexedInstanced parameters.
		UINT IndexCount = 0;
		UINT StartIndexLocation = 0;
		int BaseVertexLocation = 0;
	};

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
#pragma region Imgui
	void RenderImGui() override;
#pragma endregion Imgui
	//
	//	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	//	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	//	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

private:
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	std::vector<std::unique_ptr<RenderItem>> mAllRitems;
	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];
	std::unique_ptr<Waves> mWaves;

	bool mIsWireframe = false;

	PassConstants mMainPassCB;

	DirectX::XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

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