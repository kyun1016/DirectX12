#pragma once

#include <sl.h>
#include <sl_consts.h>
#include <sl_hooks.h>
#include <sl_version.h>

// Streamline Features
#include <sl_dlss.h>
#include <sl_reflex.h>
#include <sl_nis.h>
#include <sl_dlss_g.h>
#include <sl_deepdvc.h>

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
    // General
    // nvrhi::GraphicsAPI                  GraphicsAPI = nvrhi::GraphicsAPI::D3D12;
    bool                                EnableAnimations = true;
    float                               AnimationSpeed = 1.;
    bool                                EnableVsync = false;
    bool                                VisualiseBuffers = false;
    float                               CpuLoad = 0;
    int                                 GpuLoad = 0;
    donut::math::int2                   Resolution = { 0,0 };
    bool                                Resolution_changed = false;
    bool                                MouseOverUI = false;

    uint32_t getNViewports() const { return (uint32_t)BackBufferExtents.size(); }
    sl::Extent getExtent(uint32_t fullWidth, uint32_t fullHeight, uint32_t uV)
    {
        static const uint32_t B = 10; // boundary
        sl::Extent e{}; // extent
        switch (BackBufferExtents.size())
        {
        case 3:
            switch (uV)
            {
            case 0:
            case 1:
                if (fullWidth / 2 > 3 * B / 2 && fullHeight / 2 > 3 * B / 2)
                {
                    e.left = (uV == 0) ? B : fullWidth / 2 + B / 2;
                    e.top = B;
                    e.width = fullWidth / 2 - 3 * B / 2;
                    e.height = fullHeight / 2 - 3 * B / 2;
                }
                return e;
            case 2:
                if (fullHeight / 2 > B / 2)
                {
                    e.left = B;
                    e.top = fullHeight / 2 + B / 2;
                    e.width = fullWidth - B - e.left;
                    e.height = fullHeight - B - e.top;
                }
                return e;
            }
            return e;
        case 2:
            e.left = uV * fullWidth / 2;
            e.top = uV * fullHeight / 2;
            e.width = (uV + 1) * fullWidth / 2 - e.left;
            e.height = (uV + 1) * fullHeight / 2 - e.top;
            return e;
        default:
            return BackBufferExtents[0];
        }
        return e;
    }
private:
    friend class UIRenderer;
    std::vector<sl::Extent>             BackBufferExtents{};
