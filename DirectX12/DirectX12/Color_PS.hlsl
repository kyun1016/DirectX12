#include "Common.hlsli" // 쉐이더에서도 include 사용 가능

float4 PS(VertexOut pin) : SV_Target
{
    PS_OUTPUT ret;
    ret.Scene = pin.Color;
    ret.Imgui = pin.Color;
    
    return pin.Color;
}