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
#include "SsaoMap.h"
#include <string>
#include <array>

class MyApp : public AppBase
{
#pragma region Constant
private:
	static constexpr int MAX_LAYER_DEPTH = 20;
	static constexpr int SRV_IMGUI_SIZE = 64;

	enum class RenderLayer : int
	{
		None = 0,
		Opaque,
		SkinnedOpaque,
		Mirror,
		Reflected,
		AlphaTested,
		Transparent,
		Subdivision,
		Normal,
		SkinnedNormal,
		TreeSprites,
		Tessellation,
		BoundingBox,
		BoundingSphere,
		CubeMap,
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
		ShadowMap,
		SkinnedShadowMap,
		AddCS,
		BlurCS,
		WaveCS,
		ShaderToy,
		Count
	};
#pragma endregion Constant

	struct SkinnedModelInstance
	{
		SkinnedData SkinnedInfo;
		std::vector<DirectX::XMFLOAT4X4> FinalTransforms;
		std::string ClipName;
		float TimePos = 0.0f;

		// Called every frame and increments the time position, interpolates the 
		// animations for each bone based on the current animation clip, and 
		// generates the final transforms which are ultimately set to the effect
		// for processing in the vertex shader.
		void UpdateSkinnedAnimation(float dt)
		{
			TimePos += dt;

			// Loop animation
			if (TimePos > SkinnedInfo.GetClipEndTime(ClipName))
				TimePos = 0.0f;

			// Compute the final transforms for this time position.
			SkinnedInfo.GetFinalTransforms(ClipName, TimePos, FinalTransforms);
		}
	};

	struct RenderItem
	{
		RenderItem() = default;
		RenderItem(const RenderItem& rhs) = delete;
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

		void Push(DirectX::SimpleMath::Vector3 translation, DirectX::SimpleMath::Vector3 scale,
			DirectX::SimpleMath::Quaternion rot, DirectX::SimpleMath::Vector3 texScale, UINT boundingCount = 0, UINT matIdx = 0, bool cull = true)
		{
			Datas.push_back(EXInstanceData(&BoundingBox, &BoundingSphere, translation, scale, rot, texScale, boundingCount, matIdx, cull));
		}

		void Push(EXInstanceData data)
		{
			Datas.push_back(data);
		}

		int NumFramesDirty = APP_NUM_FRAME_RESOURCES;
		MeshGeometry* Geo = nullptr;

		int LayerFlag 
			= (1 << (int)RenderLayer::Opaque)
			| (1 << (int)RenderLayer::Reflected)
			| (1 << (int)RenderLayer::Normal)
			| (1 << (int)RenderLayer::OpaqueWireframe)
			| (1 << (int)RenderLayer::ReflectedWireframe)
			| (1 << (int)RenderLayer::NormalWireframe);

		UINT IndexCount = 0;
		UINT StartIndexLocation = 0;
		int BaseVertexLocation = 0;

		// Primitive topology.
		bool mFrustumCullingEnabled = false;
		DirectX::BoundingBox BoundingBox;
		DirectX::BoundingSphere BoundingSphere;
		
		// DrawIndexedInstanced parameters.
		UINT StartInstanceLocation = 0;
		UINT InstanceCount = 0;
		std::vector<EXInstanceData> Datas;			// CPU 및 알고리즘 연산을 위한 데이터
		
		// Skinned Mesh
		UINT SkinnedCBIndex = -1;
		SkinnedModelInstance* SkinnedModelInst = nullptr;
	};

	using Super = typename AppBase;

public:
	MyApp();
	MyApp(uint32_t width, uint32_t height, std::wstring name);
	virtual ~MyApp();

#pragma region Initialize
	virtual bool Initialize() override;
	void LoadDLLs();

	void LoadTextures();
	void LoadTextures(const std::vector<std::wstring>& filename, std::unordered_map<std::wstring, std::unique_ptr<Texture>>& texMap);
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildTexture2DSrv(const std::unordered_map<std::wstring, std::unique_ptr<Texture>>& texMap, D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc, CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE& hGpuSrv, UINT descriptorSize);
	void BuildTexture2DArraySrv(const std::unordered_map<std::wstring, std::unique_ptr<Texture>>& texMap, D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc, CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE& hGpuSrv, UINT descriptorSize);
	void BuildTextureCubeSrv(const std::unordered_map<std::wstring, std::unique_ptr<Texture>>& texMap, D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc, CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE& hGpuSrv, UINT descriptorSize);
	void BuildShadersAndInputLayout();
	GeometryGenerator::MeshData LoadModelMesh(std::string dir);
	void LoadSkinnedModelMesh(const std::string& dir);
	void BuildMeshes();
	void BuildGeometry(std::vector<GeometryGenerator::MeshData>& meshes, bool useIndex16 = true, bool useSkinnedMesh = false);
	void BuildTreeSpritesGeometry();
	void BuildMaterials();
	void BuildRenderItems();
	void BuildFrameResources();
	void BuildPSOs();

	
#pragma endregion Initialize

private:
	virtual void CreateRtvAndDsvDescriptorHeaps(UINT numRTV, UINT numDSV, UINT numRTVST)override;
#pragma region Update
	virtual void OnResize()override;
	virtual void Update()override;
	virtual void Render()override;
	virtual void Sync()override;

