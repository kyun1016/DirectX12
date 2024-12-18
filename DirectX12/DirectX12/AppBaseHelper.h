#pragma once
#include <stdexcept>
#include <iostream>
#include <windows.h>
#include "AppBase.h"

#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")

// using Microsoft::WRL::ComPtr;

// Singleton object so that worker threads can share members.
static AppBase* g_appBase = nullptr;

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	return g_appBase->MsgProc(hWnd, msg, wParam, lParam);
}

inline std::string HrToString(HRESULT hr)
{
	char s_str[64] = {};
	sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
	return std::string(s_str);
}

inline void GetAssetsPath(_Out_writes_(pathSize) WCHAR* path, UINT pathSize)
{
	if (path == nullptr)
	{
		throw std::exception();
	}

	DWORD size = GetModuleFileName(nullptr, path, pathSize);
	if (size == 0 || size == pathSize)
	{
		// Method failed or path was truncated.
		throw std::exception();
	}

	WCHAR* lastSlash = wcsrchr(path, L'\\');
	if (lastSlash)
	{
		*(lastSlash + 1) = L'\0';
	}
}