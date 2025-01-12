#pragma once

#ifdef _DEBUG
#pragma comment(lib, "EngineCore\\Debug\\EngineCore.lib")
#pragma comment(lib, "ImGuiCore\\Debug\\ImGuiCore.lib")
#else
#pragma comment(lib, "EngineCore\\Release\\EngineCore.lib")
#pragma comment(lib, "ImGuiCore\\Release\\ImGuiCore.lib")
#endif

#include "../EngineCore/EngineCorePch.h"
#include "../ImGuiCore/ImGuiCorePch.h"