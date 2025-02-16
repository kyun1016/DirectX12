#include "Memory.hlsli"

struct VertexIn
{
    float3 PosL : POSITION;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 TexC : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;

    // Already in homogeneous clip space.
    vout.PosH = float4(vin.PosL, 1.0f);
	
    vout.TexC = vin.TexC;
	
    return vout;
}

struct PixelOut
{
    float4 color0 : SV_Target0;
    float4 color1 : SV_Target1;
};

PixelOut PS(VertexOut pin)
{
    PixelOut ret;
    ret.color0 = float4(gShadowMap[0].Sample(gsamLinearWrap, pin.TexC).rrr, 1.0f);
    ret.color1 = float4(gShadowMap[0].Sample(gsamLinearWrap, pin.TexC).rrr, 1.0f);
    return ret;
}
