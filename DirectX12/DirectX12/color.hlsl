cbuffer cbPerObject : register(b0)
{
    float4x4 gWorldViewProj;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float4 Color : COLOR;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
};

struct PS_OUTPUT
{
    float4 Scene;
    float4 Imgui;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
	
	// Transform to homogeneous clip space.
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
	
	// Just pass vertex color into the pixel shader.
    vout.Color = vin.Color;
    
    return vout;
}

//PS_OUTPUT PS(VertexOut pin) : SV_Target
//{
//    PS_OUTPUT ret;
//    ret.Scene = pin.Color;
//    ret.Imgui = pin.Color;
    
//    return ret;
//}

float4 PS(VertexOut pin) : SV_Target
{
    PS_OUTPUT ret;
    ret.Scene = pin.Color;
    ret.Imgui = pin.Color;
    
    return pin.Color;
}