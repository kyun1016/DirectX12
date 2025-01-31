#pragma once
#include "AppBase.h"
#include <dxgi1_5.h>
#include "../EngineCore/MathHelper.h"
#include "../EngineCore/UploadBuffer.h"
#include "FrameResource.h"
#include "Waves.h"
#include "BlurFilter.h"
#include "GpuWaves.h"
#include "CSAdd.h"
#include <string>
#include <array>

class MyApp : public AppBase
{
#pragma region Constant
private:
	static constexpr int MAX_LAYER_DEPTH = 20;
	static constexpr int SRV_IMGUI_SIZE = 64;
	static const inline std::wstring				TEXTURE_DIR = L"../Data/Textures/";
	static constexpr int SIZE_STD_TEX = 16;
	static constexpr int SIZE_ARY_TEX = SIZE_STD_TEX + 2;
	static const inline std::vector<std::wstring>	TEXTURE_FILENAMES = {
		// STD TEX
		L"bricks.dds",L"bricks2.dds",L"bricks3.dds", L"checkboard.dds", L"grass.dds",
		L"ice.dds", L"stone.dds", L"tile.dds", L"WireFence.dds", L"WoodCrate01.dds",
		L"WoodCrate02.dds", L"water1.dds", L"white1x1.dds", L"tree01S.dds", L"tree02S.dds",
		L"tree35S.dds",

		// Array Tex (for Billboard Shader)
		L"treearray.dds", L"treeArray2.dds",
	};

	static constexpr int SIZE_STD_MAT = SIZE_STD_TEX;
	static constexpr int SIZE_ARY_MAT = SIZE_STD_MAT + 2;
	static constexpr int SIZE_USR_MAT = SIZE_ARY_MAT + 3;
	static const inline std::vector<std::string>	MATERIAL_NAMES = {
		// STD TEX Material
		"bricks", "bricks2", "bricks3", "checkboard", "grass",
		"ice", "stone", "tile", "WireFence", "WoodCrate01",
		"WoodCrate02", "water1", "white1x1", "tree01S", "tree02S",
		"tree35S",

		// Array TEX Material
		"treearray", "treeArray2",

		// User Define Material
		"skullMat", "shadowMat", "viewport"
	};

	static const inline std::vector<std::pair<std::string, std::vector<std::string>>> GEO_MESH_NAMES = {
		{"ShapeGeo", std::vector<std::string>({"box", "grid", "sphere", "cylinder" })},
		{"ModelGeo", std::vector<std::string>({"skull"})},
		{"LandGeo",	std::vector<std::string>({"land"})},
		{"WaterGeo", std::vector<std::string>({"water"})},
		{"RoomGeo", std::vector<std::string>({"floor", "wall", "mirror"})},
		{"TreeSpritesGeo", std::vector<std::string>({"points"})}
	};

	static const inline std::wstring				MESH_MODEL_DIR = L"../Data/Models/";
	static const inline std::vector<std::wstring>	MESH_MODEL_FILE_NAMES = {
		L"skull.txt"
	};

	static const inline std::vector<std::wstring>	VS_DIR = { L"Shaders\\MainVS.cso", L"Shaders\\SubdivisionVS.cso", L"Shaders\\NormalVS.cso", L"Shaders\\BillboardVS.cso", L"Shaders\\WaveVS.cso" };
	static const inline std::vector<std::string>	VS_NAME = { "standardVS", "subdivisionVS", "normalVS", "billboardVS", "waveVS" };
	static const inline std::vector<std::wstring>	GS_DIR = { L"Shaders\\SubdivisionGS.cso", L"Shaders\\NormalGS.cso", L"Shaders\\BillboardGS.cso" };
	static const inline std::vector<std::string>	GS_NAME = { "subdivisionGS", "normalGS", "billboardGS" };
	static const inline std::vector<std::wstring>	CS_DIR = { L"Shaders\\AddCS.cso", L"Shaders\\BlurHorCS.cso", L"Shaders\\BlurVerCS.cso", L"Shaders\\WaveDisturbCS.cso", L"Shaders\\WaveUpdateCS.cso"};
	static const inline std::vector<std::string>	CS_NAME = { "AddCS", "BlurHorCS", "BlurVerCS", "WaveDisturbCS", "WaveUpdateCS"};
	static const inline std::vector<std::wstring>	PS_DIR = { L"Shaders\\MainPS.cso", L"Shaders\\AlphaTestedPS.cso", L"Shaders\\NormalPS.cso", L"Shaders\\BillboardPS.cso" };
	static const inline std::vector<std::string>	PS_NAME = { "opaquePS" , "alphaTestedPS", "normalPS", "billboardPS" };

	enum class RenderLayer : int
	{
		None = 0,
		Opaque,
		Mirror,
		Reflected,
		AlphaTested,
		Transparent,
		Shadow,
		Subdivision,
		Normal,
		TreeSprites,
		OpaqueWireframe,
		MirrorWireframe,
		ReflectedWireframe,
		AlphaTestedWireframe,
		TransparentWireframe,
		ShadowWireframe,
		SubdivisionWireframe,
		NormalWireframe,
		TreeSpritesWireframe,
		AddCS,
		BlurHorCS,
		BlurVerCS,
		WaveVS,
		WaveDisturbCS,
		WaveUpdateCS,
		Count
	};
#pragma endregion Constant
	struct RenderItem
	{
		RenderItem() = default;

