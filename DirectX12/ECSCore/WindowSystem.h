#pragma once
#include "ECSCoordinator.h"
#include "WindowComponent.h"
#include <WinUser.h>
#include "InputSystem.h"
#include "ImGuiSystem.h" // ImGui WndProc 핸들러를 위해 추가

extern LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

class WindowSystem
{
public: 
    inline static WindowSystem& GetInstance() {
        static WindowSystem instance;
        return instance;
    } 

public:
    void Initialize()
    {
        auto& coordinator = ECS::Coordinator::GetInstance();

        RegisterWindowClass();
        MakeWindowHandle();
        ShowWindow(mWindowComponent.hwnd, SW_SHOWDEFAULT);
        UpdateWindow(mWindowComponent.hwnd);
    }

	void Sync() {
		LOG_INFO("WindowSystem::Sync() called");
        // 메시지 처리 루프
        MSG msg = {};
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // 필요시 윈도우 상태 갱신
	}

    WindowComponent& GetWindowComponent()
    {
		return mWindowComponent;
    }

    LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        LOG_INFO("msg: {} |  LPARAM: {} {} |  WPARAM: {} {}", msg, (int)HIWORD(lParam), (int)LOWORD(lParam), (int)HIWORD(wParam), (int)LOWORD(wParam));
        // ImGui가 입력을 처리하도록 먼저 전달
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
            return true;

        auto& inputSystem = ECS::InputSystem::GetInstance();

