// https://anteru.net/blog/2016/mapping-between-HLSL-and-GLSL/
// https://www.shadertoy.com/view/3csSWB
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
    float2 F = input.texcoord;
       //Iterator and attenuation (distance-squared)
    float i = .2;
    //Resolution for scaling and centering
    float2 r = iResolution.xy;
    //Centered ratio-corrected coordinates
    float2 p = p = (F + F - r) / r.y / .7;    
    //Diagonal vector for skewing
    float2 d = float2(-1, 1);
    //Blackhole center
    float2 b = p - i * d;
    //Rotate and apply perspective
    float2x2 M1 = float2x2(1, 1, d / (.1 + i / dot(b, b)));
    float2 c = mul(p, M1);
    //Rotate into spiraling coordinates
    float a = dot(c, c);
    float2x2 M2 = float2x2(cos(.5 * log(a) + iTime * i + float4(0, 33, 11, 0)));
    float2 v = mul(c, M2) / i;
    //Waves cumulative total for coloring
    float2 w;
    
    //Loop through waves
    for (; i++ < 9.; w += 1. + sin(v))
        //Distort coordinates
        v += .7 * sin(v.yx * i + iTime) / i + .5;
    //Acretion disk radius
    i = length(sin(v / .3) * .4 + c * (3. + d));
    //Red/blue gradient
    return 1. - exp(-exp(c.x * float4(.6, -.4, -1, 0))
                   //Wave coloring
                   / w.xyyx
                   //Acretion disk brightness
                   / (2. + i * i / 4. - i)
                   //Center darkness
                   / (.5 + 1. / a)
                   //Rim highlight
                   / (.03 + abs(length(p) - .7)));
}