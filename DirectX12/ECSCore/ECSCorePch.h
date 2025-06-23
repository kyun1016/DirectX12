#pragma once
#define NOMINMAX 1
#ifdef _DEBUG
#pragma comment(lib, "..\\Libraries\\Libs\\EngineCore\\Debug\\EngineCore.lib")
#pragma comment(lib, "..\\Libraries\\Libs\\ImGuiCore\\Debug\\ImGuiCore.lib")
#pragma comment(lib, "..\\Libraries\\Libs\\FMODCore\\Debug\\FMODCore.lib")
#else
#pragma comment(lib, "..\\Libraries\\Libs\\EngineCore\\Release\\EngineCore.lib")
#pragma comment(lib, "..\\Libraries\\Libs\\ImGuiCore\\Release\\ImGuiCore.lib")
#pragma comment(lib, "..\\Libraries\\Libs\\FMODCore\\Release\\FMODCore.lib")
#endif

#include "../ImGuiCore/ImGuiCorePch.h"
#include "../FMODCore/FMODCorePch.h"

#include "DX12_MeshRepository.h"

#include <filesystem>
#include "TimeSystem.h"
#include "WindowSystem.h"
#include "PhysicsSystem.h"
#include "FMODAudioSystem.h"
#include "MeshSystem.h"
#include "LightSystem.h"

#include "DX12_TransformSystem.h"
#include "DX12_BoundingSystem.h"
#include "DX12_InstanceSystem.h"
#include "DX12_RenderSystem.h"

#include "GameObjectFactory.h"
// #include "DX12_Core.h"