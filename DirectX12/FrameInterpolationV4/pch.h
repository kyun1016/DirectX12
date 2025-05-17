#pragma once

#define _ST

// 해당 파일은 streamline compile 결과
#pragma comment(lib, "..\\StreamlineCore\\streamline\\lib\\x64\\sl.interposer.lib")

#ifdef _DEBUG
#pragma comment(lib, "..\\Libraries\\Libs\\EngineCore\\Debug\\EngineCore.lib")
// #pragma comment(lib, "..\\Libraries\\Libs\\StreamlineCore\\Debug\\StreamlineCore.lib")
#pragma comment(lib, "..\\Libraries\\Libs\\ImGuiCore\\Debug\\ImGuiCore.lib")
#else
#pragma comment(lib, "..\\Libraries\\Libs\\EngineCore\\Release\\EngineCore.lib")
// #pragma comment(lib, "..\\Libraries\\Libs\\StreamlineCore\\Release\\StreamlineCore.lib")
#pragma comment(lib, "..\\Libraries\\Libs\\ImGuiCore\\Release\\ImGuiCore.lib")
#endif

#include "../EngineCore/EngineCorePch.h"
// #include "../StreamlineCore/StreamlineCorePch.h"
#include "../ImGuiCore/ImGuiCorePch.h"
// #include "log.h" // donut/core/