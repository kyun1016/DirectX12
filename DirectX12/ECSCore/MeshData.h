#ifdef __cplusplus
#pragma once
#include "ECSConfig.h"
#endif

#ifndef MESH_DATA_H
#define MESH_DATA_H

#ifdef __cplusplus
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
struct SpriteVertex
{
	float3 Position;
	float2 Size;
};
#else
// HLSL structure for sprite vertex
struct Vertex
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexC : TEXCOORD;
    float3 TangentU : TANGENT;
};
struct SkinnedVertex
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexC : TEXCOORD;
    float3 TangentU : TANGENT;
    float3 BoneWeights : WEIGHTS;
    uint4 BoneIndices : BONEINDICES;
};
struct SpriteVertex
{
    float3 Position : POSITION;
    float2 Size : SIZE;
};
#endif // __cplusplus
#endif // MESH_DATA_H