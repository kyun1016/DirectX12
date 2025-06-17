#pragma once
#include <string>
#include <unordered_map>
#include <d3d12.h>
#include <wrl/client.h> // For ComPtr
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <Windows.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include "LogCore.h"
#include "../EngineCore/d3dx12.h"
#include "../EngineCore/SimpleMath.h"
#include "../ImGuiCore/imgui.h"
#include "../ImGuiCore/imgui_impl_dx12.h"
#include "../ImGuiCore/imgui_impl_win32.h"
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid")
#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")

using float4x4 = DirectX::SimpleMath::Matrix;
using float4 = DirectX::SimpleMath::Vector4;
using float3 = DirectX::SimpleMath::Vector3;
using float2 = DirectX::SimpleMath::Vector2;

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif

inline std::string WStringToString(const std::wstring& wstr)
{
	if (wstr.empty()) return std::string();

	int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0,
		wstr.c_str(), (int)wstr.size(),
		nullptr, 0, nullptr, nullptr);

	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0,
		wstr.c_str(), (int)wstr.size(),
		&result[0], sizeNeeded, nullptr, nullptr);

	return result;
}

inline std::wstring StringToWString(const std::string& str)
{
	if (str.empty()) return std::wstring();

	int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0,
		str.c_str(), (int)str.size(),
		nullptr, 0);

	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0,
		str.c_str(), (int)str.size(),
		&result[0], sizeNeeded);

	return result;
}

inline static void ThrowIfFailed(HRESULT hr) {
	if (FAILED(hr)) {
		// 디버깅할 때 여기에 breakpoint 설정
		LOG_ERROR("DirectX 12 Error: {}", std::to_string(hr));
		throw std::exception();
	}
}

#define DEFAULT_SINGLETON(SystemClassName)                        \
public:                                                           \
    inline static SystemClassName& GetInstance() {                \
        static SystemClassName instance;                          \
        return instance;                                          \
    }                                                             \
private:                                                          \
    SystemClassName() = default;                                  \
    ~SystemClassName() = default;                                 \
    SystemClassName(const SystemClassName&) = delete;             \
    SystemClassName& operator=(const SystemClassName&) = delete;  \
    SystemClassName(SystemClassName&&) = delete;                  \
    SystemClassName& operator=(SystemClassName&&) = delete;       

template<typename T>
class UploadBuffer
{
public:
	UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) :
		mIsConstantBuffer(isConstantBuffer)
	{
		mElementByteSize = sizeof(T);

		// Constant buffer elements need to be multiples of 256 bytes.
		// This is because the hardware can only view constant data 
		// at m*256 byte offsets and of n*256 byte lengths. 
		// typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
		// UINT64 OffsetInBytes; // multiple of 256
		// UINT   SizeInBytes;   // multiple of 256
		// } D3D12_CONSTANT_BUFFER_VIEW_DESC;
		if (isConstantBuffer)
			mElementByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(T));
		CD3DX12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize * elementCount);
		ThrowIfFailed(device->CreateCommittedResource(
			&heapProperty,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mUploadBuffer)));

		ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));

		// We do not need to unmap until we are done with the resource.  However, we must not write to
		// the resource while it is in use by the GPU (so we must use synchronization techniques).
	}

	UploadBuffer(const UploadBuffer& rhs) = delete;
	UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
	~UploadBuffer()
	{
		if (mUploadBuffer != nullptr)
			mUploadBuffer->Unmap(0, nullptr);

		mMappedData = nullptr;
	}

	ID3D12Resource* Resource()const
	{
		return mUploadBuffer.Get();
	}

	void CopyData(int elementIndex, const T& data)
	{
		memcpy(&mMappedData[elementIndex * mElementByteSize], &data, sizeof(T));
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
	BYTE* mMappedData = nullptr;

	UINT mElementByteSize = 0;
	bool mIsConstantBuffer = false;
};