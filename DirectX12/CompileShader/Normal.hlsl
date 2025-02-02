#include "Memory.hlsli"

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

PixelOut PS(PixelShaderInput input)
{
    PixelOut ret;
    ret.color0 = float4(input.color, 1.0f);
    ret.color1 = float4(input.color, 1.0f);
    return ret;
}