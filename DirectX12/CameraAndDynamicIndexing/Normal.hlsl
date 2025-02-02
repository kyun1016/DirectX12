
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

Texture2D gDiffuseMap : register(t0);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gTexTransform;
    float4x4 gWorldInvTranspose; // Geometery Shader 동작 간 법선 벡터 변환 시 직교 성질 유지를 위함
};

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

cbuffer cbMaterial : register(b2)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
    float4x4 gMatTransform;
};
static const float lineScale = 0.1;

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct NormalGeometryShaderInput
{
    float4 PosL : SV_POSITION;
    float3 NormalL : NORMAL;
};

NormalGeometryShaderInput VS(VertexIn input)
{
    NormalGeometryShaderInput output;

    output.PosL = float4(input.PosL, 1.0);
    output.NormalL = input.NormalL;

    return output;
}

struct NormalPixelShaderInput
{
    float4 pos : SV_POSITION;
    float3 color : COLOR;
};

[maxvertexcount(2)]
void GS(point NormalGeometryShaderInput input[1], inout LineStream<NormalPixelShaderInput> outputStream)
{
    NormalPixelShaderInput output;
    
    float4 posW = mul(input[0].PosL, gWorld);
    float4 normalL = float4(input[0].NormalL, 0.0);
    float4 normalW = mul(normalL, gWorldInvTranspose);
    normalW = float4(normalize(normalW.xyz), 0.0);
    
    output.pos = mul(posW, gViewProj);
    output.color = float3(1.0, 1.0, 0.0);
    outputStream.Append(output);
    
    output.pos = mul(posW + lineScale * normalW, gViewProj);
    output.color = float3(1.0, 0.0, 0.0);
    outputStream.Append(output);
}

struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float3 color : COLOR;
};

struct PixelOut
{
    float4 color0 : SV_Target0;
    float4 color1 : SV_Target1;
};

PixelOut PS(PixelShaderInput input) : SV_TARGET
{
    PixelOut ret;
    ret.color0 = float4(input.color, 1.0f);
    ret.color1 = float4(input.color, 1.0f);
    return ret;
}