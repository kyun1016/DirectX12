#include "Memory.hlsli"

// Subdivision
struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

VertexIn main(VertexIn vin)
{
    return vin;
}