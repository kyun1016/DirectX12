// https://registry.khronos.org/OpenGL-Refpages/gl4/html/mix.xhtml
// https://registry.khronos.org/OpenGL-Refpages/gl4/html/texelFetch.xhtml
// https://anteru.net/blog/2016/mapping-between-HLSL-and-GLSL/
// https://www.shadertoy.com/view/W3X3zl
#include "memory.hlsli"

// TODO: Modified This Logic!!
float mod(float x, float y)
{
    return x - y * floor(x / y);
}

float3 render(float2 F)
{
    // Setup ray origin & direction
    float2 u = (F + F - iResolution.xy) / iResolution.y;
    float2 a = texelFetch(iChannel1, ifloat2(0), 0).wz + .001;
    float3 ro = 17. * float3(sin(a.x) * cos(a.y), cos(a.x), sin(a.x) * sin(a.y)), r0 = ro;
    float3 rd = computeRayDirection(u, lookAt(ro));
    
    // Fetch current rotation state
    float4 state = texelFetch(iChannel1, ifloat2(1, 0), 0);
    float rot = state.z;
    float dir = state.w * 2. - 1.;
    
    float anim = dir * 1.571 * smoothstep(0., ANIM_DURATION, iTime - state.r);
    
    // Compute rotation matrix and axis
    float3x3 rotM;
    float3 axis;
    
    if (rot < 3.)
    {
        rotM = rotateX(anim);
        axis = float3(2, 0, 0);
    }
    else if (rot < 6.)
    {
        rotM = rotateY(anim);
        axis = float3(0, 2, 0);
    }
    else
    {
        rotM = rotateZ(anim);
        axis = float3(0, 0, 2);
    }
    
    // Compute slice offsets
    float3 size = 3. - axis;
    float3 ro1 = ro - axis * (mod(rot + 1., 3.) - 1.);
    float3 ro2 = ro - axis * (mod(rot + 2., 3.) - 1.);
    float3 roA = ro - axis * (mod(rot, 3.) - 1.); // Rotating slice
    
    // Compute intersection with each slice
    float3 roAnim = mul(rotM, roA);
    float3 rdAnim = mul(rotM, rd);
    float3 m = 1. / rd;
    float3 mA = 1. / rdAnim;
    float3 k = size * abs(m);
    float3 kA = size * abs(mA);
    
    float4 hit = boxIntersectOpti(ro1, m, k);
    float4 hit2 = boxIntersectOpti(ro2, m, k);
    float4 hitA = boxIntersectOpti(roAnim, mA, kA); // Rotating slice
    
    // Find closest intersection
    float i = 0.;
    ro = ro1;
    if (hit2.w < hit.w)
        i = 1., hit = hit2, ro = ro2;
    if (hitA.w < hit.w)
        i = -1., hit = hitA, ro = roAnim, rd = rdAnim;
    
    bool inside = true;
    if (hit.w > 1e2)
    {
        // Quick hack to get an "interesting" background
        hit = boxIntersectOpti(r0, m, abs(m) * 49.);
        inside = false;
    }
    
    // Compute world position and cell index
    float3 p = ro + rd * (hit.w + .01);
    ifloat3 pid = ifloat3(floor(p * .5 + .5) + axis * .5 * (mod(rot + i + 1., 3.) - 1.));
    int id = abs(pid.x + pid.y * 3 + pid.z * 9 + 13) % 27;
    
    // Fetch cell data
    float4 cell = texelFetch(iChannel0, ifloat2(id, 0), 0);
    float3 col;
    float2 uv;
    
    // Compute UVs and color depending on current side
    if (hit.x > hit.y && hit.x > hit.z)
    {
        uv = ro.zy + rd.zy * hit.x;
        uv.x *= sign(p.x);
        col = palette(cell.x);
    }
    else if (hit.y > hit.z)
    {
        uv = ro.xz + rd.xz * hit.y;
        uv.x *= sign(p.y);
        col = palette(cell.y);
    }
    else
    {
        uv = ro.xy + rd.xy * hit.z;
        uv.x *= -sign(p.z);
        col = palette(cell.z);
    }
    
    uv = fract(uv * .5 + .5);
    
    // Vignette
    float2 v = uv * (1. - uv.yx);
    col *= min(1., v.x * v.x * v.y * v.y) * 120.;
    
    // Skip lighting for background
    if (!inside)
        return col * .075;
        
    uv = uv * 2. - 1.;

    // Light up rotating slice
    float K = abs(iTime - state.r - ANIM_DURATION * .5) - ANIM_DURATION * .1;
    K = smoothstep(ANIM_DURATION * .4, 0., K + i + 1.);
    col *= 1. + K * 2.;

    // Compute normal, up and right
    float3 n = -sign(rd) * step(hit.yzx, hit.xyz) * step(hit.zxy, hit.xyz), n0 = n;
    float3 right = normalize(cross(n.xyz, float3(0, 1, 1e-4)));
    float3 up = normalize(cross(right, n.xyz));

    // Create fake gaps/depth between cells by playing with normal,
    // inspired by @Observer's shader (https://www.shadertoy.com/view/XtG3D1)
    float2 g = smoothstep(.8, 1., abs(uv)) * .5;
    float h = 1. - max(g.x, g.y);
    n = normalize(n * h + right * g.x * sign(uv.x) + up * g.y * sign(uv.y)); // Try removing this line!

    // Specular light
    float spec = pow(max(0., -dot(n, rd)), 200.);
    col += spec * smoothstep(.085, .0, dot(n, n0) - .93) * 2.;
    
    return col;
}
struct PixelShaderInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 PS(PixelShaderInput input) : SV_TARGET
{
    float3 col = float3(0);
    
    for (float i = 0.; i < AA; i++)
        for (float j = 0.; j < AA; j++)
        {
            col += render(F + float2(i, j) / AA - .5);
        }
    
    O.rgb = pow(col / (AA * AA), float3(.4545));
}