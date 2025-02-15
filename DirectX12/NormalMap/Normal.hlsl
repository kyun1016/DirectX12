#include "Memory.hlsli"

static const float lineScale = 0.1;

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
    float3 TangentU : TANGENT;
};

struct NormalGeometryShaderInput
{
    float4 PosL : SV_POSITION;
    float3 NormalL : NORMAL;
    
    // nointerpolation is used so the index is not interpolated 
	// across the triangle.
    nointerpolation uint InsIndex : SV_InstanceID;
};

NormalGeometryShaderInput VS(VertexIn input, uint instanceID : SV_InstanceID)
{
    NormalGeometryShaderInput output;

    output.PosL = float4(input.PosL, 1.0);
    output.NormalL = input.NormalL;
    output.InsIndex = instanceID + gBaseInstanceIndex;
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
    
    InstanceData instData = gInstanceData[input[0].InsIndex];
    float4x4 world = instData.World;
    float4x4 worldInvTranspose = instData.WorldInvTranspose;
    
    float4 posW = mul(input[0].PosL, world);
    float4 normalL = float4(input[0].NormalL, 0.0);
    float4 normalW = mul(normalL, worldInvTranspose);
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

PixelOut PS(PixelShaderInput input)
{
    PixelOut ret;
    ret.color0 = float4(input.color, 1.0f);
    ret.color1 = float4(input.color, 1.0f);
    return ret;
}