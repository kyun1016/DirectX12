#ifdef __cplusplus
#pragma once
#include "ECSConfig.h"
#endif

#ifndef PASS_DATA_H
#define PASS_DATA_H
#include "LightData.h"
#include "CameraData.h"

struct PassData
{
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    // Allow application to change fog parameters once per frame.
// For example, we may only use fog for certain times of day.
    float4 gFogColor;
    float gFogStart;
    float gFogRange;
    float2 cbPerObjectPad2;

    uint gCubeMapIndex;
};
#endif // PASS_DATA_H