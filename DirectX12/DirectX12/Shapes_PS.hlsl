#include "Shapes_Common.hlsli"

float4 PS(VertexOut pin) : SV_Target
{
    return pin.Color;
}