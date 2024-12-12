#pragma once
#include<d3d12.h>


namespace kyun
{
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
	};
}


