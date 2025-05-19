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

// #define _CRT_SECURE_NO_WARNINGS
// #include "../DonutCore/DonutCorePch.h"
// #include "../DonutEngine/DonutEnginePch.h"
// #include "../DonutRender/DonutRenderPch.h"
// #include "../DonutApp/DonutAppPch.h"

#include "../DonutCore/VFS.h"
#include "../DonutCore/log.h"
#include "../DonutCore/string_utils.h"
#include "../DonutEngine/CommonRenderPasses.h"
#include "../DonutEngine/ConsoleInterpreter.h"
#include "../DonutEngine/ConsoleObjects.h"
#include "../DonutEngine/FramebufferFactory.h"
#include "../DonutEngine/Scene.h"
#include "../DonutEngine/ShaderFactory.h"
#include "../DonutEngine/TextureCache.h"
#include "../DonutRender/BloomPass.h"
#include "../DonutRender/CascadedShadowMap.h"
#include "../DonutRender/DeferredLightingPass.h"
#include "../DonutRender/DepthPass.h"
#include "../DonutRender/DrawStrategy.h"
#include "../DonutRender/ForwardShadingPass.h"
#include "../DonutRender/GBuffer.h"
#include "../DonutRender/GBufferFillPass.h"
#include "../DonutRender/LightProbeProcessingPass.h"
#include "../DonutRender/PixelReadbackPass.h"
#include "../DonutRender/SkyPass.h"
#include "../DonutRender/SsaoPass.h"
#include "../DonutRender/TemporalAntiAliasingPass.h"
#include "../DonutRender/ToneMappingPasses.h"
#include "../DonutApp/ApplicationBase.h"
#include "../DonutApp/UserInterfaceUtils.h"
#include "../DonutApp/Camera.h"
#include "../DonutApp/imgui_console.h"
#include "../DonutApp/imgui_renderer.h"
#include "../nvrhi/utils.h"
#include "../nvrhi/common/misc.h"

// #include "log.h" // donut/core/