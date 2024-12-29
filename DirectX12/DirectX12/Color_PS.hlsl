#include "Color_Common.hlsli" // ���̴������� include ��� ����

float4 PS(VertexOut pin) : SV_Target
{
    PS_OUTPUT ret;
    ret.Scene = pin.Color;
    ret.Imgui = pin.Color;
    
    const float pi = 3.14159;
    
    float s = 0.5f * sin(2 * gTime - 0.25f * pi) + 0.5f;
    
    float4 c = lerp(pin.Color, gPulseColor, s);
    
    return c;
}