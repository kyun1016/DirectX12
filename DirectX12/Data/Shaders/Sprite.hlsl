#include "../../ECSCore/DX12_InstanceData.h"
#include "../../ECSCore/DX12_MeshData.h"
#include "../../ECSCore/DX12_LightData.h"

cbuffer cbPass : register(b0)
{
    float4x4 gViewProj;
};

cbuffer cbInstanceID : register(b1) { uint gBaseInstanceIndex; }
// cbuffer cbPass : register(b1)
// {
//     float4x4 gView;
//     float4x4 gInvView;
//     float4x4 gProj;
//     float4x4 gInvProj;
//     float4x4 gViewProj;
//     float4x4 gInvViewProj;
//     float3 gEyePosW;
//     float cbPerPassPad1;
//     float2 gRenderTargetSize;
//     float2 gInvRenderTargetSize;
//     float gNearZ;
//     float gFarZ;
//     float gTotalTime;
//     float gDeltaTime;
//     float4 gAmbientLight;

//     // Allow application to change fog parameters once per frame.
// 	// For example, we may only use fog for certain times of day.
//     float4 gFogColor;
//     float gFogStart;
//     float gFogRange;
//     float2 cbPerObjectPad2;
    
//     Light gLights[MAX_LIGHTS];
    
//     uint gCubeMapIndex;
// };
StructuredBuffer<InstanceData> gInstanceData : register(t0);

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
    vout.CenterW = vin.Position;
    vout.SizeW = vin.Size;
    vout.InstanceID = instanceID;

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
    // Get world space data from vertex and instance buffers.
    float3 centerW = mul(float4(gin[0].CenterW, 1.0f), gInstanceData[gin[0].InstanceID].World).xyz;
    float2 sizeW = gin[0].SizeW;

    // Compute the vertices of the quad facing the camera.
    // For simplicity, this creates a screen-aligned quad.
    // For a true 3D billboard, you'd use the camera's right and up vectors.
    float halfWidth = 0.5f * sizeW.x;
    float halfHeight = 0.5f * sizeW.y;

    float4 v[4];
    v[0] = float4(centerW + float3(-halfWidth, -halfHeight, 0.0f), 1.0f);
    v[1] = float4(centerW + float3(-halfWidth, +halfHeight, 0.0f), 1.0f);
    v[2] = float4(centerW + float3(+halfWidth, -halfHeight, 0.0f), 1.0f);
    v[3] = float4(centerW + float3(+halfWidth, +halfHeight, 0.0f), 1.0f);
    
    GeoOut gout;

    // Transform to clip space and output vertices
    gout.PosH = mul(v[1], gViewProj); // Upper-left
    gout.TexC = float2(0.0f, 0.0f);
    triStream.Append(gout);

    gout.PosH = mul(v[0], gViewProj); // Lower-left
    gout.TexC = float2(0.0f, 1.0f);
    triStream.Append(gout);

    gout.PosH = mul(v[3], gViewProj); // Upper-right
    gout.TexC = float2(1.0f, 0.0f);
    triStream.Append(gout);

    gout.PosH = mul(v[2], gViewProj); // Lower-right
    gout.TexC = float2(1.0f, 1.0f);
    triStream.Append(gout);
}

float4 PS(GeoOut pin) : SV_Target
{
    // For now, just return a solid color.
    // Later, you can sample a texture using pin.TexC.
    return float4(1.0f, 1.0f, 0.0f, 0.5f); // Semi-transparent yellow
}
