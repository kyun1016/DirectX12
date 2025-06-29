#pragma once
#include "ECSCoordinator.h"
#include "InputComponent.h"
#include <windowsx.h>

namespace ECS {
class InputSystem
{
    DEFAULT_SINGLETON(InputSystem)
public:
    void Initialize()
    {
        m_CurrentKeyRawState.reset();
        m_PreviousKeyRawState.reset();
        m_CurrentMouseRawState.reset();
        m_PreviousMouseRawState.reset();
        m_MousePosition = { 0.0f, 0.0f };
        m_PreviousMousePosition = { 0.0f, 0.0f };
    }

    // 매 프레임 시작 시 호출되어 모든 입력 상태를 갱신합니다.
    void PreUpdate()
    {
	    UpdateKeyboardState();
	    UpdateMouseState();
	    UpdateGamepadStates();
    }

    // --- Public API ---
    // 키보드
    bool IsKeyDown(int vk_code) const // 키가 눌려있는 동안 true
    {
        if (const auto it = m_KeyStates.find(vk_code); it != m_KeyStates.end()) {
	    	return it->second.bIsDown;
	    }
	    return false;
    }
    bool IsKeyPressed(int vk_code) const // 키가 처음 눌린 프레임에만 true
    {
        if (const auto it = m_KeyStates.find(vk_code); it != m_KeyStates.end()) {
	    	return it->second.bIsPressed;
	    }
	    return false;
    }
    bool IsKeyReleased(int vk_code) const // 키를 뗀 프레임에만 true
    {
        if (const auto it = m_KeyStates.find(vk_code); it != m_KeyStates.end()) {
	    	return it->second.bIsReleased;
	    }
	    return false;
    }

    // 마우스
    bool IsMouseButtonDown(int button) const // 0:L, 1:R, 2:M
    {
        if (const auto it = m_MouseButtonStates.find(button); it != m_MouseButtonStates.end()) {
	    	return it->second.bIsDown;
	    }
	    return false;
    }
    bool IsMouseButtonPressed(int button) const
    {
        if (const auto it = m_MouseButtonStates.find(button); it != m_MouseButtonStates.end()) {
	    	return it->second.bIsPressed;
	    }
	    return false;
    }
    bool IsMouseButtonReleased(int button) const
    {
        if (const auto it = m_MouseButtonStates.find(button); it != m_MouseButtonStates.end()) {
	    	return it->second.bIsReleased;
	    }
	    return false;
    }
    float2 GetMousePosition() const
    {
        return m_MousePosition;
    }
    float2 GetMouseDelta() const
    {
        return { m_MousePosition.x - m_PreviousMousePosition.x, m_MousePosition.y - m_PreviousMousePosition.y };
    }

    // 게임패드
    const GamepadState& GetGamepadState(int controllerIndex) const
    {
        return m_GamepadStates[controllerIndex];
    }
    bool IsGamepadButtonDown(int controllerIndex, WORD button) const
    {
        if (const auto it = m_GamepadStates[controllerIndex].buttonStates.find(button); it != m_GamepadStates[controllerIndex].buttonStates.end()) {
            return it->second.bIsDown;
        }
        return false;
    }
    bool IsGamepadButtonPressed(int controllerIndex, WORD button) const
    {
        if (const auto it = m_GamepadStates[controllerIndex].buttonStates.find(button); it != m_GamepadStates[controllerIndex].buttonStates.end()) {
	    	return it->second.bIsPressed;
	    }
	    return false;
    }
    bool IsGamepadButtonReleased(int controllerIndex, WORD button) const
    {
        if (const auto it = m_GamepadStates[controllerIndex].buttonStates.find(button); it != m_GamepadStates[controllerIndex].buttonStates.end()) {
            return it->second.bIsReleased;
        }
        return false;
    }

