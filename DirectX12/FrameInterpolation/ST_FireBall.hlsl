// https://anteru.net/blog/2016/mapping-between-HLSL-and-GLSL/
// https://www.shadertoy.com/view/lsf3RH
#include "memory.hlsli"

float mod(float x, float y)
{
    return x - y * floor(x / y);
}

float2 mod(float2 x, float y)
{
    return x - y * floor(x / y);
}

float3 mod(float3 x, float y)
{
    return x - y * floor(x / y);
}

float snoise(float3 uv, float res)
{
    const float3 s = float3(1e0, 1e2, 1e3);
	
    uv *= res;
	
    float3 uv0 = floor(mod(uv, res)) * s;
    float3 uv1 = floor(mod(uv + float3(1., 1., 1.), res)) * s;
	
    float3 f = frac(uv);
    f = f * f * (3.0 - 2.0 * f);

    float4 v = float4(uv0.x + uv0.y + uv0.z, uv1.x + uv0.y + uv0.z,
		      	  uv0.x + uv1.y + uv0.z, uv1.x + uv1.y + uv0.z);

    float4 r = frac(sin(v * 1e-1) * 1e3);
    float r0 = lerp(lerp(r.x, r.y, f.x), lerp(r.z, r.w, f.x), f.y);
	
    r = frac(sin((v + uv1.z - uv0.z) * 1e-1) * 1e3);
    float r1 = lerp(lerp(r.x, r.y, f.x), lerp(r.z, r.w, f.x), f.y);
	
    return lerp(r0, r1, f.z) * 2. - 1.;
}

struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 PS(PixelShaderInput input) : SV_TARGET
{
    float2 p = input.texcoord * 2 - 1.0;
    float color = 3.0 - (3. * length(2. * p));
	
    float3 coord = float3(atan2(p.x, p.y) / 6.2832 + .5, length(p) * .4, .5);
	
    for (int i = 1; i <= 7; i++)
    {
        float power = pow(2.0, float(i));
        color += (1.5 / power) * snoise(coord + float3(0., -iTime * .05, iTime * .01), power * 16.);
    }
    return float4(color, pow(max(color, 0.), 2.) * 0.4, pow(max(color, 0.), 3.) * 0.15, 1.0);
}