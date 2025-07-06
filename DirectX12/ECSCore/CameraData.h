#ifdef __cplusplus
#pragma once
#include "ECSConfig.h"
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
    float    dummy1;
};
#endif // CAMERA_DATA_H