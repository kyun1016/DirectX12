#include "Memory.hlsli"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosL : POSITION;
    
    nointerpolation uint MatIndex : MATINDEX;
};
 
VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout;

    InstanceData instData = gInstanceData[instanceID + gBaseInstanceIndex];
    float4x4 world = instData.World;
    uint matIndex = instData.MaterialIndex;
    
    vout.MatIndex = matIndex;
	// Use local vertex position as cubemap lookup vector.
    vout.PosL = vin.PosL;
	
	// Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), world);

	// Always center sky about camera.
    posW.xyz += gEyePosW;

	// Set z = w so that z/w = 1 (i.e., skydome always on far plane).
    vout.PosH = mul(posW, gViewProj).xyww;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    MaterialData matData = gMaterialData[pin.MatIndex];
    uint diffuseTexIndex = matData.DiffuseMapIndex;
    
    return gCubeMap[diffuseTexIndex].Sample(gsamLinearWrap, pin.PosL);
}