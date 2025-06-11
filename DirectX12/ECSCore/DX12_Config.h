#pragma once
#include <d3d12.h>
#include <wrl/client.h> // For ComPtr
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <string>
#include <Windows.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include "LogCore.h"
#include "../EngineCore/d3dx12.h"
#include "../ImGuiCore/imgui.h"
#include "../ImGuiCore/imgui_impl_dx12.h"
#include "../ImGuiCore/imgui_impl_win32.h"
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid")
#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")

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