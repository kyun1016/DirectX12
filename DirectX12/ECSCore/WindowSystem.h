#pragma once
#include "ECSCoordinator.h"
#include "WindowComponent.h"

class WindowSystem : public ECS::ISystem
{
public:
    void BeginPlay() override {
        auto& WindowComponent = ECS::Coordinator::GetInstance().GetSingletonComponent<WindowComponent>();
        if (!wc.hwnd) {
            wc.hwnd = CreateAppWindow(wc);
        }
    }

    void Update() override {
        // 메시지 처리 루프
        MSG msg = {};
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // 필요시 윈도우 상태 갱신
    }

    void EndPlay() override {
        auto& WindowComponent = ECS::Coordinator::GetInstance().GetComponent<WindowComponent>(entity);
        if (WindowComponent.hwnd) {
            DestroyWindow(WindowComponent.hwnd);
            WindowComponent.hwnd = nullptr;
        }
    }

private:
    WNDCLASSEXW mWindowClass;
    HWND mHwndWindow;

    HWND CreateAppWindow(const WindowComponent& wc) {
        // WNDCLASSEX 등록, CreateWindowEx 호출 등
        // 필요시 wc.caption, wc.width, wc.height, wc.fullscreen 사용
        // ...
        return hwnd;
    }
};