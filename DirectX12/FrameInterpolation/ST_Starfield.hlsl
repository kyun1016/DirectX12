// https://registry.khronos.org/OpenGL-Refpages/gl4/html/mix.xhtml
// https://anteru.net/blog/2016/mapping-between-HLSL-and-GLSL/
// https://www.shadertoy.com/view/lcjGWV
// https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-per-component-math

#include "ST_Core.hlsli"

// https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-asint
#define FFBI(a) asint(cos(a))^asint(a)

static const float3 RAY_ORIGIN = float3(0.2, 0.2, -5.0); // ray origin
static const float3 RAY_TARGET = float3(0.0, 0.0, 0.0); // ray target

// Ray marching parameters
static const float MAX_STEPS = 100.0; // Maximum number of steps
static const float MAX_DIST = 100.0; // How far we march before we give up
static const float EPSILON = 0.001; // How close we need to be to consider it a hit

// 
static const float SIZE = 1.0;
    
static const float3 z = normalize(RAY_TARGET - RAY_ORIGIN);
static const float3 x = normalize(cross(z, float3(0.0, -1.0, 0.0)));
static const float3 y = cross(z, x);


struct Grid
{
    float3 id;
    float d;
};

float hash(float3 uv)
{
    int x = int (uv.x);
    int y = int (uv.y);
    int z = int (uv.z);
    return float((x * x + y) * (y * y - x) * (z * z + x)) / 2.14e9;
}

void dogrid(inout Grid gr, float3 ro, float3 rd, float size)
{
    gr.id = (floor(ro + rd * 1E-3) / size + float3(.5, .5, .5)) * size;
    float3 src = -(ro - gr.id) / rd;
    float3 dst = abs(.5 * size) / rd;
    float3 bz = src + dst;
    gr.d = min(bz.x, min(bz.y, bz.z));
}

float3 erot(float3 p)
{
    float temp = iTime * .33;
    float3 ax = normalize(sin(float3(temp, temp, temp) + float3(-.6, .4, .2)));
    float t = iTime * .2;
    
    return lerp(dot(ax, p) * ax, p, cos(t)) + cross(ax, p) * sin(t);
}

float4 PS(PixelShaderInput input) : SV_TARGET
{
    // vec2 uv = (fragCoord.xy -.5* iResolution.xy)/iResolution.y;
    float2 uv = -1.0 + 2.0*input.texcoord;
    float3 col = float3(0.0, 0.0, 0.0);
    
    Grid gr;
    gr.id = float3(0.0, 0.0, 0.0);
    gr.d = 0.0;
        
    float3x3 m = float3x3(x, y, z);
    float3 rd = mul(m, normalize(float3(uv, 2. + tanh(hash(uv.xyy + float3(iTime, iTime, iTime)) * .5 + 10. * sin(iTime)))));
    
    float g = .0;
    float epsilon = EPSILON;
    float gridlen = .0;
    
    for (float i = 0.; i < MAX_STEPS; ++i)
    {
        float3 curPos = RAY_ORIGIN + rd * g;
        // curPos = erot(curPos);
        curPos.z += iTime;
        
        float3 op = curPos; // original position
        if (gridlen <= epsilon)
        {
            dogrid(gr, curPos, rd, SIZE);
            gridlen += gr.d;
        }
        
        curPos -= gr.id;
        float gy = dot(sin(gr.id * 2.), cos(gr.id.zxy * 5.));
        float rn = hash(gr.id + float3(floor(iTime), floor(iTime), floor(iTime)));
        curPos.x += sin(rn) * 0.25;
        
        float h = rn > .0 ? .5 : length(curPos) - .01 - gy * .05 + rn * .02;
        epsilon = max(.001 + op.z * .000002, abs(h));
        g += epsilon;
        col += float3(.25, .25, 1. + abs(rn)) * (.025 + (.02 * exp(5. * frac(gy + iTime)))) / exp(epsilon * epsilon * i);
    }
    
    col *= exp(-.08 * g);
    
    return float4(sqrt(col), 1.0);
}