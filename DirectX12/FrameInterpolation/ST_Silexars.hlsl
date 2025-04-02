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

float mod(float x, float y)
{
    return x - y * floor(x / y);
}

float2 mod(float2 x, float y)
{
    float2 ret = x;
    return x - y * floor(x / y);
}


float4 PS(PixelShaderInput input) : SV_TARGET
{
    float3 c;
    float l, z = iTime;
    [unroll]
    for (int i = 0; i < 3; i++)
    {
        float2 uv = input.texcoord;
        uv.y = 1.0 - uv.y;
        // uv = uv * 2.0 - 1.0;
        float2 p = uv;
        p -= .5;
        // p.x *= (iResolution.x / iResolution.y);
        z += .07;
        l = length(p) * 0.8;
        uv += (p / l) * (sin(z) + 1.) * abs(sin(l * 9. - z - z));
        c[i] = .01 / length(mod(uv, 1.) - 0.5);
    }
    return float4(c/l, iTime);
}