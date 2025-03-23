#pragma once

#ifdef _DEBUG
#pragma comment(lib, "..\\Libraries\\Libs\\EngineCore\\Debug\\EngineCore.lib")
#pragma comment(lib, "..\\Libraries\\Libs\\ImGuiCore\\Debug\\ImGuiCore.lib")
#else
#pragma comment(lib, "..\\Libraries\\Libs\\EngineCore\\Release\\EngineCore.lib")
#pragma comment(lib, "..\\Libraries\\Libs\\ImGuiCore\\Release\\ImGuiCore.lib")
#endif

#include "../EngineCore/EngineCorePch.h"
#include "../ImGuiCore/ImGuiCorePch.h"