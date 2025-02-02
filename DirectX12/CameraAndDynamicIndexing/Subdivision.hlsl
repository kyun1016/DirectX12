// Defaults for number of lights.
#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

#define MaxLights 16

#include "LightingUtil.hlsli"

Texture2D gDiffuseMap : register(t0);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gTexTransform;
    float4x4 gWorldInvTranspose;
};

// Constant data that varies per material.
cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerPassPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;

    	// Allow application to change fog parameters once per frame.
	// For example, we may only use fog for certain times of day.
    float4 gFogColor;
    float gFogStart;
    float gFogRange;
    float2 cbPerObjectPad2;
    
    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    Light gLights[MaxLights];
};

cbuffer cbMaterial : register(b2)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
    float4x4 gMatTransform;
};

// Subdivision
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

struct SubdivisionGeometryOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
};

VertexIn VS(VertexIn vin)
{
    return vin;
}

void Subdivide(VertexIn inVerts[3], out VertexIn outVerts[6])
{
    //         1(5)                
    //          *                
    //         / \               
    //        /   \              
    // m0(1) *-----* m1(3)       
    //      / \   / \            
    //     /   \ /   \           
    //    *-----*-----*          
    // 0(0)   m2(2)   2(4)          
    
    VertexIn m[3];
    
    // 각 변의 중점을 계산
    m[0].PosL = 0.5f * (inVerts[0].PosL + inVerts[1].PosL);
    m[1].PosL = 0.5f * (inVerts[1].PosL + inVerts[2].PosL);
    m[2].PosL = 0.5f * (inVerts[2].PosL + inVerts[0].PosL);
    
    // // 단위 구에 투영
    // m[0].PosL = normalize(m[0].PosL);
    // m[1].PosL = normalize(m[1].PosL);
    // m[2].PosL = normalize(m[2].PosL);
    
    // 법선을 유도
    m[0].NormalL = m[0].PosL;
    m[1].NormalL = m[1].PosL;
    m[2].NormalL = m[2].PosL;
    
    // 텍스처 좌표를 보간
    m[0].TexC = 0.5f * (inVerts[0].TexC + inVerts[1].TexC);
    m[1].TexC = 0.5f * (inVerts[1].TexC + inVerts[2].TexC);
    m[2].TexC = 0.5f * (inVerts[2].TexC + inVerts[0].TexC);
    
    outVerts[0] = inVerts[0];
    outVerts[1] = m[0];
    outVerts[2] = m[2];
    outVerts[3] = m[1];
    outVerts[4] = inVerts[2];
    outVerts[5] = inVerts[1];
}

void OutputSubdivision(VertexIn v[6], inout TriangleStream<SubdivisionGeometryOut> triStream)
{
    SubdivisionGeometryOut gout[6];
    
    [unroll]
    for (int i = 0; i < 6; ++i)
    {
        // world space로 변환
        float4 posW = mul(float4(v[i].PosL, 1.0f), gWorld);
        gout[i].PosW = posW.xyz;
        gout[i].NormalW = mul(v[i].NormalL, (float3x3) gWorldInvTranspose);
        
        // homogeneous clip space로 변환
        gout[i].PosH = mul(posW, gViewProj);
        
        float4 texC = mul(float4(v[i].TexC, 0.0f, 1.0f), gTexTransform);
        gout[i].TexC = mul(texC, gMatTransform).xy;
    }
    //         1(5)                     1(5)                
    //          *                        *                
    //         / \                      / \             <- Strip 2 (1 5 3)
    //        /   \                    / 3 \              
    // m0(1) *-----* m1(3)  ->  m0(1) *-----* m1(3)       
    //      / \   / \                / \ 1 / \            
    //     /   \ /   \              / 0 \ / 2 \         <- Strip 1 (0 1 2 3 4) -> (0 1 2) / (1 2 3) / (2 3 4)
    //    *-----*-----*            *-----*-----*          
    // 0(0)   m2(2)   2(4)       0(0)   m2(2)   2(4)          
    
    // 삼각형을 2개의 띠로 그린다.
    // 1. 띠 1: 아래쪽 삼각형 세 개
    // 2. 띠 2: 위쪽 삼각형 1개
    [unroll]
    for (int j = 0; j < 5; ++j)
    {
        triStream.Append(gout[j]);
    }
    triStream.RestartStrip();
    
    triStream.Append(gout[1]);
    triStream.Append(gout[5]);
    triStream.Append(gout[3]);
}

[maxvertexcount(8)]
void GS(triangle VertexIn gin[3], inout TriangleStream<SubdivisionGeometryOut> triStream)
{
    VertexIn v[6];
    Subdivide(gin, v);
    OutputSubdivision(v, triStream);
}


struct PixelOut
{
    float4 color0 : SV_Target0;
    float4 color1 : SV_Target1;
};

PixelOut PS(VertexOut pin) : SV_Target
{
    float4 diffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC) * gDiffuseAlbedo;
	
#ifdef ALPHA_TEST
	// Discard pixel if texture alpha < 0.1.  We do this test as soon 
	// as possible in the shader so that we can potentially exit the
	// shader early, thereby skipping the rest of the shader code.
	clip(diffuseAlbedo.a - 0.1f);
#endif

    // Interpolating normal can unnormalize it, so renormalize it.
    pin.NormalW = normalize(pin.NormalW);

    // Vector from point being lit to eye. 
    float3 toEyeW = gEyePosW - pin.PosW;
    float distToEye = length(toEyeW);
    toEyeW /= distToEye; // normalize

    // Light terms.
    float4 ambient = gAmbientLight * diffuseAlbedo;

    const float shininess = 1.0f - gRoughness;
    Material mat = { diffuseAlbedo, gFresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        pin.NormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

#ifdef FOG
    float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
    litColor = lerp(litColor, gFogColor, fogAmount);
#endif

    // Common convention to take alpha from diffuse albedo.
    litColor.a = diffuseAlbedo.a;

    PixelOut ret;
    ret.color0 = litColor;
    ret.color1 = litColor;
    return ret;
}