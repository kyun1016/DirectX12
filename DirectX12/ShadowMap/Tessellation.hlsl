#include "Memory.hlsli"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
    float3 TangentU : TANGENT;
};

struct VertexOut
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
    float3 TangentU : TANGENT;
    
    // nointerpolation is used so the index is not interpolated 
	// across the triangle.
    nointerpolation uint InsIndex : SV_InstanceID;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout;
	
    vout.PosL = vin.PosL;
    vout.NormalL = vin.NormalL;
    vout.TexC = vin.TexC;
    vout.TangentU = vin.TangentU;
    vout.InsIndex = instanceID + gBaseInstanceIndex;
    
    return vout;
}
 
struct PatchTess
{
    float EdgeTess[4] : SV_TessFactor;
    float InsideTess[2] : SV_InsideTessFactor;
};

PatchTess ConstantHS(InputPatch<VertexOut, 4> patch, uint patchID : SV_PrimitiveID)
{
    PatchTess pt;
    
    InstanceData instData = gInstanceData[patch[0].InsIndex];
    float4x4 world = instData.World;
    float4x4 texTransform = instData.TexTransform;
    float4x4 worldInvTranspose = instData.WorldInvTranspose;
    uint matIndex = instData.MaterialIndex;
	
    float3 centerL = 0.25f * (patch[0].PosL + patch[1].PosL + patch[2].PosL + patch[3].PosL);
    float3 centerW = mul(float4(centerL, 1.0f), world).xyz;
	
    float d = distance(centerW, gEyePosW);

	// Tessellate the patch based on distance from the eye such that
	// the tessellation is 0 if d >= d1 and 64 if d <= d0.  The interval
	// [d0, d1] defines the range we tessellate in.
	
    const float d0 = 20.0f;
    const float d1 = 100.0f;
    float tess = 64.0f * saturate((d1 - d) / (d1 - d0));

	// Uniformly tessellate the patch.

    pt.EdgeTess[0] = tess;
    pt.EdgeTess[1] = tess;
    pt.EdgeTess[2] = tess;
    pt.EdgeTess[3] = tess;
	
    pt.InsideTess[0] = tess;
    pt.InsideTess[1] = tess;
	
    return pt;
}

struct HullOut
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
    float3 TangentU : TANGENT;
    
    // nointerpolation is used so the index is not interpolated 
	// across the triangle.
    nointerpolation uint InsIndex : SV_InstanceID;
};

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
HullOut HS(InputPatch<VertexOut, 4> p,
           uint i : SV_OutputControlPointID,
           uint patchId : SV_PrimitiveID)
{
    HullOut hout;
	
    hout.PosL = p[i].PosL;
    hout.NormalL = p[i].NormalL;
    hout.TexC = p[i].TexC;
    hout.TangentU = p[i].TangentU;
    hout.InsIndex = p[i].InsIndex;
	
    return hout;
}

struct DomainOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 TexC : TEXCOORD;
    
    // nointerpolation is used so the index is not interpolated 
	// across the triangle.
    nointerpolation uint MatIndex : MATINDEX;
};

// The domain shader is called for every vertex created by the tessellator.  
// It is like the vertex shader after tessellation.
[domain("quad")]
DomainOut DS(PatchTess patchTess,
             float2 uv : SV_DomainLocation,
             const OutputPatch<HullOut, 4> quad)
{
    DomainOut dout;
    
    InstanceData instData = gInstanceData[quad[0].InsIndex];
    float4x4 world = instData.World;
    float4x4 texTransform = instData.TexTransform;
    float4x4 worldInvTranspose = instData.WorldInvTranspose;
    float2 displacementMapTexelSize = instData.DisplacementMapTexelSize;
    float gridSpatialStep = instData.GridSpatialStep;
    uint matIndex = instData.MaterialIndex;
    
    MaterialData matData = gMaterialData[matIndex];
	
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
    
    float3 tan1 = lerp(quad[0].TangentU, quad[1].TangentU, uv.x);
    float3 tan2 = lerp(quad[2].TangentU, quad[3].TangentU, uv.x);
    float3 tan = lerp(tan1, tan2, uv.y);
	
	
    float4 posW = mul(float4(p, 1.0f), world);
    dout.PosW = posW.xyz;
    
    dout.NormalW = mul(n, (float3x3) worldInvTranspose);
    dout.TangentW = mul(tan, (float3x3) worldInvTranspose);
    
    dout.PosH = mul(posW, gViewProj);
    
    float4 texC = mul(float4(t, 0.0f, 1.0f), texTransform);
    dout.TexC = mul(texC, matData.MatTransform).xy;
    
    dout.MatIndex = matIndex;
    
    // TODO: TangentW 관리 로직 추가
	
    return dout;
}
