#include "Memory.hlsli"

static const float lineScale = 0.1;

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct NormalGeometryShaderInput
{
    float4 PosL : SV_POSITION;
    float3 NormalL : NORMAL;
};

NormalGeometryShaderInput main(VertexIn input)
{
    NormalGeometryShaderInput output;

    output.PosL = float4(input.PosL, 1.0);
    output.NormalL = input.NormalL;

    return output;
}