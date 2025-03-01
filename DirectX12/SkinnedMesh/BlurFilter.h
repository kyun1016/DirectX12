#pragma once

#include "../EngineCore/D3DUtil.h"

class BlurFilter
{
public:
	///<summary>
	/// The width and height should match the dimensions of the input texture to blur.
	/// Recreate when the screen is resized. 
	///</summary>
	BlurFilter(ID3D12Device* device,
		UINT width, UINT height,
		DXGI_FORMAT format);

	BlurFilter(const BlurFilter& rhs) = delete;
	BlurFilter& operator=(const BlurFilter& rhs) = delete;
	~BlurFilter() = default;

	UINT DescriptorCount()const;

	ID3D12Resource* Output();

	void BuildShader();
	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor,
		UINT descriptorSize);
	void BuildRootSignature();
	void BuildPSOs();

	void OnResize(UINT newWidth, UINT newHeight);

	///<summary>
	/// Blurs the input texture blurCount times.
	///</summary>
	void Execute(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* input, int blurCount);

private:
	std::vector<float> CalcGaussWeights(float sigma);

	void BuildDescriptors();
	void BuildResources();

private:

	const int MaxBlurRadius = 5;

	ID3D12Device* md3dDevice = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> mHorShader;
	Microsoft::WRL::ComPtr<ID3DBlob> mVerShader;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mHorPSO;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mVerPSO;	

	UINT mWidth = 0;
	UINT mHeight = 0;
	DXGI_FORMAT mFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur0CpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur0CpuUav;

	CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur1CpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE mBlur1CpuUav;

	CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur0GpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur0GpuUav;

	CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur1GpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur1GpuUav;

	// Two for ping-ponging the textures.
	Microsoft::WRL::ComPtr<ID3D12Resource> mBlurMap0 = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mBlurMap1 = nullptr;
};
