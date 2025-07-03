#ifdef __cplusplus
#pragma once
#include "DX12_Config.h"
#endif

#ifndef PASS_DATA_H
#define PASS_DATA_H
#include "DX12_LightData.h"

 struct PassData
 {
     float4x4 gView;
     float4x4 gInvView;
     float4x4 gProj;
     float4x4 gInvProj;
     float4x4 gViewProj;
     float4x4 gInvViewProj;
     float3 gEyePosW;
     float cbPerPassPad1;
     float2 gRenderTargetSize;
     float2 gInvRenderTargetSize;
     float gNearZ;
     float gFarZ;
     float gTotalTime;
     float gDeltaTime;
     float4 gAmbientLight;

     // Allow application to change fog parameters once per frame.
 	// For example, we may only use fog for certain times of day.
     float4 gFogColor;
     float gFogStart;
     float gFogRange;
     float2 cbPerObjectPad2;

     Light gLights[MAX_LIGHTS];

     uint gCubeMapIndex;
 };
#endif // PASS_DATA_H