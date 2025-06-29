#pragma once
#include <unordered_map>
#include <Windows.h>
#include <Xinput.h>
#include <unordered_map>
#include <bitset>

#pragma comment(lib, "xinput.lib")

struct KeyState
{
    bool bIsDown = false;      // 현재 프레임에 눌려있는지 여부
    bool bIsPressed = false;   // 현재 프레임에 '처음' 눌렸는지 여부
    bool bIsReleased = false;  // 현재 프레임에 '방금' 떼어졌는지 여부
};

// 게임패드 전체 상태를 추적하기 위한 구조체
struct GamepadState
{
    bool bIsConnected = false;
    XINPUT_STATE rawState{};
    std::unordered_map<WORD, KeyState> buttonStates;
};

// XInput 버튼 상수를 사용하기 쉽게 enum으로 정의
enum eGamepadButton : WORD
{
    DPAD_UP = XINPUT_GAMEPAD_DPAD_UP,
    DPAD_DOWN = XINPUT_GAMEPAD_DPAD_DOWN,
    DPAD_LEFT = XINPUT_GAMEPAD_DPAD_LEFT,
    DPAD_RIGHT = XINPUT_GAMEPAD_DPAD_RIGHT,
    START = XINPUT_GAMEPAD_START,
    BACK = XINPUT_GAMEPAD_BACK,
    LEFT_THUMB = XINPUT_GAMEPAD_LEFT_THUMB,
    RIGHT_THUMB = XINPUT_GAMEPAD_RIGHT_THUMB,
    LEFT_SHOULDER = XINPUT_GAMEPAD_LEFT_SHOULDER,
    RIGHT_SHOULDER = XINPUT_GAMEPAD_RIGHT_SHOULDER,
    A = XINPUT_GAMEPAD_A,
    B = XINPUT_GAMEPAD_B,
    X = XINPUT_GAMEPAD_X,
    Y = XINPUT_GAMEPAD_Y,
};