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
#include "ShadowMap.h"
#include <string>
#include <array>

class MyApp : public AppBase
{
#pragma region Constant
private:
	static constexpr int MAX_LAYER_DEPTH = 20;
	static constexpr int SRV_IMGUI_SIZE = 64;
	
	static const inline std::vector<std::wstring>	TEX_DIFF_FILENAMES = {
		// STD TEX
		L"bricks.dds",L"bricks2.dds",L"bricks3.dds", L"checkboard.dds", L"grass.dds",
		L"ice.dds", L"stone.dds", L"tile.dds", L"WireFence.dds", L"WoodCrate01.dds",
		L"WoodCrate02.dds", L"water1.dds", L"white1x1.dds", L"tree01S.dds", L"tree02S.dds",
		L"tree35S.dds",
	};
	static const inline std::vector<std::wstring>	TEX_NORM_FILENAMES = {
		// STD TEX
		L"default_nmap.dds", L"bricks_nmap.dds",L"bricks2_nmap.dds",L"tile_nmap.dds"
	};
	static const inline std::vector<std::wstring>	TEX_ARRAY_FILENAMES = {
		// Array Tex (for Billboard Shader)
		L"treearray.dds", L"treeArray2.dds",
	};
	static const inline std::vector<std::wstring>	TEX_CUBE_FILENAMES = {
		// Array Tex (for Billboard Shader)
		L"desertcube1024.dds", L"grasscube1024.dds",L"snowcube1024.dds",L"sunsetcube1024.dds",
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

	enum class RenderLayer : int
	{
		None = 0,
		Opaque,
		Mirror,
		Reflected,
		AlphaTested,
		Transparent,
		Subdivision,
		Normal,
		TreeSprites,
		Tessellation,
		BoundingBox,
		BoundingSphere,
		CubeMap,
		ShadowMap,
		DebugShadowMap,
		OpaqueWireframe,
		MirrorWireframe,
		ReflectedWireframe,
		AlphaTestedWireframe,
		TransparentWireframe,
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
	struct RenderItem
	{
		RenderItem() = default;
		RenderItem(MeshGeometry* geo, SubmeshGeometry submesh, bool culling = true)
			: Geo(geo)
			, IndexCount(submesh.IndexCount)
			, StartIndexLocation(submesh.StartIndexLocation)
			, BaseVertexLocation(submesh.BaseVertexLocation)
			, BoundingBox(submesh.BoundingBox)
			, BoundingSphere(submesh.BoundingSphere)
			, mFrustumCullingEnabled(culling)
		{
		}

		void Push(DirectX::SimpleMath::Vector3 translation, DirectX::SimpleMath::Vector3 scale, DirectX::SimpleMath::Quaternion rot, DirectX::SimpleMath::Vector3 texScale, UINT boundingCount = 0, UINT matIdx = 0, bool cull = true)
		{
			Datas.push_back(EXInstanceData(BoundingBox, BoundingSphere, translation, scale, rot, texScale, boundingCount, matIdx, cull));
		}

		void Push(EXInstanceData data)
		{
			Datas.push_back(data);
		}

		MeshGeometry* Geo = nullptr;

		// Primitive topology.
		D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		DirectX::BoundingBox BoundingBox;
		DirectX::BoundingSphere BoundingSphere;
		UINT StartInstanceLocation = 0;
		UINT InstanceCount = 0;
		bool mFrustumCullingEnabled = false;
		std::vector<EXInstanceData> Datas;			// CPU 및 알고리즘 연산을 위한 데이터

		int LayerFlag
			= (1 << (int)RenderLayer::Opaque)
			| (1 << (int)RenderLayer::Reflected)
			| (1 << (int)RenderLayer::Normal)
			| (1 << (int)RenderLayer::OpaqueWireframe)
			| (1 << (int)RenderLayer::ReflectedWireframe)
			| (1 << (int)RenderLayer::NormalWireframe);

		// DrawIndexedInstanced parameters.
		UINT IndexCount = 0;
		UINT StartIndexLocation = 0;
		int BaseVertexLocation = 0;

		int NumFramesDirty = APP_NUM_BACK_BUFFERS;
	};

	using Super = typename AppBase;

public:
	MyApp();
	MyApp(uint32_t width, uint32_t height, std::wstring name);
	virtual ~MyApp();

#pragma region Initialize
	virtual bool Initialize() override;
	void LoadTextures();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	GeometryGenerator::MeshData LoadModelMesh(std::wstring dir);
	void BuildMeshes();
	void BuildGeometry(std::vector<GeometryGenerator::MeshData>& meshes, const DXGI_FORMAT indexFormat = DXGI_FORMAT_R16_UINT);
	void BuildTreeSpritesGeometry();
	void BuildMaterials();
	void BuildRenderItems();
	void BuildFrameResources();
	void BuildPSO();
#pragma endregion Initialize

private:
	virtual void CreateRtvAndDsvDescriptorHeaps(UINT numRTV, UINT numDSV)override;
#pragma region Update
	virtual void OnResize()override;
	virtual void Update()override;
	virtual void Render()override;
	virtual void Sync()override;

	void AnimateMaterials();
	void UpdateTangents();
	void UpdateShadowMap();
	void UpdateInstanceBuffer();
	void UpdateMaterialBuffer();
	void UpdateMainPassCB();
	void UpdateShadowPassCB();

	void DrawRenderItems(const RenderLayer ritems);
	void DrawSceneToShadowMap(int index = 0);

	void Pick();
	std::pair<int, int> PickClosest(const DirectX::SimpleMath::Ray& pickingRay, float& minDist);

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
	void ShowCubeMapWindow();
	bool mShowDemoWindow = false;
	bool mShowTextureWindow = false;
	bool mShowMaterialWindow = false;
	bool mShowRenderItemWindow = false;
	bool mShowViewportWindow = false;
	bool mShowCubeMapWindow = false;

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
	std::vector<std::unique_ptr<EXMaterialData>> mAllMatItems;
	std::pair<int, int> mPickModel = { -1,-1 };
	UINT mInstanceCount = 0;

	std::unique_ptr<Waves> mWaves;
	std::unique_ptr<CSAdd> mCSAdd;
	std::unique_ptr<BlurFilter> mCSBlurFilter;
	std::unique_ptr<GpuWaves> mCSWaves;
	std::array<std::unique_ptr<ShadowMap>, MAX_LIGHTS> mShadowMap;

	PassConstants mMainPassCB;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mMainInputLayout;		//input layout description
	std::vector<D3D12_INPUT_ELEMENT_DESC> mTreeSpriteInputLayout;

	std::vector<GeometryGenerator::MeshData> mMeshes;
	std::vector<std::unique_ptr<MeshGeometry>> mGeometries;
	
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::wstring, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<RenderLayer, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;

	bool mCullingEnable = false;

	bool mUpdateBoundingMesh = false;
	DirectX::BoundingFrustum mCamFrustum;
	Camera mCamera;

	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUUser;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUDiff;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUNorm;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUShadow;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUArray;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUCube;
};