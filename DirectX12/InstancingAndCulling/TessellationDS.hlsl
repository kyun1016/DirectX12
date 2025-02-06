#include "Memory.hlsli"
 
struct PatchTess
{
    float EdgeTess[4] : SV_TessFactor;
    float InsideTess[2] : SV_InsideTessFactor;
};

struct HullOut
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct DomainOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
};

// The domain shader is called for every vertex created by the tessellator.  
// It is like the vertex shader after tessellation.
[domain("quad")]
DomainOut main(PatchTess patchTess,
             float2 uv : SV_DomainLocation,
             const OutputPatch<HullOut, 4> quad)
{
    MaterialData matData = gMaterialData[gMaterialIndex];
    
    DomainOut dout;
	
	// Bilinear interpolation.
    float3 v1 = lerp(quad[0].PosL, quad[1].PosL, uv.x);
    float3 v2 = lerp(quad[2].PosL, quad[3].PosL, uv.x);
    float3 p = lerp(v1, v2, uv.y);
    
    // Displacement mapping
    p.y = 0.3f * (p.z * sin(p.x) + p.x * cos(p.z));
    
    float3 n1 = lerp(quad[0].NormalL, quad[1].NormalL, uv.x);
    float3 n2 = lerp(quad[2].NormalL, quad[3].NormalL, uv.x);
    float3 n = lerp(n1, n2, uv.y);
    n.y = 0.3f * (n.z * sin(n.x) + n.x * cos(n.z));
    
    float2 t1 = lerp(quad[0].TexC, quad[1].TexC, uv.x);
    float2 t2 = lerp(quad[2].TexC, quad[3].TexC, uv.x);
    float2 t = lerp(t1, t2, uv.y);
	
	
    float4 posW = mul(float4(p, 1.0f), gWorld);
    dout.PosW = posW.xyz;
    
    dout.NormalW = mul(n, (float3x3) gWorld);
    
    dout.PosH = mul(posW, gViewProj);
    
    float4 texC = mul(float4(t, 0.0f, 1.0f), gTexTransform);
    dout.TexC = mul(texC, matData.MatTransform).xy;
	
    return dout;
}