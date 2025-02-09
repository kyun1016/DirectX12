#pragma once
#include "AppBase.h"
#include <dxgi1_5.h>
#include "../EngineCore/MathHelper.h"
#include "../EngineCore/UploadBuffer.h"
#include "../EngineCore/Camera.h"
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
	static const inline std::vector<std::wstring>	TEXTURE_FILENAMES = {
		// STD TEX
		L"bricks.dds",L"bricks2.dds",L"bricks3.dds", L"checkboard.dds", L"grass.dds",
		L"ice.dds", L"stone.dds", L"tile.dds", L"WireFence.dds", L"WoodCrate01.dds",
		L"WoodCrate02.dds", L"water1.dds", L"white1x1.dds", L"tree01S.dds", L"tree02S.dds",
		L"tree35S.dds",
	};
	static const inline std::vector<std::wstring>	TEXTURE_ARRAY_FILENAMES = {
		// Array Tex (for Billboard Shader)
		L"treearray.dds", L"treeArray2.dds",
	};

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
		{"ShapeGeo", std::vector<std::string>({"box", "grid", "sphere", "cylinder"})},
		{"ModelGeo", std::vector<std::string>({"skull"})},
		{"LandGeo",	std::vector<std::string>({"land"})},
		{"WaterGeo", std::vector<std::string>({"water"})},
		{"RoomGeo", std::vector<std::string>({"floor", "wall", "mirror"})},
		{"TreeSpritesGeo", std::vector<std::string>({"points"})},
		{"QuadPatchGeo", std::vector<std::string>({"quadpatch"})}
	};

	static const inline std::wstring				MESH_MODEL_DIR = L"../Data/Models/";
	static const inline std::vector<std::wstring>	MESH_MODEL_FILE_NAMES = {
		L"skull.txt"
	};

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
		Tessellation,
		OpaqueWireframe,
		MirrorWireframe,
		ReflectedWireframe,
		AlphaTestedWireframe,
		TransparentWireframe,
		ShadowWireframe,
		SubdivisionWireframe,
		NormalWireframe,
		TreeSpritesWireframe,
		TessellationWireframe,
		AddCS,
		BlurCS,
		WaveVS_CS,
		WaveVS_CS_Wireframe,
		Count
	};
#pragma endregion Constant
	struct RootData
	{
		RootData() = default;
		RootData(float tranX = 0.0f, float tranY = 0.0f, float tranZ = 0.0f, float scale = 1.0f)
			: Translation(tranX, tranY, tranZ)
			, Scale(scale, scale, scale)
			, RotationQuat(1.f, 1.f, 1.f, 0.f)
		{
		}

		DirectX::XMFLOAT3 Translation;
		DirectX::XMFLOAT3 Scale;
		DirectX::XMFLOAT4 RotationQuat;
	};

	struct RenderItem
	{
		RenderItem() = default;

		// Dirty flag indicating the object data has changed and we need to update the constant buffer.
		// Because we have an object cbuffer for each FrameResource, we have to apply the
		// update to each FrameResource.  Thus, when we modify obect data we should set 
		// NumFramesDirty = APP_NUM_FRAME_RESOURCES so that each frame resource gets the update.
		int NumFramesDirty = APP_NUM_FRAME_RESOURCES;

		MeshGeometry* Geo = nullptr;

		// Primitive topology.
		D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		UINT StartInstanceLocation = 0;
		UINT InstanceCount = 0;
		std::vector<InstanceData> Instances;

		int LayerFlag
			= (1 << (int)RenderLayer::Opaque)
			| (1 << (int)RenderLayer::Reflected)
			| (1 << (int)RenderLayer::Shadow)
			| (1 << (int)RenderLayer::Normal)
			| (1 << (int)RenderLayer::OpaqueWireframe)
			| (1 << (int)RenderLayer::ReflectedWireframe)
			| (1 << (int)RenderLayer::ShadowWireframe)
			| (1 << (int)RenderLayer::NormalWireframe);

		std::vector<RootData> Datas;

		// DrawIndexedInstanced parameters.
		UINT IndexCount = 0;
		UINT StartIndexLocation = 0;
		int BaseVertexLocation = 0;		
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
	void BuildShadersAndInputLayout();
	void BuildShapeGeometry();
	void BuildModelGeometry();
	void BuildLandGeometry();
	void BuildCSWavesGeometry();
	void BuildRoomGeometry();
	void BuildTreeSpritesGeometry();
	void BuildQuadPatchGeometry();
	void BuildMaterials();
	void BuildRenderItems();
	void BuildFrameResources();
	void BuildPSO();
#pragma endregion Initialize

private:
#pragma region Update
	virtual void OnResize()override;
	virtual void Update()override;
	virtual void Render()override;
	virtual void Sync()override;

	void AnimateMaterials();
	void UpdateInstanceBuffer();
	void UpdateMaterialBuffer();
	void UpdateMainPassCB();
	void UpdateReflectedPassCB();

	void DrawRenderItems(const RenderLayer ritems);

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void OnKeyboardInput();
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
	UINT mInstanceCount = 0;

	std::unique_ptr<Waves> mWaves;
	RenderItem* mWavesRitem = nullptr;

	std::unique_ptr<CSAdd> mCSAdd;
	std::unique_ptr<BlurFilter> mCSBlurFilter;
	std::unique_ptr<GpuWaves> mCSWaves;

	PassConstants mMainPassCB;
	PassConstants mReflectedPassCB;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mMainInputLayout;		//input layout description
	std::vector<D3D12_INPUT_ELEMENT_DESC> mTreeSpriteInputLayout;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::wstring, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<RenderLayer, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;

	Camera mCamera;

	POINT mLastMousePos;
};