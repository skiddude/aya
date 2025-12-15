#define PIN_SURFACE
#include "default.sc"

#ifndef CFG_WANG_TILES
vec4 sampleFar(sampler2D s, vec2 uv, float fade, float cutoff)
{
#ifdef GLSLES
    return texture2D(s, uv);
#else
    if (cutoff == 0.0)
        return texture2D(s, uv);
    else
    {
        float cscale = 1.0 / (1.0 - cutoff);
        return mix(texture2D(s, uv * (CFG_FAR_TILING)), 
                   texture2D(s, uv), 
                   saturate0(fade * cscale - cutoff * cscale));
    }
#endif
}
#endif

SAMPLER2D(s_diffuseMap, 5);
SAMPLER2D(s_normalMap, 6);
SAMPLER2D(s_specularMap, 7);
SAMPLER2D(s_studsMap, 0);

#ifndef GLSLES
SAMPLER2D(s_normalDetailMap, 8);
#endif

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

vec3 nmapUnpack(vec4 value);

Surface surfaceShader(SurfaceInput IN, vec2 fade2)
{
#ifdef CFG_WANG_TILES
    vec2 wangUv;
    vec4 wangUVDerivatives;
    getWang(s_normalDetailMap, IN.Uv, CFG_TEXTURE_TILING, wangUv, wangUVDerivatives);
#endif

    vec2 uv = IN.Uv * (CFG_TEXTURE_TILING);
    
    float fadeDiffuse = fade2.x;
    float fade = fade2.y;

#ifdef CFG_OPT_DIFFUSE_CONST
    vec4 diffuse = vec4(1.0);
#else 
 
    #ifdef CFG_WANG_TILES
        vec4 diffuse = sampleWang(s_diffuseMap, wangUv, wangUVDerivatives);
    #else
        vec4 diffuse = sampleFar(s_diffuseMap, uv, fadeDiffuse, CFG_FAR_DIFFUSE_CUTOFF);
    #endif
   
   diffuse.rgba = diffuse.rgba * (CFG_DIFFUSE_SCALE);

#endif

#ifdef CFG_OPT_NORMAL_CONST
    vec3 normal = vec3(0.0, 0.0, 1.0);
#else
    #ifdef CFG_WANG_TILES
        vec3 normal = nmapUnpack(sampleWang(s_normalMap, wangUv, wangUVDerivatives));
    #else
        vec3 normal = nmapUnpack(sampleFar(s_normalMap, uv, fade, CFG_FAR_NORMAL_CUTOFF));
    #endif
#endif

#ifndef GLSLES
    #ifndef CFG_WANG_TILES // normal detail unavailable when running wang tiles
        vec3 normalDetail = nmapUnpack(texture2D(s_normalDetailMap, uv * (CFG_NORMAL_DETAIL_TILING)));
        normal.xy += normalDetail.xy * (CFG_NORMAL_DETAIL_SCALE);
    #endif
#endif

    normal.xy *= fade;

    float shadowFactor = 1.0 + normal.x * (CFG_NORMAL_SHADOW_SCALE);

#ifdef CFG_OPT_BLEND_COLOR
    vec3 albedo = mix(vec3(1.0), IN.Color.rgb, diffuse.a) * diffuse.rgb * shadowFactor;
#else
    vec3 albedo = IN.Color.rgb * diffuse.rgb * shadowFactor;
#endif

#ifndef GLSLES
    vec4 studs = texture2D(s_studsMap, IN.UvStuds);
    albedo *= studs.r * 2.0;
#endif

#ifdef CFG_WANG_TILES
    vec2 specular = sampleWang(s_specularMap, wangUv, wangUVDerivatives).rg;
#else
    vec2 specular = sampleFar(s_specularMap, uv, fade, CFG_FAR_SPECULAR_CUTOFF).rg;
#endif

    // make sure glossiness is never 0 to avoid fp specials
    vec2 specbase = specular * vec2(CFG_SPECULAR_SCALE, CFG_GLOSS_SCALE) + vec2(0.0, 0.01);
    vec2 specfade = mix(vec2(CFG_SPECULAR_LOD, CFG_GLOSS_LOD), specbase, fade);

    Surface surface;
    surface.albedo = albedo;
    surface.normal = normal;
    surface.specular = specfade.r;
    surface.gloss = specfade.g;
    surface.reflectance = specular.g * fade * (CFG_REFLECTION_SCALE);

    return surface;
}
