#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>

#define _ST

// 해당 파일은 streamline compile 결과
#pragma comment(lib, "..\\StreamlineCore\\streamline\\lib\\x64\\sl.interposer.lib")

#ifdef _DEBUG
#pragma comment(lib, "..\\Libraries\\Libs\\EngineCore\\Debug\\EngineCore.lib")
#pragma comment(lib, "..\\Libraries\\Libs\\ImGuiCore\\Debug\\ImGuiCore.lib")
#pragma comment(lib, "..\\Libraries\\Libs\\FMODCore\\Debug\\FMODCore.lib")
#else
#pragma comment(lib, "..\\Libraries\\Libs\\EngineCore\\Release\\EngineCore.lib")
#pragma comment(lib, "..\\Libraries\\Libs\\ImGuiCore\\Release\\ImGuiCore.lib")
#pragma comment(lib, "..\\Libraries\\Libs\\FMODCore\\Release\\FMODCore.lib")
#endif

#include "../EngineCore/EngineCorePch.h"
#include "../ImGuiCore/ImGuiCorePch.h"
#include "../FMODCore/FMODCorePch.h"