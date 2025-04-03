// https://registry.khronos.org/OpenGL-Refpages/gl4/html/mix.xhtml
// https://anteru.net/blog/2016/mapping-between-HLSL-and-GLSL/
// https://www.shadertoy.com/view/7stfzB
#include "ST_Core.hlsli"

// https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-asint
#define FBI asint
#define FFBI(a) FBI(cos(a))^FBI(a)


struct Grid
{
    float3 id;
    float d;
} gr;

// int floatBitsToInt(float a)
// {
//     //Nan
//     if (a != a)
//         return 0x7fc00000;
// 
//     //-0
//     if (a == 0.0)
//         return (1.0 / a == -1.0 / 0.0) ? 0x80000000 : 0;
// 
//     bool neg = false;
//     if (a < 0.0)
//     {
//         neg = true;
//         a = -a;
//     }
// 
//     if (isinf(a))
//     {
//         return neg ? 0xff800000 : 0x7f800000;
//     }
// 
//     int exp = ((a >> 52) & 0x7ff) - 1023;
//     int mantissa = (a & 0xffffffff) >> 29;
//     if (exp <= -127)
//     {
//         mantissa = (0x800000 | mantissa) >> (-127 - exp + 1);
//         exp = -127;
//     }
//     int bits = negative ? 2147483648 : 0;
//     bits |= (exp + 127) << 23;
//     bits |= mantissa;
// 
//     return bits;
// }

float4 PS(PixelShaderInput input) : SV_TARGET
{
    floatBitsToInt(0.0);
    float4 m = iMouse;
    m = float4(0.0, 0.0, 0.0, 0.0);
    float2 uv0 = 1. - 2. * input.texcoord;
    float t = iTime / PI * 2.0;
    
    float z = 1.0; // zoom (+)
    float e = 1.0; // screen exponent
    float se = 1.0; // spiral exponent
    float aa = 3.0; // anti-aliasing

    float3 bg = float3(0.0, 0.0, 0.0); // black background
    
    if (m.z > 0.0)
    {
        t += m.y * 1.0; // move time with mouse y
        z = pow(1.0 - abs(m.y), sign(m.y));
        e = pow(1.0 - abs(m.x), sign(m.x));
        se = e * -sign(m.y);
        uv0 = exp(log(abs(uv0)) * e) * sign(uv0);
    }

    for (float i = 0.0; i < aa; i++)    
        for (float j = 0.0; j < aa; j++)
        {
            float3 c = float3(0.0, 0.0, 0.0);
            float2 o = float2(i, j) / aa;
        // vec2 uv = (XY - 0.5 * R + o) / R.y * SCALE * z; // apply cartesian, scale and zoom
            float2 uv = (uv0 + o / iResolution.y) * SCALE * z; // scale uv
            if (m.z > 0.0)
            {
                uv = exp(log(abs(uv)) * e) * sign(uv0);
            }
        
            float px = length(fwidth(uv)); // pixel width
            float l = length(uv); // hypot of xy: sqrt(x*x + y*y)
            
            float mc = (uv.x * uv.x + uv.y * uv.y - 1.0) / uv.y; // metallic circle at xy
            float g = min(abs(mc), 1.0 / abs(mc)); // metallic gradient
            float3 goldCol = gold * g * l; // gold color
            float3 blueCol = blue * (1.0 - g); // blue color
            float3 rgb = max(goldCol, blueCol); // max of gold and blue
            
            c = max(c, gm(rgb, mc, -t, w, d, false)); // metalic clolr
            c = max(c, gm(rgb, abs(uv.y / uv.x) * sign(uv.y), -t, w, d, false)); // tangent
            c = max(c, gm(rgb, (uv.x * uv.x) / (uv.y * uv.y) * sign(uv.y), -t, w, d, false)); // sqrt cotangent
            c = max(c, gm(rgb, (uv.x * uv.x) + (uv.y * uv.y), t, w, d, true)); // sqrt circles
            c += rgb * ds(uv, se, t / TAU, px * 2.0, 2.0, 0.0); // spiral 1a
            c += rgb * ds(uv, se, t / TAU, px * 2.0, 2.0, PI); // spiral 1b
            c += rgb * ds(uv, -se, t / TAU, px * 2.0, 2.0, 0.0); // spiral 2a
            c += rgb * ds(uv, -se, t / TAU, px * 2.0, 2.0, PI); // spiral 2b
            c = max(c, 0.0); // clear negative color
            c += pow(max(1.0 - l, 0.0), 3.0 / z); // center glow
        
            if (m.z > 0.0)  // display grid on click
            {
                float2 xyg = abs(frac(uv0 + 0.5) - 0.5) / px; // xy grid
                c.gb += 0.2 * (1.0 - min(min(xyg.x, xyg.y), 1.0)); // grid color)
            }
            bg += c; // add color to background
        }
        
    bg /= aa * aa;
    bg *= sqrt(bg) * 1.5;
    
    return float4(bg, 1.0);
}