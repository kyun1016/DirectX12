#pragma once

#include <wrl/client.h> // ComPtr
#include <dxgi1_4.h>
#include <string>
#include <memory>

struct DeviceParameter
{
    uint32_t backBufferWidth = 1280;
    uint32_t backBufferHeight = 720;
	float aspectRatio;

    DXGI_FORMAT swapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    UINT rtvDescriptorSize = 0;
    UINT dsvDescriptorSize = 0;
    UINT cbvSrvUavDescriptorSize = 0;

    bool vsyncEnabled = false;

    bool enableDebugRuntime = false;
    bool enableGPUValidation = false; // Affects only DX12
    bool headlessDevice = false;

    bool dlssgEnable = false;
    bool latewarpEnable = false;
};

struct UIData
{
//    // General
//    nvrhi::GraphicsAPI                  GraphicsAPI = nvrhi::GraphicsAPI::D3D12;
//    bool                                EnableAnimations = true;
//    float                               AnimationSpeed = 1.;
//    bool                                EnableVsync = false;
//    bool                                VisualiseBuffers = false;
//    float                               CpuLoad = 0;
//    int                                 GpuLoad = 0;
//    donut::math::int2                   Resolution = { 0,0 };
//    bool                                Resolution_changed = false;
//    bool                                MouseOverUI = false;
//
//    uint32_t getNViewports() const { return (uint32_t)BackBufferExtents.size(); }
//    sl::Extent getExtent(uint32_t fullWidth, uint32_t fullHeight, uint32_t uV);
//private:
//    friend class UIRenderer;
//    std::vector<sl::Extent>             BackBufferExtents{};
//public:
//
//    // SSAO
//    bool                                EnableSsao = true;
//    donut::render::SsaoParameters       SsaoParams;
//
//    // Tonemapping
//    bool                                 EnableToneMapping = true;
//    donut::render::ToneMappingParameters ToneMappingParams;
//
//    // Sky
//    bool                                EnableProceduralSky = true;
//    donut::render::SkyParameters        SkyParams;
//    float                               AmbientIntensity = .2f;
//
//    // Antialising (+TAA)
//    AntiAliasingMode                                   AAMode = AntiAliasingMode::NONE;
//    donut::render::TemporalAntiAliasingJitter          TemporalAntiAliasingJitter = donut::render::TemporalAntiAliasingJitter::MSAA;
//    donut::render::TemporalAntiAliasingParameters      TemporalAntiAliasingParams;
//
//    // Bloom
//    bool                                EnableBloom = true;
//    float                               BloomSigma = 32.f;
//    float                               BloomAlpha = 0.05f;
//
//    // Shadows
//    bool                                EnableShadows = true;
//    float                               CsmExponent = 4.f;
//
//    // DLSS specific parameters
//    float                               DLSS_Sharpness = 0.f;
//    bool                                DLSS_Supported = false;
//    sl::DLSSMode                        DLSS_Mode = sl::DLSSMode::eOff;
//    RenderingResolutionMode             DLSS_Resolution_Mode = RenderingResolutionMode::FIXED;
//    bool                                DLSS_Dynamic_Res_change = true;
//    AntiAliasingMode                    DLSS_Last_AA = AntiAliasingMode::NONE;
//    bool                                DLSS_DebugShowFullRenderingBuffer = false;
//    bool                                DLSS_lodbias_useoveride = false;
//    float                               DLSS_lodbias_overide = 0.f;
//    bool                                DLSS_always_use_extents = false;
//    sl::DLSSPreset                      DLSS_presets[static_cast<int>(sl::DLSSMode::eCount)] = {};
//    sl::DLSSPreset                      DLSS_last_presets[static_cast<int>(sl::DLSSMode::eCount)] = {};
//    bool UIData::DLSSPresetsChanged()
//    {
//        for (int i = 0; i < static_cast<int>(sl::DLSSMode::eCount); i++)
//        {
//            if (DLSS_presets[i] != DLSS_last_presets[i])
//                return true;
//        }
//        return false;
//    };
//    bool UIData::DLSSPresetsAnyNonDefault()
//    {
//        for (int i = 0; i < static_cast<int>(sl::DLSSMode::eCount); i++)
//        {
//            if (DLSS_presets[i] != sl::DLSSPreset::eDefault)
//                return true;
//        }
//        return false;
//    };
//    void UIData::DLSSPresetsUpdate()
//    {
//        for (int i = 0; i < static_cast<int>(sl::DLSSMode::eCount); i++)
//            DLSS_last_presets[i] = DLSS_presets[i];
//    };
//    void UIData::DLSSPresetsReset()
//    {
//        for (int i = 0; i < static_cast<int>(sl::DLSSMode::eCount); i++)
//            DLSS_last_presets[i] = DLSS_presets[i] = sl::DLSSPreset::eDefault;
//    };
//
//    // NIS specific parameters
//    bool                                NIS_Supported = false;
//    sl::NISMode                         NIS_Mode = sl::NISMode::eOff;
//    float                               NIS_Sharpness = 0.5f;
//
//    // DeepDVC specific parameters
//    bool                                DeepDVC_Supported = false;
//    sl::DeepDVCMode                     DeepDVC_Mode = sl::DeepDVCMode::eOff;
//    float                               DeepDVC_Intensity = 0.5f;
//    float                               DeepDVC_SaturationBoost = 0.75f;
//    uint64_t                            DeepDVC_VRAM = 0;
//
//    // LATENCY specific parameters
//    bool                                REFLEX_Supported = false;
//    bool                                REFLEX_LowLatencyAvailable = false;
//    int                                 REFLEX_Mode = static_cast<int>(sl::ReflexMode::eOff);
//    int                                 REFLEX_CapedFPS = 0;
//    std::string                         REFLEX_Stats = "";
//
//    // DLFG specific parameters
//    bool                                DLSSG_Supported = false;
//    sl::DLSSGMode                       DLSSG_mode = sl::DLSSGMode::eOff;
//    int                                 DLSSG_numFrames = 2;
//    int                                 DLSSG_numFramesMaxMultiplier = 4;
//    float                               DLSSG_fps = 0;
//    size_t                              DLSSG_memory = 0;
//    std::string                         DLSSG_status = "";
//    bool                                DLSSG_cleanup_needed = false;
//
//    // Latewarp
//    bool                                Latewarp_Supported = false;
//    int                                 Latewarp_active = 0;

};