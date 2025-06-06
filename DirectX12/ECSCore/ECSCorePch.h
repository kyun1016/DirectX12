#pragma once

#ifdef _DEBUG
#pragma comment(lib, "..\\Libraries\\Libs\\EngineCore\\Debug\\EngineCore.lib")
#pragma comment(lib, "..\\Libraries\\Libs\\ImGuiCore\\Debug\\ImGuiCore.lib")
#pragma comment(lib, "..\\Libraries\\Libs\\FMODCore\\Debug\\FMODCore.lib")
#else
#pragma comment(lib, "..\\Libraries\\Libs\\EngineCore\\Release\\EngineCore.lib")
#pragma comment(lib, "..\\Libraries\\Libs\\ImGuiCore\\Release\\ImGuiCore.lib")
#pragma comment(lib, "..\\Libraries\\Libs\\FMODCore\\Release\\FMODCore.lib")
#endif

#include "../FMODCore/FMODCorePch.h"

#include <filesystem>
#include "TimeSystem.h"
#include "PhysicsSystem.h"
#include "FMODAudioSystem.h"
#include "RenderSystem.h"
#include "MeshSystem.h"
#include "LightSystem.h"
