#pragma once
#include "AppBase.h"
#include <dxgi1_5.h>
#include "../EngineCore/MathHelper.h"
#include "../EngineCore/UploadBuffer.h"
#include "FrameResource.h"
#include "Waves.h"
#include <string>
#include <array>

enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	Count
};

class MyApp : public AppBase
{
	struct RenderItem
	{
		RenderItem() = default;

		// World matrix of the shape that describes the object's local space
		// relative to the world space, which defines the position, orientation,
		// and scale of the object in the world.
		DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

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
	};

	using Super = typename AppBase;

public:
	MyApp();
	MyApp(uint32_t width, uint32_t height, std::wstring name);
	virtual ~MyApp() { };

#pragma region Initialize
	virtual bool Initialize() override;
	void LoadTextures();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildLandGeometry();
	void BuildWavesGeometryBuffers();
	void BuildShapeGeometry();
	void BuildSkullGeometry();
	void BuildRoomGeometry();
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
	void AnimateMaterials();
	void UpdateObjectCBs();
	void UpdateMaterialCBs();
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

#pragma endregion Imgui
	//
	//	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	//	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	//	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

#pragma region Land
	float GetHillsHeight(const float x, const float z) const;
	DirectX::XMFLOAT3 GetHillsNormal(const float x, const float z) const;
#pragma endregion Land


private:
	static constexpr size_t	TEXTURE_NUM = 11;

	static const inline std::wstring	TEXTUER_DIR = L"../Data/Textures/";

	static const inline std::wstring	TEXTUER_FILE_NAMES[TEXTURE_NUM]
		= { L"bricks.dds", L"stone.dds", L"tile.dds", L"grass.dds"
		, L"water1.dds", L"WireFence.dds",L"WoodCrate01.dds", L"bricks3.dds"
		, L"checkboard.dds", L"ice.dds", L"white1x1.dds"
		};
	static const inline std::string	TEXTURE_NAMES[TEXTURE_NUM]
		= { "bricksTex", "stoneTex", "tileTex", "grassTex"
		, "waterTex", "fenceTex", "woodCrateTex", "bricks3Tex"
		, "checkboardTex", "iceTex", "white1x1Tex"
		};

	static const inline std::string	MESH_GEOMETRY_NAMES[5] = { "ShapeGeo", "SkullGeo", "LandGeo", "WaterGeo", "RoomGeo" };

	static constexpr size_t	MESH_MAIN_NUM = 4;
	static const inline std::string	MESH_MAIN_NAMES[MESH_MAIN_NUM] = { "box", "grid", "sphere", "cylinder" };

	static const inline std::wstring	MESH_MODEL_DIR = L"../Data/Models/";
	static constexpr size_t		MESH_MODEL_NUM = 1;
	static const inline std::wstring	MESH_MODEL_FILE_NAMES[MESH_MODEL_NUM] = { L"skull.txt" };
	static const inline std::string	MESH_MODEL_NAMES[MESH_MODEL_NUM] = { "skull" };

	static constexpr size_t		MATERIAL_NUM = 8;
	static const inline std::string	MATERIAL_NAMES[MATERIAL_NUM] = { "bricks0", "stone0", "tile0", "grass", "water", "fence", "woodCrate", "skullMat" };

	static const inline std::wstring	VS_DIR[1] = { L"Shaders\\MainVS.cso" };
	static const inline std::string	VS_NAME[1] = { "standardVS" };

	static const inline std::wstring	PS_DIR[2] = { L"Shaders\\MainPS.cso", L"Shaders\\AlphaTestedPS.cso" };
	static const inline std::string	PS_NAME[2] = { "opaquePS" , "alphaTestedPS" };

	static const inline std::string	gPSOName[6] = { "opaque", "transparent", "alphaTested", "opaque_wireframe", "transparent_wireframe", "alphaTested_wireframe" };



private:
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	std::vector<std::unique_ptr<RenderItem>> mAllRitems;
	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];
	std::unique_ptr<Waves> mWaves;
	RenderItem* mWavesRitem = nullptr;

	bool mIsWireframe = false;

	PassConstants mMainPassCB;

	DirectX::XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;		//input layout description


	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
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