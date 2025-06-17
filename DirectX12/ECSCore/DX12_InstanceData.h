#ifdef __cplusplus
#pragma once
#include "DX12_Config.h"
#endif

#ifndef INSTANCE_DATA_H
#define INSTANCE_DATA_H

struct InstanceData
{
    float4x4 World;
    float4x4 TexTransform;
    // Geometery Shader 법선 벡터 직교 성질 유지를 위함
    float4x4 WorldInvTranspose;

    float2 DisplacementMapTexelSize;
    float GridSpatialStep;
    int useDisplacementMap;

    int DisplacementIndex;
    int MaterialIndex;
    int dummy1;
    int dummy2;
};

#endif // INSTANCE_DATA_H