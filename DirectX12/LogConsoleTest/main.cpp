#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <functional>
#include <mutex>
#include <atomic>
#include <map>
#include <Windows.h>
#include <assert.h>

#include "sl_core_types.h"
#include "log.h"

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	sl::Preferences pref;
	pref.showConsole = true;
	auto log = sl::log::getInterface();
	log->enableConsole(pref.showConsole);
	log->setLogLevel(pref.logLevel);
	log->setLogPath(pref.pathToLogsAndData);
	log->setLogCallback((void*)pref.logMessageCallback);
	log->setLogName(L"sl.log");

	// void ConfigureLogOverridesFromEnvironment(log::ILog * log)
	{
		// log->enableConsole(std::atoi(value.c_str()) != 0);
		// log->setLogLevel(ToLogLevel(std::atoi(value.c_str())));
		// log->setLogPath(extra::toWStr(value).c_str());
		// log->setLogName(extra::toWStr(value).c_str());
	}
	


	SL_LOG_INFO("Hello from WinMain!");
	SL_LOG_INFO("Hello from WinMain!");
	SL_LOG_INFO("Hello from WinMain!");
	SL_LOG_INFO("Hello from WinMain!");
	SL_LOG_INFO("Hello from WinMain!");

	
	sl::log::destroyInterface();

	Sleep(100000000);

	return 0;
}
#endif