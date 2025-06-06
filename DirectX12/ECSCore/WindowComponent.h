#pragma once
#include <Windows.h>
#include <string>
#include <cstdint>

struct WindowComponent
{
    HWND hwnd = nullptr;
    std::wstring caption = L"App";
    uint32_t width = 1280;
    uint32_t height = 720;
    bool fullscreen = false;
};