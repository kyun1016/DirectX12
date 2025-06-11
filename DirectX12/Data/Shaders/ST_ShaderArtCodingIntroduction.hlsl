// https://registry.khronos.org/OpenGL-Refpages/gl4/html/mix.xhtml
// https://anteru.net/blog/2016/mapping-between-HLSL-and-GLSL/
// https://www.shadertoy.com/view/mtyGWy

#include "memory.hlsli"

static const float REPEAT = 5.0;

//https://iquilezles.org/articles/palettes/
float3 palette(float t)
{
    float3 a = float3(0.5, 0.5, 0.5);
    float3 b = float3(0.5, 0.5, 0.5);
    float3 c = float3(1.0, 1.0, 1.0);
    float3 d = float3(0.263, 0.416, 0.557);

    return a + b * cos(6.28318 * (c * t + d));
}

struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 PS(PixelShaderInput input) : SV_TARGET
{
    // float2 p = (fragCoord.xy * 2. - iResolution.xy) / min(iResolution.x, iResolution.y);
    float2 uv = input.texcoord * 2 - 1.0;
    float2 uv0 = uv;
    float3 finalColor = float3(0.0, 0.0, 0.0);
    
    for (float i = 0.0; i < 4.0; i++)
    {
        uv = frac(uv * 1.5) - 0.5;

        float d = length(uv) * exp(-length(uv0));

        float3 col = palette(length(uv0) + i * .4 + iTime * .4);

        d = sin(d * 8. + iTime) / 8.;
        d = abs(d);

        d = pow(0.01 / d, 1.2);

        finalColor += col * d;
    }
        
    return float4(finalColor, 1.0);
}

/** SHADERDATA
{
	"title": "Octgrams",
	"description": "Lorem ipsum dolor",
	"model": "person"
}
*/