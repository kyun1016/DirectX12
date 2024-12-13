#pragma once
#include<d3d12.h>
#include <wrl/client.h> // ComPtr

namespace kyun
{
	using Microsoft::WRL::ComPtr;

	class AppBase
	{
	public:
		AppBase();
		virtual ~AppBase();

		int Run();
		virtual bool Initialize();
		virtual bool InitScene();

		virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	protected:
		bool InitMainWindow();
		bool InitDirect3D();
		bool InitGUI();
	public:
		int m_screenWidth;
		int m_screenHeight;
		HWND m_mainWindow;
		ComPtr<ID3D12Device> m_device;
		ComPtr<ID3D12DeviceContext> m_context;
	};
}


