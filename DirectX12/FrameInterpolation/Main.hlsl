#include "Memory.hlsli"
// Main 
struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
    float3 TangentU : TANGENT;
    
#ifdef SKINNED
    float3 BoneWeights : WEIGHTS;
    uint4 BoneIndices  : BONEINDICES;
#endif
};

struct VertexOut
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

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout = (VertexOut) 0.0f;
    
    InstanceData instData = gInstanceData[instanceID + gBaseInstanceIndex];
    float4x4 world = instData.World;
    float4x4 texTransform = instData.TexTransform;
    float4x4 worldInvTranspose = instData.WorldInvTranspose;
    float2 displacementMapTexelSize = instData.DisplacementMapTexelSize;
    float gridSpatialStep = instData.GridSpatialStep;
    int matIndex = instData.MaterialIndex;
    int displacementIndex = instData.DisplacementIndex;
    vout.MatIndex = matIndex;
    
    // Fetch the material data.
    MaterialData matData = gMaterialData[matIndex];
    
#ifdef SKINNED
    float weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    weights[0] = vin.BoneWeights.x;
    weights[1] = vin.BoneWeights.y;
    weights[2] = vin.BoneWeights.z;
    weights[3] = 1.0f - weights[0] - weights[1] - weights[2];

    float3 posL = float3(0.0f, 0.0f, 0.0f);
    float3 normalL = float3(0.0f, 0.0f, 0.0f);
    float3 tangentL = float3(0.0f, 0.0f, 0.0f);
    for(int i = 0; i < 4; ++i)
    {
        // Assume no nonuniform scaling when transforming normals, so 
        // that we do not have to use the inverse-transpose.

        posL += weights[i] * mul(float4(vin.PosL, 1.0f), gBoneTransforms[vin.BoneIndices[i]]).xyz;
        normalL += weights[i] * mul(vin.NormalL, (float3x3)gBoneTransforms[vin.BoneIndices[i]]);
        tangentL += weights[i] * mul(vin.TangentU.xyz, (float3x3) gBoneTransforms[vin.BoneIndices[i]]);
    }

    vin.PosL = posL;
    vin.NormalL = normalL;
    vin.TangentU.xyz = tangentL;
#endif
    
    if (instData.useDisplacementMap)
    {
	    // Sample the displacement map using non-transformed [0,1]^2 tex-coords.
        vin.PosL.y += gDisplacementMap[displacementIndex].SampleLevel(gsamLinearWrap, vin.TexC, 1.0f).r;
	    
	    // Estimate normal using finite difference.
        float du = displacementMapTexelSize.x;
        float dv = displacementMapTexelSize.y;
        float l = gDisplacementMap[displacementIndex].SampleLevel(gsamPointClamp, vin.TexC - float2(du, 0.0f), 0.0f).r;
        float r = gDisplacementMap[displacementIndex].SampleLevel(gsamPointClamp, vin.TexC + float2(du, 0.0f), 0.0f).r;
        float t = gDisplacementMap[displacementIndex].SampleLevel(gsamPointClamp, vin.TexC - float2(0.0f, dv), 0.0f).r;
        float b = gDisplacementMap[displacementIndex].SampleLevel(gsamPointClamp, vin.TexC + float2(0.0f, dv), 0.0f).r;
        vin.NormalL = normalize(float3(-r + l, 2.0f * gridSpatialStep, b - t));
    }
    
    // Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), world);
    vout.PosW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul(vin.NormalL, (float3x3) worldInvTranspose);
    
    vout.TangentW = mul(vin.TangentU, (float3x3) worldInvTranspose);

    // Transform to homogeneous clip space.
    vout.PosH = mul(posW, gViewProj);
	
	// Output vertex attributes for interpolation across triangle.
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), texTransform);
    vout.TexC = mul(texC, matData.MatTransform).xy;
	
    return vout;
}

struct PixelOut
{
    float4 color0 : SV_Target0;
    float4 color1 : SV_Target1;
};

PixelOut PS(VertexOut pin)
{
    // Fetch the material data.
    MaterialData matData = gMaterialData[pin.MatIndex];
    
    float3 fresnelR0 = matData.FresnelR0;
    float roughness = matData.Roughness;
    int diffuseTexIndex = matData.DiffMapIndex;
    int normalMapIndex = matData.NormMapIndex;

    float4 diffuseAlbedo = matData.useAlbedoMap ? gDiffuseMap[diffuseTexIndex].Sample(gsamLinearWrap, pin.TexC) * matData.DiffuseAlbedo
                                                : matData.DiffuseAlbedo;
	
#ifdef ALPHA_TEST
    clip(diffuseAlbedo.a - 0.1f);
#else
    if(matData.useAlphaTest)
 	    clip(diffuseAlbedo.a - 0.1f);    
#endif


    // Interpolating normal can unnormalize it, so renormalize it.
    pin.NormalW = normalize(pin.NormalW);
    float shininessCoeff = 1.0f;
    float3 bumpedNormalW = pin.NormalW;
    if(matData.useNormalMap)
    {
        float4 normalMapSample = gNormalMap[normalMapIndex].Sample(gsamAnisotropicWrap, pin.TexC);
        bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, pin.NormalW, pin.TangentW);
        
        shininessCoeff = normalMapSample.a;
    }
    
    // Vector from point being lit to eye. 
    float3 toEyeW = gEyePosW - pin.PosW;
    float distToEye = length(toEyeW);
    toEyeW /= distToEye; // 정규화

    // Light terms.
    float4 ambient = gAmbientLight * diffuseAlbedo;

    const float shininess = (1.0f - roughness) * shininessCoeff;
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
    float shadowFactor[NUM_DIR_LIGHTS];
    
    [unroll]
    for (int i = 0; i < NUM_DIR_LIGHTS; ++i)
    {
        float4 ShadowPosH = mul(float4(pin.PosW, 1.0f), gLights[i].shadowTransform);
        shadowFactor[i] = CalcShadowFactor(ShadowPosH, i);
    }
    
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        bumpedNormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

    // Add in specular reflections.
    float3 r = reflect(-toEyeW, bumpedNormalW);
    float4 reflectionColor = gCubeMap[gCubeMapIndex].Sample(gsamLinearWrap, r);
    float3 fresnelFactor = SchlickFresnel(fresnelR0, bumpedNormalW, r);
    litColor.rgb += shininess * fresnelFactor * reflectionColor.rgb;
    
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