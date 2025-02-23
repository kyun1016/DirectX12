#pragma once
#include <d3d12.h>
#include <wrl/client.h> // ComPtr

class CSAdd
{
	struct Data
	{
		DirectX::XMFLOAT3 v1;
		DirectX::XMFLOAT2 v2;
	};

public:
	CSAdd(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	CSAdd(const CSAdd& rhs) = delete;
	CSAdd& operator=(const CSAdd& rhs) = delete;
	~CSAdd() = default;

	void BuildShader();
	void BuildResources(ID3D12GraphicsCommandList* cmdList);
	void BuildRootSignature();
	void BuildPSOs();
	void DoComputeWork(ID3D12GraphicsCommandList* cmdList, ID3D12CommandAllocator* cmdAlloc);
	void PrintOutput();
private:
	static constexpr int NumDataElements = 32;

	ID3D12Device* mDevice = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> mPSO;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
	Microsoft::WRL::ComPtr<ID3DBlob> mShader;

	Microsoft::WRL::ComPtr<ID3D12Resource> mInputBufferA;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputUploadBufferA;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputBufferB;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputUploadBufferB;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutputBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> mReadBackBuffer;
};

