#pragma once
#include <unordered_map>
#include <Windows.h>

struct InputComponent {
    std::unordered_map<UINT, bool> keyDown;    // 눌림 여부
    std::unordered_map<UINT, bool> keyUp;      // 뗀 여부
};

struct MouseComponent {
    int x = 0;
    int y = 0;
    bool leftDown = false;
    bool rightDown = false;
};