public:

    // SSAO
    bool                                EnableSsao = true;
    struct SsaoParameters
    {
        float amount = 2.f;
        float backgroundViewDepth = 100.f;
        float radiusWorld = 0.5f;
        float surfaceBias = 0.1f;
        float powerExponent = 2.f;
        bool enableBlur = true;
        float blurSharpness = 16.f;
    } SsaoParams;

    // Tonemapping
    bool                                 EnableToneMapping = true;
    struct ToneMappingParameters
    {
        float histogramLowPercentile = 0.8f;
        float histogramHighPercentile = 0.95f;
        float eyeAdaptationSpeedUp = 1.f;
        float eyeAdaptationSpeedDown = 0.5f;
        float minAdaptedLuminance = 0.02f;
        float maxAdaptedLuminance = 0.5f;
        float exposureBias = -0.5f;
        float whitePoint = 3.f;
        bool enableColorLUT = true;
    } ToneMappingParams;

    // Sky
    bool                                EnableProceduralSky = true;
    struct SkyParameters
    {
        dm::float3 skyColor{ 0.17f, 0.37f, 0.65f };
        dm::float3 horizonColor{ 0.50f, 0.70f, 0.92f };
        dm::float3 groundColor{ 0.62f, 0.59f, 0.55f };
        dm::float3 directionUp{ 0.f, 1.f, 0.f };
        float brightness = 0.1f; // scaler for sky brightness
        float horizonSize = 30.f; // +/- degrees
        float glowSize = 5.f; // degrees, starting from the edge of the light disk
        float glowIntensity = 0.1f; // [0-1] relative to light intensity
        float glowSharpness = 4.f; // [1-10] is the glow power exponent
        float maxLightRadiance = 100.f; // clamp for light radiance derived from its angular size, 0 = no clamp
    } SkyParams;
    float                               AmbientIntensity = .2f;

    // Antialising (+TAA)
    enum class AntiAliasingMode {
        NONE,
        TEMPORAL,
        DLSS,
    } AAMode = AntiAliasingMode::NONE;
    enum class TemporalAntiAliasingJitter
    {
        MSAA,
        Halton,
        R2,
        WhiteNoise
    } TemporalAntiAliasingJitter = TemporalAntiAliasingJitter::MSAA;
    struct TemporalAntiAliasingParameters
    {
        float newFrameWeight = 0.1f;
        float clampingFactor = 1.0f;
        float maxRadiance = 10000.f;
        bool enableHistoryClamping = true;

        // Requires CreateParameters::historyClampRelax single channel [0, 1] mask to be provided. 
        // For texels with mask value of 0 the behavior is unchanged; for texels with mask value > 0, 
        // 'newFrameWeight' will be reduced and 'clampingFactor' will be increased proportionally. 
        bool useHistoryClampRelax = false;
    } TemporalAntiAliasingParams;

    // Bloom
    bool                                EnableBloom = true;
    float                               BloomSigma = 32.f;
    float                               BloomAlpha = 0.05f;

    // Shadows
    bool                                EnableShadows = true;
    float                               CsmExponent = 4.f;

    // DLSS specific parameters
    float                               DLSS_Sharpness = 0.f;
    bool                                DLSS_Supported = false;
    sl::DLSSMode                        DLSS_Mode = sl::DLSSMode::eOff;

    enum class RenderingResolutionMode {
        FIXED,
        DYNAMIC,
        COUNT
    } DLSS_Resolution_Mode = RenderingResolutionMode::FIXED;
    bool                                DLSS_Dynamic_Res_change = true;
    AntiAliasingMode                    DLSS_Last_AA = AntiAliasingMode::NONE;
    bool                                DLSS_DebugShowFullRenderingBuffer = false;
    bool                                DLSS_lodbias_useoveride = false;
    float                               DLSS_lodbias_overide = 0.f;
    bool                                DLSS_always_use_extents = false;
    sl::DLSSPreset                      DLSS_presets[static_cast<int>(sl::DLSSMode::eCount)] = {};
    sl::DLSSPreset                      DLSS_last_presets[static_cast<int>(sl::DLSSMode::eCount)] = {};
    bool DLSSPresetsChanged()
    {
        for (int i = 0; i < static_cast<int>(sl::DLSSMode::eCount); i++)
        {
            if (DLSS_presets[i] != DLSS_last_presets[i])
                return true;
        }
        return false;
    };
    bool DLSSPresetsAnyNonDefault()
    {
        for (int i = 0; i < static_cast<int>(sl::DLSSMode::eCount); i++)
        {
            if (DLSS_presets[i] != sl::DLSSPreset::eDefault)
                return true;
        }
        return false;
    };
    void DLSSPresetsUpdate()
    {
        for (int i = 0; i < static_cast<int>(sl::DLSSMode::eCount); i++)
            DLSS_last_presets[i] = DLSS_presets[i];
    };
    void DLSSPresetsReset()
    {
        for (int i = 0; i < static_cast<int>(sl::DLSSMode::eCount); i++)
            DLSS_last_presets[i] = DLSS_presets[i] = sl::DLSSPreset::eDefault;
    };

    // NIS specific parameters
    bool                                NIS_Supported = false;
    sl::NISMode                         NIS_Mode = sl::NISMode::eOff;
    float                               NIS_Sharpness = 0.5f;

    // DeepDVC specific parameters
    bool                                DeepDVC_Supported = false;
    sl::DeepDVCMode                     DeepDVC_Mode = sl::DeepDVCMode::eOff;
    float                               DeepDVC_Intensity = 0.5f;
    float                               DeepDVC_SaturationBoost = 0.75f;
    uint64_t                            DeepDVC_VRAM = 0;

    // LATENCY specific parameters
    bool                                REFLEX_Supported = false;
    bool                                REFLEX_LowLatencyAvailable = false;
    int                                 REFLEX_Mode = static_cast<int>(sl::ReflexMode::eOff);
    int                                 REFLEX_CapedFPS = 0;
    std::string                         REFLEX_Stats = "";

    // DLFG specific parameters
    bool                                DLSSG_Supported = false;
    sl::DLSSGMode                       DLSSG_mode = sl::DLSSGMode::eOff;
    int                                 DLSSG_numFrames = 2;
    int                                 DLSSG_numFramesMaxMultiplier = 4;
    float                               DLSSG_fps = 0;
    size_t                              DLSSG_memory = 0;
    std::string                         DLSSG_status = "";
    bool                                DLSSG_cleanup_needed = false;

    // Latewarp
    bool                                Latewarp_Supported = false;
    int                                 Latewarp_active = 0;

};