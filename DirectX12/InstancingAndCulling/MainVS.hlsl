#include "Memory.hlsli"
// Main 
struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
};

VertexOut main(VertexIn vin)
{
    // Fetch the material data.
    MaterialData matData = gMaterialData[gMaterialIndex];
    
    VertexOut vout = (VertexOut) 0.0f;

#ifdef DISPLACEMENT_MAP
	// Sample the displacement map using non-transformed [0,1]^2 tex-coords.
    vin.PosL.y += gDisplacementMap.SampleLevel(gsamLinearWrap, vin.TexC, 1.0f).r;
	
	// Estimate normal using finite difference.
    float du = gDisplacementMapTexelSize.x;
    float dv = gDisplacementMapTexelSize.y;
    float l = gDisplacementMap.SampleLevel(gsamPointClamp, vin.TexC - float2(du, 0.0f), 0.0f).r;
    float r = gDisplacementMap.SampleLevel(gsamPointClamp, vin.TexC + float2(du, 0.0f), 0.0f).r;
    float t = gDisplacementMap.SampleLevel(gsamPointClamp, vin.TexC - float2(0.0f, dv), 0.0f).r;
    float b = gDisplacementMap.SampleLevel(gsamPointClamp, vin.TexC + float2(0.0f, dv), 0.0f).r;
    vin.NormalL = normalize(float3(-r + l, 2.0f * gGridSpatialStep, b - t));
#endif
    
    // Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);

    // Transform to homogeneous clip space.
    vout.PosH = mul(posW, gViewProj);
	
	// Output vertex attributes for interpolation across triangle.
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, matData.MatTransform).xy;
	
    return vout;
}