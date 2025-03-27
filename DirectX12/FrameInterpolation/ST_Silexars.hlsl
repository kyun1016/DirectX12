// https://anteru.net/blog/2016/mapping-between-HLSL-and-GLSL/
// https://www.shadertoy.com/view/XsXXDn
#include "memory.hlsli"


struct SamplingVertexShaderInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
    float3 TangentU : TANGENT;
};

struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

PixelShaderInput VS(SamplingVertexShaderInput input)
{
    PixelShaderInput output;
    
    output.position = float4(input.position, 1.0);
    output.texcoord = input.texcoord;

    return output;
}


float4 PS(PixelShaderInput input) : SV_TARGET
{
    float3 c;
    float l, z = iTime;
    for (int i = 0; i < 3; i++)
    {
        float2 uv = input.texcoord;
        float2 p = input.texcoord;
        p -= .5;
        p.x = iResolution.x / iResolution.y;
        z += .07;
        l = length(p);
        uv += p / l * (sin(z) + 1.) * abs(sin(l * 9. - z - z));
        c[i] = .01 / length(fmod(uv, 1.) - .5);
    }
    return float4(c / l, iTime);
}