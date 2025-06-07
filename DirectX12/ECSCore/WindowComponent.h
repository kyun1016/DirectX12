#pragma once
#include <Windows.h>
#include <string>
#include <cstdint>
#include "ECSConfig.h"

enum class eWindowFlags : std::uint32_t
{
	WINDOW_FLAG_PAUSED = 1 << 0,
	WINDOW_FLAG_MINIMIZED = 1 << 1,
	WINDOW_FLAG_FULLSCREEN = 1 << 2,
	WINDOW_FLAG_RESIZING = 1 << 3,
	WINDOW_FLAG_TORESIZE = 1 << 4
};

ENUM_OPERATORS_32(eWindowFlags)
struct WindowComponent
{
	HWND hwnd = nullptr;
    std::uint32_t width = 1080;
	std::uint32_t height = 720;

	eWindowFlags flags;
};