        switch (msg)
        {
        case WM_CREATE:
            return 0;
            // WM_ACTIVATE is sent when the window is activated or deactivated.  
            // We pause the game when the window is deactivated and unpause it 
            // when it becomes active.  
        case WM_ACTIVATE:
            //if (LOWORD(wParam) == WA_INACTIVE)
            //{
            //	mAppPaused = true;
            //	mTimer.Stop();
            //}
            //else
            //{
            //	mAppPaused = false;
            //	mTimer.Start();
            //}
            return 0;
        case WM_SIZE:
            mWindowComponent.width = (UINT)LOWORD(lParam);
            mWindowComponent.height = (UINT)HIWORD(lParam);
            if (wParam == SIZE_MINIMIZED)
            {
                mWindowComponent.flags |= eWindowFlags::WINDOW_FLAG_PAUSED;
                mWindowComponent.flags |= eWindowFlags::WINDOW_FLAG_MINIMIZED;
            }
            else
            {
                if (wParam == SIZE_MAXIMIZED)
                    mWindowComponent.flags |= eWindowFlags::WINDOW_FLAG_FULLSCREEN;
                else if (wParam == SIZE_RESTORED)
                {
                    if (mWindowComponent.flags & eWindowFlags::WINDOW_FLAG_RESIZING)
                        return 0;

                    mWindowComponent.flags &= ~eWindowFlags::WINDOW_FLAG_PAUSED;
                    if (mWindowComponent.flags & eWindowFlags::WINDOW_FLAG_MINIMIZED)
                        mWindowComponent.flags &= ~eWindowFlags::WINDOW_FLAG_MINIMIZED;
                    else if (mWindowComponent.flags & eWindowFlags::WINDOW_FLAG_FULLSCREEN)
                        mWindowComponent.flags &= ~eWindowFlags::WINDOW_FLAG_FULLSCREEN;

                    // OnResize로 Render Target의 크기를 제조정 해줘야 한다. 이를 Component Update를 통해 관련 System으로 flag 전달
                    // OnResize();
                }
            }
            return 0;
        case WM_ENTERSIZEMOVE:
            mWindowComponent.flags |= eWindowFlags::WINDOW_FLAG_RESIZING;
            // mTimer.Stop();
            return 0;
        case WM_EXITSIZEMOVE:
            mWindowComponent.flags &= ~eWindowFlags::WINDOW_FLAG_RESIZING;
            // mTimer.Start();
            // OnResize();
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_MENUCHAR:
            // Don't beep when we alt-enter.
            return MAKELRESULT(0, MNC_CLOSE);
            // --- 키보드 입력 ---
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            inputSystem.OnKeyDown(wParam);
            return 0;
        case WM_KEYUP:
        case WM_SYSKEYUP:
            inputSystem.OnKeyUp(wParam);
            return 0;
            // --- 마우스 입력 ---
        case WM_MOUSEMOVE:      inputSystem.OnMouseMove(lParam); return 0;
        case WM_LBUTTONDOWN:    inputSystem.OnMouseDown(0); return 0;
        case WM_LBUTTONUP:      inputSystem.OnMouseUp(0); return 0;
        case WM_RBUTTONDOWN:    inputSystem.OnMouseDown(1); return 0;
        case WM_RBUTTONUP:      inputSystem.OnMouseUp(1); return 0;
        case WM_MBUTTONDOWN:    inputSystem.OnMouseDown(2); return 0;
        case WM_MBUTTONUP:      inputSystem.OnMouseUp(2); return 0;
        case WM_XBUTTONDOWN:    inputSystem.OnMouseDown(HIWORD(wParam) == XBUTTON1 ? 3 : 4); return 0;
        case WM_XBUTTONUP:      inputSystem.OnMouseUp(HIWORD(wParam) == XBUTTON1 ? 3 : 4); return 0;
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

private:
    WindowSystem() = default;
    ~WindowSystem()
    {
        if (mWindowComponent.hwnd) {
            DestroyWindow(mWindowComponent.hwnd);
            mWindowComponent.hwnd = nullptr;
        }
    };
    WindowSystem(const WindowSystem&) = delete;
    WindowSystem& operator=(const WindowSystem&) = delete;
    WindowSystem(WindowSystem&&) = delete;
    WindowSystem& operator=(WindowSystem&&) = delete;
private:
    WNDCLASSEXW mWindowClass;
	WindowComponent mWindowComponent;
    std::wstring mCaption = L"App";

    bool RegisterWindowClass()
    {
        mWindowClass = {
            /*UINT        cbSize		*/	.cbSize = sizeof(mWindowClass),
            /*UINT        style			*/	.style = CS_CLASSDC,
            /*WNDPROC     lpfnWndProc	*/	.lpfnWndProc = WndProc,
            /*int         cbClsExtra	*/	.cbClsExtra = 0L,
            /*int         cbWndExtra	*/	.cbWndExtra = 0L,
            /*HINSTANCE   hInstance		*/	.hInstance = GetModuleHandle(nullptr),
            /*HICON       hIcon			*/	.hIcon = nullptr,
            /*HCURSOR     hCursor		*/	.hCursor = nullptr,
            /*HBRUSH      hbrBackground	*/	.hbrBackground = nullptr,
            /*LPCWSTR     lpszMenuName	*/	.lpszMenuName = nullptr,
            /*LPCWSTR     lpszClassName	*/	.lpszClassName = mCaption.c_str(), // lpszClassName, L-string
            /*HICON       hIconSm		*/	.hIconSm = nullptr
        };

        if (!RegisterClassEx(&mWindowClass)) {
            LOG_ERROR("RegisterClassEx() failed.");
            MessageBox(0, L"Register WindowClass Failed.", 0, 0);
            return false;
        }

        return true;
    }

    bool MakeWindowHandle()
    {
        RECT rect{ 0, 0, static_cast<long>(mWindowComponent.width), static_cast<long>(mWindowComponent.height) };
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
        mWindowComponent.hwnd = CreateWindow(
            /* _In_opt_ LPCWSTR lpClassName	*/ mCaption.c_str(),
            /* _In_opt_ LPCWSTR lpWindowName*/ mCaption.c_str(),
            /* _In_ DWORD dwStyle			*/ WS_OVERLAPPEDWINDOW,
            /* _In_ int X					*/ 100, // 윈도우 좌측 상단의 x 좌표
            /* _In_ int Y					*/ 100, // 윈도우 좌측 상단의 y 좌표
            /* _In_ int nWidth				*/ mWindowComponent.width,
            /* _In_ int nHeight				*/ mWindowComponent.height,
            /* _In_opt_ HWND hWndParent		*/ NULL,
            /* _In_opt_ HMENU hMenu			*/ (HMENU)0,
            /* _In_opt_ HINSTANCE hInstance	*/ mWindowClass.hInstance,
            /* _In_opt_ LPVOID lpParam		*/ NULL
        );

        if (!mWindowComponent.hwnd) {
            LOG_ERROR("CreateWindow() failed.");

            MessageBox(0, L"CreateWindow Failed.", 0, 0);
            return false;
        }

        return true;
    }
};

inline static LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return WindowSystem::GetInstance().MsgProc(hwnd, msg, wParam, lParam);
}

//inline static LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
//    return WindowSystem::g_WindowSystem->MsgProc(hwnd, msg, wParam, lParam);
//}
//
//class WindowSystem
//{
//    inline static WindowSystem* g_WindowSystem;
//public:
//    inline static WindowSystem* GetInstance()
//    {
//        if (!g_WindowSystem)
//        {
//            g_WindowSystem = new WindowSystem();
//        }
//
//        return g_WindowSystem;
//    }