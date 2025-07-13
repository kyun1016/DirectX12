#ifdef __cplusplus
#pragma once
#include "ECSConfig.h"
#endif

#ifndef INSTANCE_DATA_H
#define INSTANCE_DATA_H

struct InstanceData
{
    float4x4 World;
    float4x4 TexTransform;
    float4x4 WorldInvTranspose;

    float2 DisplacementMapTexelSize;
    float GridSpatialStep;
    uint useDisplacementMap;

    uint DisplacementIndex;
    uint MaterialIndex;
    uint CameraIndex;
    uint LightIndex;

    // TODO: 그 외 다양한 속성을 Structured Buffer에서 관리하는 경우, Index 성분 추가
};
struct InstanceIDData
{
    uint BaseInstanceIndex;
};

#endif // INSTANCE_DATA_H