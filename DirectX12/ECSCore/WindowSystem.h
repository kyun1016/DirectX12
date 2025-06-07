#pragma once
#include "ECSCoordinator.h"
#include "WindowComponent.h"
#include <WinUser.h>
#include "InputComponent.h"

inline static LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto& wc = ECS::Coordinator::GetInstance().GetSingletonComponent<WindowComponent>();
    std::cout << "msg: " << std::hex << msg << std::hex << "  |  LPARAM: " << HIWORD(lParam) << " " << LOWORD(lParam) << "  |  WPARAM: " << HIWORD(wParam) << " " << LOWORD(wParam) << std::endl;
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
        wc.width = (UINT)LOWORD(lParam);
        wc.height = (UINT)HIWORD(lParam);
        if (wParam == SIZE_MINIMIZED)
        {
            wc.flags |= eWindowFlags::WINDOW_FLAG_PAUSED;
            wc.flags |= eWindowFlags::WINDOW_FLAG_MINIMIZED;
        }
        else
        {
            if (wParam == SIZE_MAXIMIZED)
                wc.flags |= eWindowFlags::WINDOW_FLAG_FULLSCREEN;
            else if (wParam == SIZE_RESTORED)
            {
                if (wc.flags & eWindowFlags::WINDOW_FLAG_RESIZING)
                    return 0;

                wc.flags &= ~eWindowFlags::WINDOW_FLAG_PAUSED;
                if (wc.flags & eWindowFlags::WINDOW_FLAG_MINIMIZED)
                    wc.flags &= ~eWindowFlags::WINDOW_FLAG_MINIMIZED;
                else if (wc.flags & eWindowFlags::WINDOW_FLAG_FULLSCREEN)
                    wc.flags &= ~eWindowFlags::WINDOW_FLAG_FULLSCREEN;

                // OnResize로 Render Target의 크기를 제조정 해줘야 한다. 이를 Component Update를 통해 관련 System으로 flag 전달
                // OnResize();
            }
        }
    case WM_ENTERSIZEMOVE:
        wc.flags |= eWindowFlags::WINDOW_FLAG_RESIZING;
        // mTimer.Stop();
        return 0;
    case WM_EXITSIZEMOVE:
        wc.flags &= ~eWindowFlags::WINDOW_FLAG_RESIZING;
        // mTimer.Start();
        // OnResize();
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_MENUCHAR:
        // Don't beep when we alt-enter.
        return MAKELRESULT(0, MNC_CLOSE);
    }
            
    //if (msg == WM_KEYDOWN) {
    //    auto& input = ECS::Coordinator::GetInstance().GetSingletonComponent<InputComponent>();
    //    input.keyDown[wParam] = true;
    //    return 0;
    //}
    //else if (msg == WM_KEYUP) {
    //    auto& input = ECS::Coordinator::GetInstance().GetSingletonComponent<InputComponent>();
    //    input.keyUp[wParam] = true;
    //    return 0;
    //}

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

class WindowSystem : public ECS::ISystem
{
public:
    void BeginPlay() override {
        auto& coordinator = ECS::Coordinator::GetInstance();
        coordinator.RegisterSingletonComponent<WindowComponent>();
        coordinator.RegisterSingletonComponent<InputComponent>();

        RegisterWindowClass();
        MakeWindowHandle();
        ShowWindow(mHwnd, SW_SHOWDEFAULT);
        UpdateWindow(mHwnd);
    }

	void Sync() override {
		// 윈도우 상태 갱신
		auto& windowComponent = ECS::Coordinator::GetInstance().GetSingletonComponent<WindowComponent>();
        //RECT rect;
        //GetClientRect(mHwnd, &rect);
  //      windowComponent.left = rect.left;
  //      windowComponent.top = rect.top;
  //      windowComponent.right = rect.right;
  //      windowComponent.bottom = rect.bottom;

        // 메시지 처리 루프
        MSG msg = {};
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // 필요시 윈도우 상태 갱신
	}
    void Update() override {

    }

    void EndPlay() override {
        auto& windowComponent = ECS::Coordinator::GetInstance().GetSingletonComponent<WindowComponent>();
        if (mHwnd) {
            DestroyWindow(mHwnd);
            mHwnd = nullptr;
        }
    }

private:
    WNDCLASSEXW mWindowClass;
    HWND mHwnd = nullptr;
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
            std::cout << "RegisterClassEx() failed." << std::endl;
            MessageBox(0, L"Register WindowClass Failed.", 0, 0);
            return false;
        }

        return true;
    }

    bool MakeWindowHandle()
    {
        auto& windowComponent = ECS::Coordinator::GetInstance().GetSingletonComponent<WindowComponent>();
        RECT rect{ 0, 0, static_cast<long>(windowComponent.width), static_cast<long>(windowComponent.height) };
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
        mHwnd = CreateWindow(
            /* _In_opt_ LPCWSTR lpClassName	*/ mCaption.c_str(),
            /* _In_opt_ LPCWSTR lpWindowName*/ mCaption.c_str(),
            /* _In_ DWORD dwStyle			*/ WS_OVERLAPPEDWINDOW,
            /* _In_ int X					*/ 100, // 윈도우 좌측 상단의 x 좌표
            /* _In_ int Y					*/ 100, // 윈도우 좌측 상단의 y 좌표
            /* _In_ int nWidth				*/ windowComponent.width,
            /* _In_ int nHeight				*/ windowComponent.height,
            /* _In_opt_ HWND hWndParent		*/ NULL,
            /* _In_opt_ HMENU hMenu			*/ (HMENU)0,
            /* _In_opt_ HINSTANCE hInstance	*/ mWindowClass.hInstance,
            /* _In_opt_ LPVOID lpParam		*/ NULL
        );

        if (!mHwnd) {
            std::cout << "CreateWindow() failed." << std::endl;
            MessageBox(0, L"CreateWindow Failed.", 0, 0);
            return false;
        }

        return true;
    }
};
