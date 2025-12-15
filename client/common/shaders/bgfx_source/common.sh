#ifndef COMMON_SH_HEADER_GUARD
#define COMMON_SH_HEADER_GUARD

#include "globals.sh"
#include <bgfx_shader.sh>

// GLSLES has limited number of vertex shader registers so we have to use less bones
#if defined(GLSLES) && !defined(GL3)
#define MAX_BONE_COUNT 32
#else
#define MAX_BONE_COUNT 72
#endif

// PowerVR saturate() is compiled to min/max pair
// These are cross-platform specialized saturates that are free on PC and only cost 1 cycle on PowerVR
float saturate0(float v)
{
    return max(v, 0.0);
}

float saturate1(float v)
{
    return min(v, 1.0);
}

#if defined(GLSLES) && !defined(GL3)
#define WANG_SUBSET_SCALE 2.0
#else
#define WANG_SUBSET_SCALE 1.0
#endif

#define GBUFFER_MAX_DEPTH 500.0

// Texture samplers - bgfx style
#define TEX_DECLARE2D(name, reg) SAMPLER2D(name, reg)
#define TEX_DECLARE3D(name, reg) SAMPLER3D(name, reg)
#define TEX_DECLARECUBE(name, reg) SAMPLERCUBE(name, reg)

// Unified matrix uniforms
uniform mat4 u_worldMatrix;
uniform vec4 u_worldMatrixArray[MAX_BONE_COUNT * 3];

#if defined(GLSLES) || defined(PIN_WANG_FALLBACK)
#define TEXTURE_WANG(name) 0
void getWang(sampler2D unused, vec2 uv, float tiling, out vec2 wangUv, out vec4 wangUVDerivatives)
{
    wangUv = uv * WANG_SUBSET_SCALE;
    wangUVDerivatives = vec4(0.0, 0.0, 0.0, 0.0); // not used in this mode
}

vec4 sampleWang(sampler2D s, vec2 uv, vec4 wangUVDerivatives)
{
    return texture2D(s, uv);
}
#else
#define TEXTURE_WANG(name) name
void getWang(sampler2D s, vec2 uv, float tiling, out vec2 wangUv, out vec4 wangUVDerivatives)
{
#ifndef WIN_MOBILE
    float idxTexSize = 128.0;
#else
    float idxTexSize = 32.0;
#endif

    vec2 wangBase = uv * tiling * 4.0;

#if defined(DX11) && !defined(WIN_MOBILE)
    // compensate the precision problem of Point Sampling on some cards
    vec2 wangUV = (floor(wangBase) + 0.5) / idxTexSize;
#else
    vec2 wangUV = wangBase / idxTexSize;
#endif

#if defined(DX11) || defined(GL3)
    vec2 wang = texture2D(s, wangUV).rg;
#else
    vec2 wang = texture2D(s, wangUV).ba;
#endif

    wangUVDerivatives = vec4(dFdx(wangBase * 0.25), dFdy(wangBase * 0.25));

    wang *= 255.0 / 256.0;
    wangUv = wang + fract(wangBase) * 0.25;
}

vec4 sampleWang(sampler2D s, vec2 uv, vec4 derivates)
{
    return texture2DGrad(s, uv, derivates.xy, derivates.zw);
}
#endif

vec4 gbufferPack(float depth, vec3 diffuse, vec3 specular, float fog)
{
    depth = saturate(depth / GBUFFER_MAX_DEPTH);

    const vec3 lumVec = vec3(0.299, 0.587, 0.114);

    vec2 comp;
    comp = depth * vec2(255.0, 255.0 * 256.0);
    comp = fract(comp);
    comp = vec2(depth, comp.x * 256.0 / 255.0) - vec2(comp.x, comp.y) / 255.0;

    vec4 result;

    result.r = mix(1.0, dot(specular, lumVec), saturate(3.0 * fog));
    result.g = mix(0.0, dot(diffuse, lumVec), saturate(3.0 * fog));
    result.ba = comp.yx;

    return result;
}

vec3 lgridOffset(vec3 v, vec3 n)
{
    // cells are 4 studs in size
    // offset in normal direction to prevent self-occlusion
    // the offset has to be 1.5 cells in order to fully eliminate the influence of the source cell with trilinear filtering
    // (i.e. 1 cell is enough for point filtering, but is not enough for trilinear filtering)
    return v + n * (1.5 * 4.0);
}

