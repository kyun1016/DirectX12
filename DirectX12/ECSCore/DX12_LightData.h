#ifdef __cplusplus
#pragma once
#include "DX12_Config.h"
#endif

#ifndef LIGHT_DATA_H
#define LIGHT_DATA_H

#define MAX_LIGHTS 16

struct Light
{
    float3 Strength;
    float FalloffStart; // point/spot light only
    float3 Direction; // directional/spot light only
    float FalloffEnd; // point/spot light only
    float3 Position; // point light only
    float SpotPower; // spot light only

    uint type;
    float radius;
    float haloRadius;
    float haloStrength;

    float4x4 viewProj;
    float4x4 invProj;
    float4x4 shadowTransform;
};

#endif // LIGHT_DATA_H