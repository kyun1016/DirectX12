#include "Memory.hlsli"

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
    
    // �� ���� ������ ���
    m[0].PosL = 0.5f * (inVerts[0].PosL + inVerts[1].PosL);
    m[1].PosL = 0.5f * (inVerts[1].PosL + inVerts[2].PosL);
    m[2].PosL = 0.5f * (inVerts[2].PosL + inVerts[0].PosL);
    
    // // ���� ���� ����
    // m[0].PosL = normalize(m[0].PosL);
    // m[1].PosL = normalize(m[1].PosL);
    // m[2].PosL = normalize(m[2].PosL);
    
    // ������ ����
    m[0].NormalL = m[0].PosL;
    m[1].NormalL = m[1].PosL;
    m[2].NormalL = m[2].PosL;
    
    // �ؽ�ó ��ǥ�� ����
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
    // Fetch the material data.
    MaterialData matData = gMaterialData[gMaterialIndex];
    
    SubdivisionGeometryOut gout[6];
    
    [unroll]
    for (int i = 0; i < 6; ++i)
    {
        // world space�� ��ȯ
        float4 posW = mul(float4(v[i].PosL, 1.0f), gWorld);
        gout[i].PosW = posW.xyz;
        gout[i].NormalW = mul(v[i].NormalL, (float3x3) gWorld);
        
        // homogeneous clip space�� ��ȯ
        gout[i].PosH = mul(posW, gViewProj);
        
        float4 texC = mul(float4(v[i].TexC, 0.0f, 1.0f), gTexTransform);
        gout[i].TexC = mul(texC, matData.MatTransform).xy;
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
    
    // �ﰢ���� 2���� ��� �׸���.
    // 1. �� 1: �Ʒ��� �ﰢ�� �� ��
    // 2. �� 2: ���� �ﰢ�� 1��
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