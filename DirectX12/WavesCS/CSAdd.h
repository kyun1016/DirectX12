#pragma once
class CSAdd
{

private:
	static constexpr int NumDataElements = 32;

	struct Data
	{
		DirectX::XMFLOAT3 v1;
		DirectX::XMFLOAT2 v2;
	};

	Microsoft::WRL::ComPtr<ID3D12Resource> mInputBufferA = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputUploadBufferA = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputBufferB = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInputUploadBufferB = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mOutputBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> mReadBackBuffer = nullptr;
};

