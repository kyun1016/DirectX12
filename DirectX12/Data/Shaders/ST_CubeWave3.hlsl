// https://registry.khronos.org/OpenGL-Refpages/gl4/html/mix.xhtml
// https://anteru.net/blog/2016/mapping-between-HLSL-and-GLSL/
// https://www.shadertoy.com/view/lcSGDD
// https://www.shadertoy.com/view/McS3DW
#include "memory.hlsli"

// CC0: More glow experiments on a sunday
// More tinkering

#define TIME        iTime
#define RESOLUTION  iResolution
#define ROT(a)      float2x2(cos(a), sin(a), -sin(a), cos(a))

static const float PI = acos(-1.);
static const float TAU = 2. * PI;

float mod(float x, float y)
{
    return x - y * floor(x / y);
}

float2 mod(float2 x, float y)
{
    return x - y * floor(x / y);
}

float2 mod(float2 x, float2 y)
{
    // float2 ret;
    // ret.x = x.x - y.x * floor(x.x / y.x);
    // ret.y = x.y - y.y * floor(x.y / y.y);
    
    return x - y * floor(x / y);
}

float segment(float2 p, float2 a, float2 b)
{
    p -= a;
    b -= a;
    return length(p - b * clamp(dot(p, b) / dot(b, b), 0.0, 1.0));
}

#define rot(a) float2x2(cos(a +float4(0.0, 1.57, -1.57, 0.0)))
#define L(A,B) O += smoothstep(15./R.y, 0., segment( U-X, T(A), T(B) ) ) // *hue(.2*(J.y+.5*J.x))

float2 T(float3 p, float t)
{
    p.xy = mul(p.xy, rot(-t));
    p.xz = mul(p.xz, rot(.785));
    p.yz = mul(p.yz, rot(-.625));
    
    return p.xy;
}

struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 PS(PixelShaderInput input) : SV_TARGET
{
    float4 ret = float4(0., 0., 0., 1.);
    
    float2 R = iResolution.xy;
    // input.texcoord.y = 1. - input.texcoord.y;
    float2 U = 10. * (2. * input.texcoord - 1.);
    float2 M = float2(2., 2.3);
    float2 I = floor(U / M) * M;
    
    float2 X;
    float2 J;
    
    U = mod(U, M);
    
    for (uint k = 0; k < 4; ++k)
    {
        X = float2(k % 2, k / 2) * M;
        J = I + X;
        // int2 temp = int2((int) (J.x / M.x), (int) (J.y / M.y)) % 2;
        // if (int(J / M) % 2 > 0)
        // if(temp.x > 0 || temp.y > 0)
        //     X.y += 1.15;
        float t = tanh(-.2 * (J.x + J.y) + mod(2. * iTime, 10.) - 1.6) * .785;
        for (float a = 0.0; a < 6.; a += 1.57)  // draw cude
        {
            float3 A = float3(cos(a), sin(a), .7);
            float3 B = float3(-A.y, A.x, .7);
            
            ret += smoothstep(15. / R.y, 0., segment(U - X, T(A, t), T(B, t)));
            ret += smoothstep(15. / R.y, 0., segment(U - X, T(A, t), T(A * float3(1.,1.,-1.), t)));
            A.z = -A.z;
            B.z = -B.z;
            ret += smoothstep(15. / R.y, 0., segment(U - X, T(A, t), T(B, t)));
        }
    }
    return ret;
}