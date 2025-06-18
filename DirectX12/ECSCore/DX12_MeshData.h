#ifdef __cplusplus
#pragma once
#include "DX12_Config.h"
#endif

#ifndef MESH_DATA_H
#define MESH_DATA_H

struct VertexInput
{
    float3 Position;
    float3 Normal;
    float2 TexC;
    float3 TangentU;
};

struct SkinnedVertexInput
{
    float3 Position;
    float3 Normal;
    float2 TexC;
    float3 TangentU;
    float3 BoneWeights;
    BYTE BoneIndices[4];
};

#endif // MESH_DATA_H