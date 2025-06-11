// https://registry.khronos.org/OpenGL-Refpages/gl4/html/mix.xhtml
// https://anteru.net/blog/2016/mapping-between-HLSL-and-GLSL/
// https://www.shadertoy.com/view/W3X3zl
#include "memory.hlsli"

// CC0: More glow experiments on a sunday
// More tinkering

#define TIME        iTime
#define RESOLUTION  iResolution
#define ROT(a)      float2x2(cos(a), sin(a), -sin(a), cos(a))

static const float PI = acos(-1.);
static const float TAU = 2. * PI;

float mod(float x, float y)
{
    return x - y * floor(x / y);
}

float2 mod(float2 x, float y)
{
    return x - y * floor(x / y);
}

// License: Unknown, author: Matt Taylor (https://github.com/64), found: https://64.github.io/tonemapping/
float3 aces_approx(float3 v)
{
    v = max(v, 0.0);
    v *= 0.6;
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((v * (a * v + b)) / (v * (c * v + d) + e), 0.0, 1.0);
}

float3 offset(float t)
{
    t *= 0.25;
    return 0.2 * float3(sin(TAU * t), sin(0.5 * t * TAU), cos(TAU * t));
}

float3 doffset(float t)
{
    const float dt = 0.01;
    return (offset(t + dt) - offset(t - dt)) / (2. * dt);
}

// License: MIT, author: Inigo Quilez, found: https://www.iquilezles.org/www/articles/smin/smin.htm
float pmin(float a, float b, float k)
{
    float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
    // return mix(b, a, h) - k * h * (1.0 - h);
    return lerp(b, a, h) - k * h * (1.0 - h);
}

float pmax(float a, float b, float k)
{
    return -pmin(-a, -b, k);
}

float3 palette(float a)
{
    return 1. + sin(float3(0, 1, 2) + a);
}

float apollonian(float4 p, float s, float w, out float off)
{
    float scale = 1.0;

    for (int i = 0; i < 6; i++)
    {
        p = -1.0 + 2.0 * frac(0.5 * p + 0.5);
        float r2 = dot(p, p);
        float k = s / r2;
        p *= k;
        scale *= k;
    }
    float4 sp = p / scale;
    float4 ap = abs(sp) - w;
    float d = pmax(ap.w, ap.y, w * 10.);
    off = length(sp.xz);
    return d;
}

float df(float3 p, float w, out float off)
{
    float tm = mod(TIME + 50., 800.0) * 0.5;
    float scale = lerp(1.85, 1.5, 0.5 - 0.5 * cos(TAU * tm / 800.));
    float2x2 rot = ROT(tm*TAU/400.0);
    
    float4 p4 = float4(p, 0.1);
    p4.yw = mul(p4.yw, rot);
    p4.zw = mul(p4.zw, transpose(rot));
    return apollonian(p4, scale, w, off);
}

float3 glowmarch(float3 col, float3 ro, float3 rd, float tinit)
{
    float t = tinit;
    float pd = tinit;
    float tm = 0.;
    for (int i = 0; i < 60; ++i)
    {
        float3 p = ro + rd * t;
        float off;
        float d = df(p, 6E-5 + t * t * 2E-3, off);
        float3 gcol = 1E-9 * (palette((log(off))) + 5E-2) / max(d * d, 1E-8);
        col += gcol * smoothstep(0.5, 0., t);
        t += 0.5 * d;
        if (t > 0.5)
            break;
    }
    return col;
}

float3 render(float3 col, float3 ro, float3 rd)
{
    col = glowmarch(col, ro, rd, 1E-2);
    return col;
}

float3 effect(float2 p, float2 pp, float2 q)
{
    float tm = mod(TIME + 50., 800.0) * 0.5;
    tm *= 0.05;
    float3 ro = offset(tm);
    float3 dro = doffset(tm);
    float3 ww = normalize(dro);
    float3 uu = normalize(cross(float3(0, 1, 0), ww));
    float3 vv = cross(ww, uu);
    float3 rd = normalize(-p.x * uu + p.y * vv + 2. * ww);

    float3 col = float3(0.0, 0.0, 0.0);
    col += 1E-1 * palette(5.0 + 0.1 * p.y) / max(1.125 - q.y + 0.1 * p.x * p.x, 1E-1);
    col = render(col, ro, rd);
    col *= smoothstep(1.707, 0.707, length(pp));
    col -= float3(2.0, 3.0, 1.0) * 4E-2 * (0.25 + dot(pp, pp));
    col = aces_approx(col);
    col = sqrt(col);
    return col;
}

// void mainImage(out float4 fragColor, in float2 fragCoord)
// {
//     float2 q = fragCoord / RESOLUTION.xy;
//     float2 p = -1. + 2. * q;
//     float2 pp = p;
//     p.x *= RESOLUTION.x / RESOLUTION.y;
//     float3 col = effect(p, pp, q);
//     fragColor = float4(col, 1.0);
// }

struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 PS(PixelShaderInput input) : SV_TARGET
{
    float2 q = input.texcoord;
    q.y = 1 - q.y;
    float2 p = -1. + 2. * q;
    float2 pp = p;
    p.x *= RESOLUTION.x / RESOLUTION.y;
    float3 col = effect(p, pp, q);
    return float4(col, 1.0);
}