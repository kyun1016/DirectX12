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
    
    nointerpolation uint MatIndex : MATINDEX;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout = (VertexOut) 0.0f;

    InstanceData instData = gInstanceData[instanceID + gBaseInstanceIndex];
    // Already in homogeneous clip space.
    vout.PosH = float4(vin.PosL, 1.0f);
    vout.TexC = vin.TexC;
    vout.MatIndex = instData.MaterialIndex;
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
    ret.color0 = float4(gShadowMap[pin.MatIndex % MAX_LIGHTS].Sample(gsamLinearWrap, pin.TexC).rrr, 1.0f);
    ret.color1 = ret.color0;
    return ret;
}
