#include "Memory.hlsli"

// Billboard
struct BillboardVertexIn
{
    float3 PosW : POSITION;
    float2 SizeW : SIZE;
};

BillboardVertexIn main(BillboardVertexIn vin)
{
    return vin;
}