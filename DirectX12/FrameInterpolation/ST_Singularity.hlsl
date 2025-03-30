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


// void mainImage(out float4 O, float2 F)
// {
//     //Iterator and attenuation (distance-squared)
//     float i = .2, a;
//     //Resolution for scaling and centering
//     float2 r = iResolution.xy,
//          //Centered ratio-corrected coordinates
//          p = (F + F - r) / r.y / .7,
//          //Diagonal vector for skewing
//          d = float2(-1, 1),
//          //Blackhole center
//          b = p - i * d,
//          //Rotate and apply perspective
//          c = p * mat2(1, 1, d / (.1 + i / dot(b, b))),
//          //Rotate into spiraling coordinates
//          v = c * mat2(cos(.5 * log(a = dot(c, c)) + iTime * i + float4(0, 33, 11, 0))) / i,
//          //Waves cumulative total for coloring
//          w;
//     
//     //Loop through waves
//     for (; i++ < 9.; w += 1. + sin(v))
//         //Distort coordinates
//         v += .7 * sin(v.yx * i + iTime) / i + .5;
//     //Acretion disk radius
//     i = length(sin(v / .3) * .4 + c * (3. + d));
//     //Red/blue gradient
//     O = 1. - exp(-exp(c.x * float4(.6, -.4, -1, 0))
//                    //Wave coloring
//                    / w.xyyx
//                    //Acretion disk brightness
//                    / (2. + i * i / 4. - i)
//                    //Center darkness
//                    / (.5 + 1. / a)
//                    //Rim highlight
//                    / (.03 + abs(length(p) - .7))
//              );
// }

float4 PS(PixelShaderInput input) : SV_TARGET
{
    float2 F = input.texcoord;
    F.y = 1.0 - F.y;
    float4 color = float4(1, 1, 1, 1);
    
    //Iterator and attenuation (distance-squared)
    float i = .2;
    
    //Resolution for scaling and centering
    float2 r = iResolution.xy / iResolution.y;
    //Centered ratio-corrected coordinates
    float2 p = (F + F - r * 0.7) * 1.5;
    
    //Diagonal vector for skewing
    float2 d = float2(-1, 1);
    
    //Blackhole center
    float2 b = p - i * d;
    
    //Rotate and apply perspective
    float2 tmp = d / (.1 + i / dot(b, b));
    float2x2 M1 = float2x2(1, 1, tmp.x, tmp.y);
    float2 c = mul(p, M1);
    
    //Rotate into spiraling coordinates
    float a = dot(c, c);
    
    float4 angle = .5 * log(a) + iTime * i + float4(0, 33, 11, 0);
    float4 vcos = cos(angle);
    float2x2 M2 = float2x2(vcos.x, vcos.y, vcos.z, vcos.w);
    float2 v = mul(c, M2) / i;
    
    //Waves cumulative total for coloring
    float2 w = float2(0, 0);
    
    //Loop through waves
    while (i++ < 9.0f)
    {
        w += float2(1, 1) + sin(v);
         //Distort coordinates
        v += .7 * sin(v.yx * i + iTime) / i + .5;
    }
    
    //Acretion disk radius
    float2 tmp1 = sin(v / .3) * .4 + c * (3. + d);
    i = length(tmp1);
    
    //Red/blue gradient
    color = 1. - exp(-exp(c.x * float4(.6, -.4, -1, 0))
                   //Wave coloring
                   / w.xyyx
                   //Acretion disk brightness
                   / (2. + i * i / 4. - i)
                   //Center darkness
                   / (.5 + 1. / a)
                   //Rim highlight
                   / (.03 + abs(length(p) - .7)));
    
    return color;
}