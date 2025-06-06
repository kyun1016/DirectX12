#pragma once
#include <Windows.h>
#include <string>
#include <cstdint>

struct WindowComponent
{
    long left = 0;
    long top = 0;
    long right = 1080;
    long bottom = 720;
    bool fullscreen = false;
};