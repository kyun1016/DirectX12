#pragma once
#include "ECSCoordinator.h"
#include "WindowComponent.h"
#include <WinUser.h>

LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

class WindowSystem : public ECS::ISystem
{
public:
    void BeginPlay() override {
        auto& coordinator = ECS::Coordinator::GetInstance();
        coordinator.RegisterSingletonComponent<WindowComponent>();

        RegisterWindowClass();
        MakeWindowHandle();
        ShowWindow(mHwnd, SW_SHOWDEFAULT);
        UpdateWindow(mHwnd);
    }

	void Sync() override {
		// 윈도우 상태 갱신
		auto& windowComponent = ECS::Coordinator::GetInstance().GetSingletonComponent<WindowComponent>();
		RECT rect;
		GetClientRect(mHwnd, &rect);
        windowComponent.left = rect.left;
        windowComponent.top = rect.top;
        windowComponent.right = rect.right;
        windowComponent.bottom = rect.bottom;

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
        RECT rect{ windowComponent.left, windowComponent.top, windowComponent.right, windowComponent.bottom };
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
        mHwnd = CreateWindow(
            /* _In_opt_ LPCWSTR lpClassName	*/ mCaption.c_str(),
            /* _In_opt_ LPCWSTR lpWindowName*/ mCaption.c_str(),
            /* _In_ DWORD dwStyle			*/ WS_OVERLAPPEDWINDOW,
            /* _In_ int X					*/ 100, // 윈도우 좌측 상단의 x 좌표
            /* _In_ int Y					*/ 100, // 윈도우 좌측 상단의 y 좌표
            /* _In_ int nWidth				*/ windowComponent.width, // 윈도우 가로 방향 해상도
            /* _In_ int nHeight				*/ windowComponent.height, // 윈도우 세로 방향 해상도
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