	void UpdateMaterials();
	void UpdateTangents();
	void UpdateShadowMap();
	void UpdateInstanceBuffer();
	void UpdateMaterialBuffer();
	void UpdatePassCB();
	void UpdateSkinnedCB();
	void UpdateShaderToyCB();

	void DrawRenderItems(const RenderLayer ritems);
	void DrawSceneToShadowMap(int index = 0);
	void DrawShaderToy(int index = 0);

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
	void ShowInstanceWindow();
	void ShowLightWindow();
	void ShowViewportWindow();
	void ShowCubeMapWindow();
	bool mShowDemoWindow = false;
	bool mShowTextureWindow = false;
	bool mShowMaterialWindow = false;
	bool mShowInstanceWindow = false;
	bool mShowLightWindow = false;
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

	void CopyRTV(const int srvIdx);
private:
	RenderLayer mLayerType[MAX_LAYER_DEPTH];
	int mLayerStencil[MAX_LAYER_DEPTH];
	int mLayerCBIdx[MAX_LAYER_DEPTH];

	std::vector<std::unique_ptr<RenderItem>> mAllRitems;
	std::vector<std::unique_ptr<EXMaterialData>> mAllMatItems;
	std::pair<int, int> mPickModel = { 0,0 };
	UINT mInstanceCount = 0;

	std::unique_ptr<Waves> mWaves;
	std::unique_ptr<CSAdd> mCSAdd;
	std::unique_ptr<BlurFilter> mCSBlurFilter;
	std::unique_ptr<GpuWaves> mCSWaves;
	std::array<std::unique_ptr<ShadowMap>, MAX_LIGHTS> mShadowMap;
	std::array<bool, MAX_LIGHTS> mUseShadowMap;
	std::unique_ptr<SsaoMap> mSsaoMap;

	PassConstants mMainPassCB;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mMainInputLayout;		//input layout description
	std::vector<D3D12_INPUT_ELEMENT_DESC> mTreeSpriteInputLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mSkinnedInputLayout;

	std::vector<GeometryGenerator::MeshData> mMeshes;
	std::vector<GeometryGenerator::MeshData> mSkinnedMeshes;
	std::vector<std::unique_ptr<MeshGeometry>> mGeometries;
	
	std::unordered_map<std::wstring, std::unique_ptr<Texture>> mDisplacementTex;
	std::unordered_map<std::wstring, std::unique_ptr<Texture>> mDiffuseTex;
	std::unordered_map<std::wstring, std::unique_ptr<Texture>> mNormalTex;
	std::unordered_map<std::wstring, std::unique_ptr<Texture>> mAOTex;
	std::unordered_map<std::wstring, std::unique_ptr<Texture>> mMetallicTex;
	std::unordered_map<std::wstring, std::unique_ptr<Texture>> mRoughnessTex;
	std::unordered_map<std::wstring, std::unique_ptr<Texture>> mEmissiveTex;
	std::unordered_map<std::wstring, std::unique_ptr<Texture>> mTreeMapTex;
	std::unordered_map<std::wstring, std::unique_ptr<Texture>> mCubeMapTex;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<RenderLayer, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;

	std::vector<Microsoft::WRL::ComPtr<ID3DBlob>> mST_Shaders;
	std::vector<Microsoft::WRL::ComPtr<ID3D12PipelineState>> mST_PSOs;

	bool mCullingEnable = false;

	bool mUpdateBoundingMesh = false;
	DirectX::BoundingFrustum mCamFrustum;
	Camera mCamera;

	UINT mHeapDisplacementIdx = 0;
	UINT mHeapDiffIdx = 0;
	UINT mHeapNormIdx = 0;
	UINT mHeapAOIdx = 0;
	UINT mHeapMetallicIdx = 0;
	UINT mHeapRoughnessIdx = 0;
	UINT mHeapEmissiveIdx = 0;
	UINT mHeapShadowIdx = 0;
	UINT mHeapSsaoIdx = 0;
	UINT mHeapArrayIdx = 0;
	UINT mHeapCubeIdx = 0;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUUser;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUDiff;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUDisplacement;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUNorm;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUAO;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUMetallic;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPURoughness;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUEmissive;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUShadow;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUSsao;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUArray;
	D3D12_GPU_DESCRIPTOR_HANDLE mhGPUCube;


	D3D12_VERTEX_BUFFER_VIEW mLastVertexBufferView;
	D3D12_INDEX_BUFFER_VIEW mLastIndexBufferView;
	D3D12_PRIMITIVE_TOPOLOGY mLastPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

	// Temp
	std::unique_ptr<SkinnedModelInstance> mSkinnedModelInst;
	std::vector<M3DLoader::M3dMaterial> mSkinnedMats;
};