#include "Common.hlsli" // ���̴������� include ��� ����

float4 PS(VertexOut pin) : SV_Target
{
    PS_OUTPUT ret;
    ret.Scene = pin.Color;
    ret.Imgui = pin.Color;
    
    return pin.Color;
}