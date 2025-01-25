#include "Common.hlsli"
struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float3 color : COLOR;
};

PixelOut main(PixelShaderInput input) : SV_TARGET
{
    PixelOut ret;
    ret.color0 = float4(input.color, 1.0f);
    ret.color1 = float4(input.color, 1.0f);
    return ret;
}