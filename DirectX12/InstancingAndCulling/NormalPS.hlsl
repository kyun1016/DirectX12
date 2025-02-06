#include "Memory.hlsli"

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

PixelOut main(PixelShaderInput input)
{
    PixelOut ret;
    ret.color0 = float4(input.color, 1.0f);
    ret.color1 = float4(input.color, 1.0f);
    return ret;
}