vec3 lgridPrepareSample(vec3 c)
{
    // yxz swizzle is necessary for GLSLES sampling to work efficiently
    // (having .y as the first component allows to do the LUT lookup as a non-dependent texture fetch)
    return c.yxz * u_lightConfig0.xyz + u_lightConfig1.xyz;
}

#if defined(GLSLES) && !defined(GL3)
#define LGRID_SAMPLER(name, register) TEX_DECLARE2D(name, register)

vec4 lgridSample(sampler2D t, sampler2D lut, vec3 data)
{
    vec4 offsets = texture2D(lut, data.xy);

    // texture is 64 pixels high
    // let's compute slice lerp coeff
    float slicef = fract(data.x * 64.0);

    // texture has 64 slices with 8x8 atlas setup
    vec2 base = clamp(data.yz, vec2(0.0), vec2(1.0)) * 0.125;

    vec4 s0 = texture2D(t, base + offsets.xy);
    vec4 s1 = texture2D(t, base + offsets.zw);

    return mix(s0, s1, slicef);
}
#else
#define LGRID_SAMPLER(name, register) TEX_DECLARE3D(name, register)

vec4 lgridSample(sampler3D t, sampler2D lut, vec3 data)
{
    vec3 edge = step(u_lightConfig3.xyz, abs(data - u_lightConfig2.xyz));
    float edgef = saturate1(dot(edge, vec3(1.0)));

    // replace data with 0 on edges to minimize texture cache misses
    vec4 light = texture3D(t, data.yzx - data.yzx * edgef);

    return mix(light, u_lightBorder, vec4(edgef));
}
#endif

vec3 nmapUnpack(vec4 value)
{
#ifdef GLSLES
    return value.rgb * 2.0 - 1.0;
#else
    vec2 xy = value.ag * 2.0 - 1.0;
    return vec3(xy, sqrt(saturate(1.0 + dot(-xy, xy))));
#endif
}

vec3 terrainNormal(vec4 tnp0, vec4 tnp1, vec4 tnp2, vec3 w, vec3 normal, vec3 tsel)
{
    // Inspired by "Voxel-Based Terrain for Real-Time Virtual Simulations" [Lengyel2010] 5.5.2
    vec3 tangentTop = vec3(normal.y, -normal.x, 0.0);
    vec3 tangentSide = vec3(normal.z, 0.0, -normal.x);

    vec3 bitangentTop = vec3(0.0, -normal.z, normal.y);
    vec3 bitangentSide = vec3(0.0, -1.0, 0.0);

    // Blend pre-unpack to save cycles
    vec3 tn = nmapUnpack(tnp0 * w.x + tnp1 * w.y + tnp2 * w.z);

    // We blend all tangent frames together as a faster approximation to the correct world normal blend
    float tselw = dot(tsel, w);

    vec3 tangent = mix(tangentSide, tangentTop, tselw);
    vec3 bitangent = mix(bitangentSide, bitangentTop, tselw);

    return normalize(tangent * tn.x + bitangent * tn.y + normal * tn.z);
}

vec3 shadowPrepareSample(vec3 p)
{
    vec4 c = vec4(p, 1.0);

    return vec3(dot(u_shadowMatrix0, c), dot(u_shadowMatrix1, c), dot(u_shadowMatrix2, c));
}

float shadowDepth(vec3 lpos)
{
    return lpos.z;
}

float shadowStep(float d, float z)
{
    // saturate returns 1 for z in [0.1..0.9]; it fades to 0 as z approaches 0 or 1
    return step(d, z) * saturate(9.0 - 20.0 * abs(z - 0.5));
}

float shadowSample(sampler2D map, vec3 lpos, float lightShadow)
{
#ifdef CLASSIC
    return lightShadow;
#else
    vec2 smDepth = texture2D(map, lpos.xy).rg;
    float smShadow = shadowStep(smDepth.x, shadowDepth(lpos));

    return (1.0 - smShadow * smDepth.y * u_outlineBrightness_ShadowInfo.w) * lightShadow;
#endif
}

#endif // COMMON_SH_HEADER_GUARD
