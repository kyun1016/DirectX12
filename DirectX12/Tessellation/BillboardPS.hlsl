#include "BillboardCommon.hlsli"

#define ALPHA_TEST
#define FOG

PixelOut main(BillboardGeometryOut pin) : SV_TARGET
{
    float3 uvw = float3(pin.TexC, pin.PrimID % 4);
    float4 diffuseAlbedo = gTreeMapArray.Sample(gsamAnisotropicWrap, uvw) * gDiffuseAlbedo;
    
#ifdef ALPHA_TEST
	// Discard pixel if texture alpha < 0.1.  We do this test as soon 
	// as possible in the shader so that we can potentially exit the
	// shader early, thereby skipping the rest of the shader code.
	clip(diffuseAlbedo.a - 0.1f);
#endif
	
	// ������ ������ �ٽ� ����ȭ
    pin.NormalW = normalize(pin.NormalW);
	
	// ����Ǵ� ������ �������� ����
    float3 toEyeW = gEyePosW - pin.PosW;
    float distToEye = length(toEyeW);
    toEyeW /= distToEye; // ����ȭ
	
	// ���� ��꿡 ���ԵǴ� �׵�.
    float4 ambient = gAmbientLight * diffuseAlbedo;
	
    const float shininess = 1.0f - gRoughness;
    Material mat = { diffuseAlbedo, gFresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW, pin.NormalW, toEyeW, shadowFactor);
    
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