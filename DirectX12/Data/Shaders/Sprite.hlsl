#include "../../ECSCore/InstanceData.h"
#include "../../ECSCore/MeshData.h"
#include "../../ECSCore/LightData.h"
#include "../../ECSCore/CameraData.h"

cbuffer cbInstanceID : register(b0) { uint gBaseInstanceIndex; }
StructuredBuffer<InstanceData> gInstanceData : register(t0, space0);
StructuredBuffer<CameraData> gCameraData : register(t1, space0);
StructuredBuffer<LightData> gLightData : register(t2, space0);

struct VertexOut
{
    float3 CenterW : POSITION;
    float2 SizeW   : SIZE;
    uint InstanceID : SV_InstanceID;
};

VertexOut VS(SpriteVertex vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout;

    // Pass data to the geometry shader.
    vout.InstanceID = gBaseInstanceIndex + instanceID;
    vout.CenterW = mul(float4(vin.Position, 1.0f), gInstanceData[vout.InstanceID].World).xyz;
    vout.SizeW = vin.Size * gInstanceData[vout.InstanceID].World._11_22;

    return vout;
}

struct GeoOut
{
    float4 PosH : SV_POSITION;
    float2 TexC : TEXCOORD;
};

[maxvertexcount(4)]
void GS(point VertexOut gin[1], inout TriangleStream<GeoOut> triStream)
{
    // Compute the vertices of the quad facing the camera.
    // For simplicity, this creates a screen-aligned quad.
    // For a true 3D billboard, you'd use the camera's right and up vectors.
    float halfWidth = 0.5f * gin[0].SizeW.x;
    float halfHeight = 0.5f * gin[0].SizeW.y;

    float4 pos[4] =
    {
        float4(gin[0].CenterW + float3(+halfWidth, -halfHeight, 0.0f), 1.0f),
        float4(gin[0].CenterW + float3(+halfWidth, +halfHeight, 0.0f), 1.0f),
        float4(gin[0].CenterW + float3(-halfWidth, -halfHeight, 0.0f), 1.0f),
        float4(gin[0].CenterW + float3(-halfWidth, +halfHeight, 0.0f), 1.0f)
    };
    
    // Triangle 1: v[0], v[1], v[2] (CCW)
    // Triangle 2: v[1], v[3], v[2] (CCW)
    float2 tex[4] =
    {
        float2(0.0f, 1.0f),
        float2(0.0f, 0.0f),
        float2(1.0f, 1.0f),
        float2(1.0f, 0.0f)
    };

    GeoOut gout;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        // gout.PosH = mul(pos[i], gCameraData[0].ViewProj);
        gout.PosH = pos[i];
        gout.TexC = tex[i];
        triStream.Append(gout);
    }
}

float4 PS(GeoOut pin) : SV_Target
{
    // For now, just return a solid color.
    // Later, you can sample a texture using pin.TexC.
    return float4(1.0f, 0.0f, 1.0f, 0.5f); // Semi-transparent yellow
}
