// #include "Memory.hlsli"
struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
    float3 TangentU : TANGENT;
};

struct VertexOut
{
    float4 PosL : SV_POSITION;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout;
	
    vout.PosL = float4(vin.PosL * 0.5, 1.0f);
    
    return vout;
}

struct PixelOut
{
    float4 color : SV_Target0;
};

PixelOut PS(VertexOut pin)
{
    PixelOut ret;
    ret.color.rgba = 1.0;
    return ret;
}