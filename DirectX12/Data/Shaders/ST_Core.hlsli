#include "memory.hlsli"

#define PI 3.141592
#define TAU (PI*2.0)

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

struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};