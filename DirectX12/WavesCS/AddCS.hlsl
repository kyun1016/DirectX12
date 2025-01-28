#include "Common.hlsli"

struct Data
{
    float3 v1;
    float2 v2;
};

//StructuredBuffer<Data> gInputA : register(t0);
//StructuredBuffer<Data> gInputB : register(t1);
//RWStructuredBuffer<Data> gOutput : register(u0);

Texture2D gInputA;
Texture2D gInputB;
RWTexture2D<float4> gOutput;

[numthreads(16, 16, 1)]
void main( uint3 dispatchThreadID : SV_DispatchThreadID )
{
    gOutput[dispatchThreadID.xy] = gInputA[dispatchThreadID.xy] + gInputB[dispatchThreadID.xy];
}