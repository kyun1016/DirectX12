// https://registry.khronos.org/OpenGL-Refpages/gl4/html/mix.xhtml
// https://anteru.net/blog/2016/mapping-between-HLSL-and-GLSL/
// https://www.shadertoy.com/view/7stfzB
#include "ST_Core.hlsli"

// contains a few field transforms for artistic purposes
// the metallic ratio transform is the one that looks like a magnetic field
// see this for a better explanation: https://www.shadertoy.com/view/ssGczz
// phi is often used to refer to the golden ratio which is the first metallic ratio
// hence the name of this shader
// also, here's a remake: https://www.shadertoy.com/view/X3VBR3

#define SCALE 8.0
#define CS(a) float2(cos(a), sin(a))
#define PT(u,r) smoothstep(0.0, r, r-length(u))


static const float3 gold = float3(1.0, 0.6, 0.0);
static const float3 blue = float3(0.3, 0.5, 0.9);
static const float w = 0.1; // line width
static const float d = 0.4; // shadow depth


// gradient map ( color, equation, time, width, shadow, reciprocal )
float3 gm(float3 c, float n, float t, float w, float d, bool i)
{
    float g = min(abs(n), 1.0 / abs(n));
    float s = abs(sin(n * PI - t));
    if (i)
        s = min(s, abs(sin(PI / n + t)));
    return (1.0 - pow(abs(s), w)) * c * pow(g, d) * 6.0;
}

// denominator spiral, use 1/n for numerator
// ( screen xy, spiral exponent, decimal, line width, hardness, rotation )
float ds(float2 u, float e, float n, float w, float h, float ro)
{
    float ur = length(u); // unit radius
    float sr = pow(ur, e); // spiral radius
    float a = round(sr) * n * TAU; // arc
    float2 xy = CS(a + ro) * ur; // xy coords
    float l = PT(u - xy, w); // line
    float s = mod(sr + 0.5, 1.0); // gradient smooth
    s = min(s, 1.0 - s); // darken filter
    return l * s * h;
}

float4 PS(PixelShaderInput input) : SV_TARGET
{
    float4 m = iMouse;
    m = float4(0.0, 0.0, 0.0, 0.0);
    float2 uv0 = 1. - 2. * input.texcoord;
    float t = iTime / PI * 2.0;
    
    float z = 1.0; // zoom (+)
    float e = 1.0; // screen exponent
    float se = 1.0; // spiral exponent
    float aa = 3.0; // anti-aliasing

    float3 bg = float3(0.0, 0.0, 0.0); // black background
    
    if(m.z > 0.0)
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
        float2 uv = (uv0 + o/iResolution.y) * SCALE * z; // scale uv
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
        c += rgb * ds(uv, se, t /TAU, px * 2.0, 2.0, 0.0); // spiral 1a
        c += rgb * ds(uv, se, t /TAU, px * 2.0, 2.0, PI); // spiral 1b
        c += rgb * ds(uv, -se, t /TAU, px * 2.0, 2.0, 0.0); // spiral 2a
        c += rgb * ds(uv, -se, t /TAU, px * 2.0, 2.0, PI); // spiral 2b
        c = max(c, 0.0); // clear negative color
        c += pow(max(1.0 - l, 0.0), 3.0 / z); // center glow
        
        if (m.z > 0.0)  // display grid on click
        {
            float2 xyg = abs(frac(uv0 + 0.5) - 0.5) / px;   // xy grid
            c.gb += 0.2 * (1.0 - min(min(xyg.x, xyg.y), 1.0)); // grid color)
        }
        bg += c; // add color to background
    }
        
    bg /= aa * aa;
    bg *= sqrt(bg) * 1.5;
    
    return float4(bg, 1.0);
}