    // --- Raw Input Handlers (WindowSystem의 WndProc에서 호출) ---
    void OnKeyDown(WPARAM wParam) { m_CurrentKeyRawState[wParam] = true; }
    void OnKeyUp(WPARAM wParam) { m_CurrentKeyRawState[wParam] = false; }
    void OnMouseMove(LPARAM lParam)
    {
    	m_MousePosition.x = static_cast<float>(GET_X_LPARAM(lParam));
    	m_MousePosition.y = static_cast<float>(GET_Y_LPARAM(lParam));
    }
    void OnMouseDown(int button) { m_CurrentMouseRawState[button] = true; }
    void OnMouseUp(int button) { m_CurrentMouseRawState[button] = false; }

private:
    void UpdateKeyboardState()
    {
        for (int i = 0; i < 256; ++i) {
            m_KeyStates[i].bIsPressed = m_CurrentKeyRawState[i] && !m_PreviousKeyRawState[i];
            m_KeyStates[i].bIsReleased = !m_CurrentKeyRawState[i] && m_PreviousKeyRawState[i];
            m_KeyStates[i].bIsDown = m_CurrentKeyRawState[i];
        }
        m_PreviousKeyRawState = m_CurrentKeyRawState;
    }
    void UpdateMouseState()
    {
        for (int i = 0; i < 5; ++i) {
            m_MouseButtonStates[i].bIsPressed = m_CurrentMouseRawState[i] && !m_PreviousMouseRawState[i];
            m_MouseButtonStates[i].bIsReleased = !m_CurrentMouseRawState[i] && m_PreviousMouseRawState[i];
            m_MouseButtonStates[i].bIsDown = m_CurrentMouseRawState[i];
        }
        m_PreviousMouseRawState = m_CurrentMouseRawState;
        m_PreviousMousePosition = m_MousePosition;
    }
    void UpdateGamepadStates()
    {
        for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i) {
            GamepadState& state = m_GamepadStates[i];
            XINPUT_STATE previousRawState = state.rawState;

            ZeroMemory(&state.rawState, sizeof(XINPUT_STATE));

            DWORD dwResult = XInputGetState(i, &state.rawState);

            if (dwResult == ERROR_SUCCESS) {
                state.bIsConnected = true;

                // 모든 버튼에 대해 상태 업데이트
                const WORD allButtons[] = {
                    XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_RIGHT,
                    XINPUT_GAMEPAD_START, XINPUT_GAMEPAD_BACK,
                    XINPUT_GAMEPAD_LEFT_THUMB, XINPUT_GAMEPAD_RIGHT_THUMB,
                    XINPUT_GAMEPAD_LEFT_SHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                    XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_B, XINPUT_GAMEPAD_X, XINPUT_GAMEPAD_Y
                };

                for (WORD button : allButtons) {
                    bool isCurrentlyDown = (state.rawState.Gamepad.wButtons & button) != 0;
                    bool wasPreviouslyDown = (previousRawState.Gamepad.wButtons & button) != 0;

                    state.buttonStates[button].bIsDown = isCurrentlyDown;
                    state.buttonStates[button].bIsPressed = isCurrentlyDown && !wasPreviouslyDown;
                    state.buttonStates[button].bIsReleased = !isCurrentlyDown && wasPreviouslyDown;
                }
            }
            else {
                if (state.bIsConnected) {
                    // 컨트롤러가 방금 연결 해제됨
                    state.bIsConnected = false;
                    state.buttonStates.clear();
                }
            }
        }
    }

    // 키보드 상태
    std::unordered_map<int, KeyState> m_KeyStates;
    std::bitset<256> m_CurrentKeyRawState;
    std::bitset<256> m_PreviousKeyRawState;

    // 마우스 상태
    std::unordered_map<int, KeyState> m_MouseButtonStates;
    std::bitset<5> m_CurrentMouseRawState; // L, R, M, X1, X2
    std::bitset<5> m_PreviousMouseRawState;
    float2 m_MousePosition;
    float2 m_PreviousMousePosition;

    // 게임패드 상태
    std::array<GamepadState, XUSER_MAX_COUNT> m_GamepadStates;
};
}
