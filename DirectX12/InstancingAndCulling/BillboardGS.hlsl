#include "Memory.hlsli"

// Billboard
struct BillboardVertexIn
{
    float3 PosW : POSITION;
    float2 SizeW : SIZE;
};

struct BillboardGeometryOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
    uint PrimID : SV_PrimitiveID;
};

[maxvertexcount(4)]
void main(point BillboardVertexIn gin[1],
	uint primID : SV_PrimitiveID,
	inout TriangleStream<BillboardGeometryOut> triStream
)
{
    // World Space 기준 카메라 방향 좌표계 계산
    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 look = gEyePosW - gin[0].PosW;
    look.y = 0.0f;
    look = normalize(look);
    float3 right = cross(up, look);

    //  4개의 정점 계산
    float halfWidth = 0.5f * gin[0].SizeW.x;
    float halfHeight = 0.5f * gin[0].SizeW.y;
    
    float4 v[4];
    v[0] = float4(gin[0].PosW + halfWidth * right - halfHeight * up, 1.0f);
    v[1] = float4(gin[0].PosW + halfWidth * right + halfHeight * up, 1.0f);
    v[2] = float4(gin[0].PosW - halfWidth * right - halfHeight * up, 1.0f);
    v[3] = float4(gin[0].PosW - halfWidth * right + halfHeight * up, 1.0f);
    
    float2 texC[4] =
    {
        float2(0.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(1.0f, 0.0f)
    };
    
    BillboardGeometryOut gout;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        gout.PosH = mul(v[i], gViewProj);
        gout.PosW = v[i].xyz;
        gout.NormalW = look;
        gout.TexC = texC[i];
        gout.PrimID = primID;
        
        triStream.Append(gout);
    }

}