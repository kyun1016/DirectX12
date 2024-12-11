#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <wrl/client.h> // ComPtr

namespace kyun
{
	using Microsoft::WRL::ComPtr;
	using std::vector;
	using std::wstring;

	// forward declaration
	class ID3D12Device;
	class ID3D12Buffer;
	class ID3D12PixelShader;
	
	inline void ThrowIfFailed(HRESULT hr) {
		if (FAILED(hr)) {
			// 디버깅할 때 여기에 breakpoint 설정
			throw std::exception();
		}
	}
	class D3D12Utils
	{
	public:
		// ComPtr<ID3D12Device>
		
		// static function
		static void CreateIndexBuffer(ComPtr<ID3D12Device>& device,
			const vector<uint32_t>& indices,
			ComPtr<ID3D12Buffer>& indexBuffer);

		static void CreatePixelShader(ComPtr<ID3D12Device>& device,
			const wstring& filename,
			ComPtr<ID3D12PixelShader>& m_pixelShader);
	};
}
