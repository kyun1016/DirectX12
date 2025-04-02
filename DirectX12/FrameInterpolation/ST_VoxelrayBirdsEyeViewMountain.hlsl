// https://registry.khronos.org/OpenGL-Refpages/gl4/html/mix.xhtml
// https://registry.khronos.org/OpenGL-Refpages/gl4/html/texelFetch.xhtml
// https://anteru.net/blog/2016/mapping-between-HLSL-and-GLSL/
// https://www.shadertoy.com/view/W3X3DB
#include "memory.hlsli"

// https://www.shadertoy.com/view/W3X3DB voxelray birds eye view mountain, 2025 by jt
// based on https://www.shadertoy.com/view/cdsGR7 voxelray

// A very simple voxel ray cast bird's view rendering of a terrain,
// to experiment with a pixelated voxel style and to test performance.
// Contrary to my expectations on-the fly terrain generation is FASTER than texture-lookup!

// tags: 3d, ray, terrain, pixel, voxel, landscape, mountain, perlin, noise

// The MIT License
// Copyright (c) 2025 Jakob Thomsen
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

float mod(float x, float y)
{
    return x - y * floor(x / y);
}

#define NORMAL_SMOOTHNESS 1

float hash12(float2 p) // Hash without Sine by Dave_Hoskins https://www.shadertoy.com/view/4djSRW
{
    float3 p3 = frac(float3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

float2 hash22(float2 p) // Hash without Sine by Dave_Hoskins https://www.shadertoy.com/view/4djSRW
{
    float3 p3 = frac(float3(p.xyx) * float3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.xx + p3.yz) * p3.zy);
}

float3 hash32(float2 p) // Hash without Sine by Dave_Hoskins https://www.shadertoy.com/view/4djSRW
{
    float3 p3 = frac(float3(p.xyx) * float3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz + 33.33);
    return frac((p3.xxy + p3.yzz) * p3.zyx);
}

float3 hash33(float3 p3) // Hash without Sine by Dave_Hoskins https://www.shadertoy.com/view/4djSRW
{
    p3 = frac(p3 * float3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz + 33.33);
    return frac((p3.xxy + p3.yxx) * p3.zyx);
}

float2 grad(int2 z) // Perlin noise by inigo quilez - iq/2013   https://www.shadertoy.com/view/XdXGW8
{
    // 2D to 1D  (feel free to replace by some other)
    int n = z.x + z.y * 11111;

    // Hugo Elias hash (feel free to replace by another one)
    n = (n << 13) ^ n;
    n = (n * (n * n * 15731 + 789221) + 1376312589) >> 16;

#if 0

    // simple random vectors
    return float2(cos(float(n)),sin(float(n)));

#else

    // Perlin style vectors
    n &= 7;
    float2 gr = float2(n & 1, n >> 1) * 2.0 - 1.0;
    return (n >= 6) ? float2(0.0, gr.x) :
           (n >= 4) ? float2(gr.x, 0.0) :
                              gr;
#endif
}
/*
float2 grad(int2 v)
{
    return hash22(float2(v))*2.0-1.0;
}
*/
float noise(in float2 p) // Perlin noise by inigo quilez - iq/2013   https://www.shadertoy.com/view/XdXGW8
{
    int2 i = int2(floor(p));
    float2 f = frac(p);

    float2 u = f * f * (3.0 - 2.0 * f); // feel free to replace by a quintic smoothstep instead

    return lerp(lerp(dot(grad(i + int2(0, 0)), f - float2(0.0, 0.0)),
                     dot(grad(i + int2(1, 0)), f - float2(1.0, 0.0)), u.x),
                lerp(dot(grad(i + int2(0, 1)), f - float2(0.0, 1.0)),
                     dot(grad(i + int2(1, 1)), f - float2(1.0, 1.0)), u.x), u.y);
}

#define pi 3.1415926
#define tau (pi+pi)

float3 rainbow(float t)
{
    return 0.5 + 0.5 * cos(tau * (3.0 * t + float3(0, 1, 2)) / 3.0);
}

bool map(uint s, int3 tile)
{
    float g = clamp(noise(3.5 * mul(float2x2(1, 1, -1, 1), float2(tile.xy)) * .01), -1.0, 1.0);
    //float g = texture(iChannel0, float2(tile.xy)*0.005).x;
    return float(s) * g > float(tile.z - 1);
}

bool diagonal_shadow(uint s, int3 p, int z)
{
    bool ret = false;
    if (map(s, p))
        ret = true;
        
    while (p.z < z)
    {
        p++;
        if (map(s, p))
            ret = true;
    }

    return ret; // sky
}

float3 normal(uint s, int3 p)
{
    int n = NORMAL_SMOOTHNESS; // window-size for calculating normal
    int3 sum = int3(0,0,0);
    int3 d;
    for (d.z = -n; d.z <= +n; d.z++)
    {
        for (d.y = -n; d.y <= +n; d.y++)
        {
            for (d.x = -n; d.x <= +n; d.x++)
            {
                if (map(s, p + d))
                {
                    sum -= d;
                }
            }
        }
    }

    return (sum != int3(0,0,0)) ? normalize(float3(sum)) : float3(0.0, 0.0, 0.0);
}

struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

// void mainImage(out vec4 fragColor, float2 p)
// {
//     float2 R = iResolution.xy;
//     p = (p + p - R) / R.y; // thanks Fabrice for reminding me (repeatedly) to use this nice one-liner :D
//     /*
//     if(abs(p.x) < 0.01 || abs(p.y) < 0.01)
//     {
//         fragColor = vec4(0);
//         return;
//     }
//     */
//     float2 m = iMouse.xy;
//     bool demo = all(lessThan(m, float2(10)));
//     m = !demo ? (m + m - R) / R.y : float2(cos(iTime * tau * 0.1), sin(iTime * tau * 0.1));
//     float z = 2.0;
//     uint h = 100u;
//     float3 rd = float3(p.xy / z, -1); // must NOT be normalized!
//     float3 ro = float3(-m / z, 1) * float(h);
// 
//     uint s = 20u; // scale height
//     uint h0 = h - s;
//     uint h1 = h + s;
// 
//     float3 color = float3(0.0);
// 
//     {
//         for (uint i = h0; i < h1; i++)
//         {
//             ifloat3 tile = ifloat3(floor(ro + rd * float(i)));
//             /*
//             {
//                 if(map(s,tile) && tile.xy == ifloat2(ro.xy)) // cursor
//                 {
//                     color = float3(1);
//                     break;
//                 }
//             }
//             */
//             {
//                 if (map(s, tile)) // landscape
//                 {
//                     color = hash32(float2(tile)) * rainbow(float(tile.z) / 20.0);
//                     float diffuse = diagonal_shadow(s, tile, int(h0)) ? 0.0 : max(0.0, dot(normalize(float3(1, 1, 1)), normal(s, tile)));
//                     float ambient = 0.1;
//                     float light = (ambient + diffuse);
//                     color = color * light;
//                     break;
//                 }
//             }
//         }
//     }
// 
//     fragColor = vec4(sqrt(color), 1.0); // approximate gamma
// }

bool2 lessThan(float2 x, float2 y)
{
    return bool2(x.x < y.x, x.y < y.y);
}

float4 PS(PixelShaderInput input) : SV_TARGET
{
    // float2 R = iResolution.xy;
    // p = (p + p - R) / R.y; // thanks Fabrice for reminding me (repeatedly) to use this nice one-liner :D
    float2 p = input.texcoord * 2 - 1.0;;
     /*
     if(abs(p.x) < 0.01 || abs(p.y) < 0.01)
     {
         fragColor = vec4(0);
         return;
     }
     */
    // float2 m = iMouse.xy;
    // bool demo = all(lessThan(m, float2(10.0, 10.0)));
    // m = !demo ? (m + m - R) / R.y : float2(cos(iTime * tau * 0.1), sin(iTime * tau * 0.1));
    
    float2 m = float2(cos(iTime * tau * 0.1), sin(iTime * tau * 0.1));
    float z = 2.0;
    uint h = 100u;
    float3 rd = float3(p.xy / z, -1); // must NOT be normalized!
    float3 ro = float3(-m / z, 1) * float(h);
 
    uint s = 20u; // scale height
    uint h0 = h - s;
    uint h1 = h + s;
 
    float3 color = float3(0.0, 0.0, 0.0);
 
     {
        for (uint i = h0; i < h1; i++)
        {
            int3 tile = int3(floor(ro + rd * float(i)));
             /*
             {
                 if(map(s,tile) && tile.xy == ifloat2(ro.xy)) // cursor
                 {
                     color = float3(1);
                     break;
                 }
             }
             */
             {
                if (map(s, tile)) // landscape
                {
                    color = hash32(float2(tile.xy)) * rainbow(float(tile.z) / 20.0);
                    float diffuse = diagonal_shadow(s, tile, int(h0)) ? 0.0 : max(0.0, dot(normalize(float3(1, 1, 1)), normal(s, tile)));
                    float ambient = 0.1;
                    float light = (ambient + diffuse);
                    color = color * light;
                    break;
                }
            }
        }
    }
 
    return float4(sqrt(color), 1.0); // approximate gamma
}