
// Defaults for number of lights.
#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

#define MaxLights 16
#include "LightingUtil.hlsli"

struct MaterialData
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Roughness;
    float4x4 MatTransform;
    uint DiffuseMapIndex;
    uint MatPad0;
    uint MatPad1;
    uint MatPad2;
};

#ifdef TEX_SIZE
Texture2D gDiffuseMap[TEX_SIZE] : register(t0, space0);
#else
Texture2D gDiffuseMap[16] : register(t0, space0);
#endif
// Put in space1, so the texture array does not overlap with these resources.  
// The texture array will occupy registers t0, t1, ..., t3 in space0. 
StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);
Texture2D gDisplacementMap : register(t1, space1);
#ifdef TEX_ARRAY_SIZE
Texture2DArray gTreeMapArray[TEX_ARRAY_SIZE] : register(t2, space1);
#else
Texture2DArray gTreeMapArray[2] : register(t2, space1);
#endif

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

// Constant data that varies per frame.

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gTexTransform;
    float4x4 gWorldInvTranspose; // Geometery Shader 동작 간 법선 벡터 변환 시 직교 성질 유지를 위함
    uint gMaterialIndex;
    float2 gDisplacementMapTexelSize;
    float gGridSpatialStep;
    float cbPerObjectPad1;
};

//cbuffer cbPerObject : register(b0)
//{
//    float4x4 gWorld;
//    float4x4 gTexTransform;
//    float4x4 gWorldInvTranspose; // Geometery Shader 동작 간 법선 벡터 변환 시 직교 성질 유지를 위함
//    uint gMaterialIndex;
//    uint gObjPad0;
//    uint gObjPad1;
//    uint gObjPad2;
//};


// Constant data that varies per material.
cbuffer cbPass : register(b1)
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
    
    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    Light gLights[MaxLights];
};