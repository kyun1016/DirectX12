#include "Memory.hlsli"

static const float lineScale = 0.1;

struct NormalGeometryShaderInput
{
    float4 PosL : SV_POSITION;
    float3 NormalL : NORMAL;
};

struct NormalPixelShaderInput
{
    float4 pos : SV_POSITION;
    float3 color : COLOR;
};

[maxvertexcount(2)]
void main(point NormalGeometryShaderInput input[1], inout LineStream<NormalPixelShaderInput> outputStream)
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