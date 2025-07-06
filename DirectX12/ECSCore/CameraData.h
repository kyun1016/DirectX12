#ifdef __cplusplus
#pragma once
#include "DX12_Config.h"
#endif

#ifndef CAMERA_DATA_H
#define CAMERA_DATA_H

struct CameraData
{
    float4x4 View;
    float4x4 InvView;
    float4x4 Proj;
    float4x4 InvProj;
    float4x4 ViewProj;
    float4x4 InvViewProj;
    float3   EyePosW;
    float FieldOfView = XM_PIDIV4; // 45도 시야각
};
#endif // CAMERA_DATA_H