#pragma once
#define DONUT_WITH_DX11 1
#define DONUT_WITH_DX12 1
#define DONUT_WITH_VULKAN 1
#define NOMINMAX
#define _MBCS
#define WIN32
#define _WINDOWS

// #include "../DonutCore/DonutCorePch.h"
#include "../DonutCore/Math.h"
#include "../DonutCore/VFS.h"
#include "../DonutCore/log.h"
#include "../DonutCore/string_utils.h"
#include "../DonutCore/TarFile.h"
#include "../DonutCore/Compression.h"

// #include "../DonutEngine/DonutEnginePch.h"
#include "../DonutEngine/View.h"
#include "../DonutEngine/CommonRenderPasses.h"
#include "../DonutEngine/ConsoleInterpreter.h"
#include "../DonutEngine/ConsoleObjects.h"
#include "../DonutEngine/Scene.h"
#include "../DonutEngine/SceneGraph.h"
#include "../DonutEngine/ShaderFactory.h"
#include "../DonutEngine/TextureCache.h"

#include "../DonutApp/UserInterfaceUtils.h"
#include "../DonutApp/AftermathCrashDump.h"
#include "../DonutApp/ApplicationBase.h"
#include "../DonutApp/Camera.h"
#include "../DonutApp/DeviceManager.h"
#include "../DonutApp/DeviceManager_DX11.h"
#include "../DonutApp/DeviceManager_DX12.h"
#include "../DonutApp/DeviceManager_VK.h"
#include "../DonutApp/DonutAppPch.h"
#include "../DonutApp/DonutEnginePch.h"
#include "../DonutApp/imgui_console.h"
#include "../DonutApp/imgui_nvrhi.h"
#include "../DonutApp/imgui_renderer.h"
#include "../DonutApp/MediaFileSystem.h"
#include "../DonutApp/pch.h"
#include "../DonutApp/Timer.h"

#include "../nvrhi/nvrhi.h"
#include "../nvrhi/utils.h"
#include "../nvrhi/d3d11.h"
#include "../nvrhi/d3d12.h"
#include "../nvrhi/vulkan.h"
#include "../nvrhi/validation.h"

#include "../ImGuiCore/imgui.h"

#include "../NVaftermath/GFSDK_Aftermath_GpuCrashDump.h"
#include "../NVaftermath/GFSDK_Aftermath_GpuCrashDumpDecoding.h"