		// World matrix of the shape that describes the object's local space
		// relative to the world space, which defines the position, orientation,
		// and scale of the object in the world.
		DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

		// Used for GPU waves render items.
		DirectX::XMFLOAT2 DisplacementMapTexelSize = { 1.0f, 1.0f };
		float GridSpatialStep = 1.0f;

		// Dirty flag indicating the object data has changed and we need to update the constant buffer.
		// Because we have an object cbuffer for each FrameResource, we have to apply the
		// update to each FrameResource.  Thus, when we modify obect data we should set 
		// NumFramesDirty = APP_NUM_FRAME_RESOURCES so that each frame resource gets the update.
		int NumFramesDirty = APP_NUM_FRAME_RESOURCES;

		// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
		UINT ObjCBIndex = -1;

		Material* Mat = nullptr;
		MeshGeometry* Geo = nullptr;

		// Primitive topology.
		D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		// DrawIndexedInstanced parameters.
		UINT IndexCount = 0;
		UINT StartIndexLocation = 0;
		int BaseVertexLocation = 0;

		int LayerFlag
			= (1 << (int)RenderLayer::Opaque)
			| (1 << (int)RenderLayer::Reflected)
			| (1 << (int)RenderLayer::Shadow)
			| (1 << (int)RenderLayer::Normal)
			| (1 << (int)RenderLayer::OpaqueWireframe)
			| (1 << (int)RenderLayer::ReflectedWireframe)
			| (1 << (int)RenderLayer::ShadowWireframe)
			| (1 << (int)RenderLayer::NormalWireframe);
		float WorldX = 0.0f, WorldY = 0.0f, WorldZ = 0.0f;
		float AngleX = 0.0f, AngleY = 0.5f, AngleZ = 0.0f;
		float ScaleX = 1.0f, ScaleY = 1.0f, ScaleZ = 1.0f;
		float OffsetX = 0.0f, OffsetY = 0.0f, OffsetZ = 0.0f;
	};

	using Super = typename AppBase;

public:
	MyApp();
	MyApp(uint32_t width, uint32_t height, std::wstring name);
	virtual ~MyApp() {};

#pragma region Initialize
	virtual bool Initialize() override;
	void LoadTextures();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShaderResourceViews();
	void BuildCSBlurShaderResourceViews();
	void BuildCSWavesShaderResourceViews();
	void BuildShadersAndInputLayout();
	void BuildShapeGeometry();
	void BuildModelGeometry();
	void BuildLandGeometry();
	void BuildCSWavesGeometry();
	void BuildWavesGeometryBuffers();
	void BuildRoomGeometry();
	void BuildTreeSpritesGeometry();
	void BuildMaterials();
	void BuildRenderItems();
	void BuildFrameResources();
	void BuildPSO();

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();
#pragma endregion Initialize

private:
#pragma region Update
	virtual void OnResize()override;
	virtual void Update()override;
	virtual void Render()override;

	void AnimateMaterials();
	void UpdateObjectCBs();
	void UpdateMaterialCBs();
	void UpdateMainPassCB();
	void UpdateReflectedPassCB();
	void UpdateWaves();

	void DrawRenderItems(const RenderLayer ritems);

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void OnKeyboardInput();
	void UpdateCamera();
#pragma endregion Update
#pragma region Imgui
	virtual void UpdateImGui() override;
	void ShowMainWindow();
	void MakePSOCheckbox(const std::string type, bool& flag, bool& flag2);
	void ShowTextureWindow();
	void ShowMaterialWindow();
	void ShowRenderItemWindow();
	void ShowViewportWindow();
	bool mShowDemoWindow = false;
	bool mShowTextureWindow = false;
	bool mShowMaterialWindow = false;
	bool mShowRenderItemWindow = false;
	bool mShowViewportWindow = false;

	int mImguiIdxTexture = 0;
	int mImguiWidth = 0;
	int mImguiHeight = 0;
#pragma endregion Imgui

#pragma region Land
	float GetHillsHeight(const float x, const float z) const;
	DirectX::XMFLOAT3 GetHillsNormal(const float x, const float z) const;
#pragma endregion Land

private:
	RenderLayer mLayerType[MAX_LAYER_DEPTH];
	int mLayerStencil[MAX_LAYER_DEPTH];
	int mLayerCBIdx[MAX_LAYER_DEPTH];

	std::vector<std::unique_ptr<RenderItem>> mAllRitems;
	std::unique_ptr<Waves> mWaves;
	RenderItem* mWavesRitem = nullptr;

	std::unique_ptr<CSAdd> mCSAdd;
	std::unique_ptr<BlurFilter> mCSBlurFilter;
	std::unique_ptr<GpuWaves> mCSWaves;

	PassConstants mMainPassCB;
	PassConstants mReflectedPassCB;

	DirectX::XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mCSWavesRootSignature = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mMainInputLayout;		//input layout description
	std::vector<D3D12_INPUT_ELEMENT_DESC> mTreeSpriteInputLayout;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::wstring, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<RenderLayer, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;

	DirectX::XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 mView = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 1.5f * DirectX::XM_PI;
	float mPhi = DirectX::XM_PIDIV4;
	float mRadius = 5.0f;

	POINT mLastMousePos;
};