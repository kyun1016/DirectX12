// https://registry.khronos.org/OpenGL-Refpages/gl4/html/mix.xhtml
// https://anteru.net/blog/2016/mapping-between-HLSL-and-GLSL/
// https://www.shadertoy.com/view/4tXSzM
#include "memory.hlsli"

float mod(float x, float y)
{
    return x - y * floor(x / y);
}

struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

// CC0: More glow experiments on a sunday
// More tinkering

#define moblur 4
#define harmonic 25
#define triangle 1

float3 circle(float2 uv, float rr, float cc, float ss)
{
    float2x2 m = float2x2(cc, ss, -ss, cc);
    uv = mul(uv, m);
    if(rr < 0.)
        uv.y = -uv.y;
    rr = abs(rr);
    float r = length(uv) - rr;
    float pix = fwidth(r);
    float c = smoothstep(0., pix, abs(r));
    float l = smoothstep(0., pix, abs(uv.x) + step(uv.y, 0.) + step(rr, uv.y));
    
    return float3(c, c * l, c * l);
}

float3 ima(float2 uv, float th0)
{
    float3 col = float3(1.0, 1.0, 1.0);
    float2 uv0 = uv;
    th0 -= max(0., uv0.x - 1.5) * 2.;
    th0 -= max(0., uv0.y - 1.5) * 2.;
#ifndef triangle
    float lerpy = 1.;
#else
    float lerpy = smoothstep(-0.6, 0.2, cos(th0 * 0.1));
#endif
    
    for (int i = 1; i < harmonic; i+=2)
    {
        float th = th0 * float(i);
        float fl = mod(float(i), 4.0) - 2.0; // used to be repeated assignment fl=-fl, but compiler bugs. :(
        float cc = cos(th) * fl;
        float ss = sin(th);
        float trir = -fl / float(i * i);
        float sqrr = 1. / float(i);
        float rr = lerp(trir, sqrr, lerpy);
        col = min(col, circle(uv, rr, cc, ss));
        uv.x += rr * ss;
        uv.y -= rr * cc;
    }
    float pix = fwidth(uv0.x);
    if (uv.y > 0. && frac(uv0.y * 10.) < 0.5)
        col.yz = min(col.yz, smoothstep(0., pix, abs(uv.x)));
    if (uv.x > 0. && frac(uv0.x * 10.) < 0.5)
        col.yz = min(col.yz, smoothstep(0., pix, abs(uv.y)));
    if (uv0.x >= 1.5)
    {
        float temp = smoothstep(0., fwidth(uv.y), abs(uv.y));
        col.xy = float2(temp, temp);
    }
    if (uv0.y >= 1.5)
    {
        float temp = smoothstep(0., fwidth(uv.x), abs(uv.x));
        col.xy = float2(temp, temp);
    }

    return col;
}

float4 PS(PixelShaderInput input) : SV_TARGET
{
    // input.texcoord.y = 1. - input.texcoord.y;
    float2 uv = 2.0 * input.texcoord - 1.;
    
    uv *= 5.;
    uv += 3.5;
    float th0 = iTime * 2.;
    float dt = 2. / 60. / float(moblur);
    float3 col = float3(0.0, 0.0, 0.0);
    
    for (int mb = 0; mb < moblur; ++mb)
    {
        col += ima(uv, th0);
        th0 += dt;
    }
    float temp = 1. / 2.2;
    col = pow(col * (1. / float(moblur)), float3(temp, temp, temp));
    
    return float4(col, 1.0);
}