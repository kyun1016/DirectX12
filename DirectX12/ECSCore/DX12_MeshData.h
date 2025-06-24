#ifdef __cplusplus
#pragma once
#include "DX12_Config.h"
#endif

#ifndef MESH_DATA_H
#define MESH_DATA_H

struct Vertex
{
    float3 Position;
    float3 Normal;
    float2 TexC;
    float3 TangentU;
};

struct SkinnedVertex
{
    float3 Position;
    float3 Normal;
    float2 TexC;
    float3 TangentU;
    float3 BoneWeights;
    uint4 BoneIndices;
};

#ifdef __cplusplus
struct SpriteVertex
{
	float3 Position;
	float2 Size;
};
#else
struct SpriteVertex
{
    float3 Position : POSITION;
    float2 Size : SIZE;
};
#endif


#endif // MESH_DATA_H