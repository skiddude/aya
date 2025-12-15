$input a_position, a_normal, a_texcoord0, a_texcoord1, a_color0, a_color1, a_texcoord2, a_texcoord3
$output v_pos, v_worldPos, v_texcoord0, v_texcoord1, v_color0, v_normal, v_tangent, v_lightpos_fog, v_view_depth, v_poslightspace, v_edgedist, v_reflection

#include "common.sh"

#ifdef CLASSIC
#ifdef PIN_HQ
#define PIN_SURFACE
#define ALREADY_HAS_PS

#define SPEC_EXPON 81.0
#define KS 0.75

#define FADE_DIST 500.0
#define FADE_DIST_START 0.0

#define BEVEL_WIDTH 0.06

SAMPLER2D(s_diffuseMap, 5);
SAMPLER2D(s_normalMap, 6);
SAMPLER2D(s_lightMap, 1);
SAMPLER2D(s_lightMapLookup, 2);
SAMPLERCUBE(s_environmentMap, 4);

#ifndef GLSLES
SAMPLER2D(s_normalDetailMap, 8);
#endif

uniform vec4 u_waveParams; // .x = frequency  .y = phase  .z = height  .w = lerp

vec3 CalcBevelDir(vec4 edgeDistances)
{
    vec3 dir = vec3(0.0);
    vec4 bevelMultiplier = step(edgeDistances, vec4(BEVEL_WIDTH));

    dir += bevelMultiplier.x * vec3(1.0, 0.0, 0.0);
    dir += bevelMultiplier.y * vec3(-1.0, 0.0, 0.0);
    dir += bevelMultiplier.z * vec3(0.0, 1.0, 0.0);
    dir += bevelMultiplier.w * vec3(0.0, -1.0, 0.0);

    return dir;
}

vec3 CalcBevel(vec4 edgeDistances, vec3 normal, float viewDepth);

void Shade(vec3 albedo, vec3 nn, vec3 vn, float ks, float specExpon, vec4 light, out vec3 diffuse, out vec3 specular);

void DefaultPS()
{
    float normalStrength = 0.4;
    float fade = clamp(normalStrength - (v_view_depth.w - FADE_DIST_START) / FADE_DIST, 0.0, 1.0);
    vec3 nn = normalize(v_normal);
    vec4 edgeDistances = v_edgedist;

    vec4 albedo = v_color0;
    vec2 uv;

    float wt = 1.0;
#ifndef PIN_MESH
    uv = v_texcoord1; // UvStuds
    
    mat3 normalMatrix = mat3(
        v_tangent.xyz,
        cross(nn, v_tangent.xyz),
        nn
    );
    
    vec3 tn = vec3(0.0, 0.0, 0.5);
    tn = mix(vec3(0.0, 0.0, 0.5), tn, fade);
    tn = CalcBevel(edgeDistances, tn, v_view_depth.w);
    nn = mul(tn, normalMatrix);
    wt = 1.0 - abs(length(tn.xy));
    vec4 colorTex = texture2D(s_diffuseMap, uv);
#else
    uv = v_texcoord0;
    vec4 colorTex = texture2D(s_diffuseMap, uv);
    albedo *= colorTex;
#endif

    vec3 vn = normalize(v_view_depth.xyz);
    vec4 light = lgridSample(s_lightMap, s_lightMapLookup, v_lightpos_fog.xyz);

    vec3 diffuse;
    vec3 specular;

#ifdef CLASSIC_GOURAUD
    vec3 diffusePhong;
    vec3 specularPhong;

    Shade(albedo.rgb, nn, vn, KS, SPEC_EXPON, light, diffusePhong, specularPhong);
    diffuse = mix(diffusePhong, albedo.rgb, wt); // Simplified - should use stored diffuse
    specular = mix(specularPhong, vec3(0.0), wt); // Simplified - should use stored specular
#else
    Shade(albedo.rgb, nn, vn, KS, SPEC_EXPON, light, diffuse, specular);
#endif

#ifndef PIN_MESH
    diffuse = mix(diffuse, colorTex.xyz, colorTex.w);
#endif
    
    vec3 result = diffuse + specular;

#ifdef PIN_REFLECTION
    vec3 reflection = textureCube(s_environmentMap, reflect(-vn, nn)).rgb;
    result = mix(result, reflection, v_reflection);
#endif

    float fogAlpha = clamp((u_fogParams.z - length(v_view_depth.xyz)) * u_fogParams.w, 0.0, 1.0);
    result = mix(u_fogColor, result, fogAlpha);

    gl_FragColor = vec4(result, albedo.a);

#ifdef PIN_GBUFFER
    // gl_FragData[1] = gbufferPack(v_view_depth.w, diffuse.rgb, specular.rgb, fogAlpha);
#endif
}

#else
#define PIN_PLASTIC
#include "default.sc"
#endif

#else // !CLASSIC

#if defined(PIN_HQ)
#define PIN_SURFACE

#define CFG_TEXTURE_TILING 1.0

#define CFG_BUMP_INTENSITY 0.5

#define CFG_SPECULAR 0.4
#define CFG_GLOSS 9.0

#define CFG_NORMAL_SHADOW_SCALE 0.1

struct SurfaceInput
{
    vec4 Color;
    vec2 Uv;
    vec2 UvStuds;

#ifdef PIN_REFLECTION
    float Reflectance;
#endif
};

struct Surface
{
    vec3 albedo;
    vec3 normal;
    float specular;
    float gloss;
    float reflectance;
};

SAMPLER2D(s_diffuseMap, 5);
SAMPLER2D(s_normalMap, 6);

#ifndef GLSLES
SAMPLER2D(s_normalDetailMap, 8);
#endif

vec3 nmapUnpack(vec4 value);

Surface surfaceShader(SurfaceInput IN, vec2 fade2)
{
    float fade = fade2.y;
    
    vec4 studs = texture2D(s_diffuseMap, IN.UvStuds);
    vec3 normal = nmapUnpack(texture2D(s_normalMap, IN.UvStuds));

#ifdef GLSLES
    vec3 noise = vec3(0.0, 0.0, 1.0);
#else
    vec3 noise = nmapUnpack(texture2D(s_normalDetailMap, IN.Uv * CFG_TEXTURE_TILING));
#endif

    float noiseScale = saturate0(IN.Color.a * 2.0 * CFG_BUMP_INTENSITY - 1.0 * CFG_BUMP_INTENSITY);

#ifdef PIN_REFLECTION
    noiseScale *= clamp(1.0 - 2.0 * IN.Reflectance, 0.0, 1.0);
#endif

    normal.xy += noise.xy * noiseScale;
    normal.xy *= fade;

    Surface surface;
    surface.albedo = IN.Color.rgb * (studs.r * 2.0);
    surface.normal = normal;
    surface.specular = CFG_SPECULAR;
    surface.gloss = CFG_GLOSS;

#ifdef PIN_REFLECTION
    surface.reflectance = IN.Reflectance;
#endif

    return surface;
}

#include "default.sc"

#else
#define PIN_PLASTIC
#include "default.sc"
#endif

#endif // CLASSIC
