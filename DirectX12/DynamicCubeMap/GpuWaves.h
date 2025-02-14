//***************************************************************************************
// GpuWaves.h by Frank Luna (C) 2011 All Rights Reserved.
//
// Performs the calculations for the wave simulation using the ComputeShader on the GPU.  
// The solution is saved to a floating-point texture.  The client must then set this 
// texture as a SRV and do the displacement mapping in the vertex shader over a grid.
//***************************************************************************************

#ifndef GPUWAVES_H
#define GPUWAVES_H

#include "../EngineCore/D3DUtil.h"
#include "../EngineCore/GameTimer.h"

class GpuWaves
{
public:
	// Note that m,n should be divisible by 16 so there is no 
	// remainder when we divide into thread groups.
	GpuWaves(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, int m, int n, float dx, float dt, float speed, float damping, bool x4MsaaState, UINT x4MsaaQuality);
	GpuWaves(const GpuWaves& rhs) = delete;
	GpuWaves& operator=(const GpuWaves& rhs) = delete;
	~GpuWaves() = default;

	UINT RowCount()const;
	UINT ColumnCount()const;
	UINT VertexCount()const;
	UINT TriangleCount()const;
	float Width()const;
	float Depth()const;
	float SpatialStep()const;

	CD3DX12_GPU_DESCRIPTOR_HANDLE DisplacementMap()const;

	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGPUHandle()const;
	UINT DescriptorCount()const;

	void BuildShadersAndInputLayout();
	void BuildResources(ID3D12GraphicsCommandList* cmdList);
	void BuildCSRootSignature();
	void BuildPSO();

	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor,
		UINT descriptorSize);

	void UpdateWaves(const GameTimer& gt, ID3D12GraphicsCommandList* cmdList);

	void Update(const GameTimer& gt, ID3D12GraphicsCommandList* cmdList);

	void Disturb(ID3D12GraphicsCommandList* cmdList, UINT i, UINT j, float magnitude);

private:

	UINT mNumRows;
	UINT mNumCols;

	UINT mVertexCount;
	UINT mTriangleCount;

	bool m4xMsaaState = false;		// 4X MSAA enabled
	UINT m4xMsaaQuality = 0;		// quality level of 4X MSAA
	// Simulation constants we can precompute.
	float mK[3];

	float mTimeStep;
	float mSpatialStep;

	ID3D12Device* md3dDevice = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> mCSDisturbShader;
	Microsoft::WRL::ComPtr<ID3DBlob> mCSUpdateShader;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mCSRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mCSDisturbPSO;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mCSUpdatePSO;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mhCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mhGpuDescriptor;

	CD3DX12_GPU_DESCRIPTOR_HANDLE mPrevSolSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mCurrSolSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mNextSolSrv;

	CD3DX12_GPU_DESCRIPTOR_HANDLE mPrevSolUav;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mCurrSolUav;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mNextSolUav;

	// Two for ping-ponging the textures.
	Microsoft::WRL::ComPtr<ID3D12Resource> mPrevSol = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mCurrSol = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mNextSol = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> mPrevUploadBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mCurrUploadBuffer = nullptr;
};

#endif // GPUWAVES_H