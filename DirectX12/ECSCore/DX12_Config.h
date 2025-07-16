#pragma once
#define NOMINMAX 1
#include <limits>
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
#include <d3dcompiler.h>
#include <stdint.h>
#include <algorithm>
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

// DirectXMath constants
// DirectX::XM_PIDIV4
constexpr float XM_PI = 3.141592654f;
constexpr float XM_2PI = 6.283185307f;
constexpr float XM_1DIVPI = 0.318309886f;
constexpr float XM_1DIV2PI = 0.159154943f;
constexpr float XM_PIDIV2 = 1.570796327f;
constexpr float XM_PIDIV4 = 0.785398163f;

using float4x4 = DirectX::SimpleMath::Matrix;
using float2 = DirectX::SimpleMath::Vector2;
using float3 = DirectX::SimpleMath::Vector3;
using float4 = DirectX::SimpleMath::Vector4;
using uint = uint32_t;
using uint2 = DirectX::XMUINT2;
using uint3 = DirectX::XMUINT3;
using uint4 = DirectX::XMUINT4;
using int2 = DirectX::XMINT2;
using int3 = DirectX::XMINT3;
using int4 = DirectX::XMINT4;
using quat = DirectX::SimpleMath::Quaternion;

static constexpr std::uint32_t APP_NUM_BACK_BUFFERS = 3;

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

#define ThrowIfFailed(hr) \
	if (FAILED(hr)) { \
		LogCore::GetInstance().Log(eLogLevel::Error, eConsoleForeground::RED, __FILE__, __LINE__, __func__, "DirectX 12 Error: {}", std::to_string(hr)); \
    }
// 		throw std::exception();\
// 	}

inline UINT CalcConstantBufferByteSize(UINT byteSize)
{
	// Constant buffers must be a multiple of the minimum hardware
	// allocation size (usually 256 bytes).  So round up to nearest
	// multiple of 256.  We do this by adding 255 and then masking off
	// the lower 2 bytes which store all bits < 256.
	// Example: Suppose byteSize = 300.
	// (300 + 255) & ~255
	// 555 & ~255
	// 0x022B & ~0x00ff
	// 0x022B & 0xff00
	// 0x0200
	// 512
	return (byteSize + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
};

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
			mElementByteSize = CalcConstantBufferByteSize(sizeof(T));
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

static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	const void* initData,
	UINT64 byteSize,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
	Microsoft::WRL::ComPtr<ID3D12Resource> defaultBuffer;

	// 기본 버퍼 자원 생성
	D3D12_HEAP_PROPERTIES defaultHeapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapProperty,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

	// CPU 메모리의 자료를 기본 버퍼에 복사하기 위해 임시 업로드 힙 생성
	D3D12_HEAP_PROPERTIES uploadHeapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeapProperty,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf())));


	// Describe the data we want to copy into the default buffer.
	D3D12_SUBRESOURCE_DATA subResourceData
	{
		/* const void* pData    */ initData,
		/* LONG_PTR RowPitch    */ (__int64)byteSize,
		/* LONG_PTR SlicePitch  */ (__int64)byteSize
	};

	// 기본 버퍼 자원으로 자료 복사 요청
	// CPU 메모리를 임시 업로드 힙에 복사하고, ID3D12CommandList::CopySubresourceResion을 이용해서 임시 업로드 힙의 자료를 mBuffer에 복사
	UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);

	D3D12_RESOURCE_BARRIER barrier
		= CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	cmdList->ResourceBarrier(1, &barrier);

	// 주의: 위의 함수 호출 이후에도 uploadBuffer를 계속 유지해야 함
	// 복사가 완료된 이후 uploadBuffer 소멸
	return defaultBuffer;
}