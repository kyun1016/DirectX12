#include "Common.hlsli"

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
void main(triangle VertexIn gin[3], inout TriangleStream<SubdivisionGeometryOut> triStream)
{
    VertexIn v[6];
    Subdivide(gin, v);
    OutputSubdivision(v, triStream);
}