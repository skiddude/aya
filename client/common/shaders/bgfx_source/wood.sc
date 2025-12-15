$input a_position, a_normal, a_texcoord0, a_texcoord1, a_color0, a_color1, a_texcoord2, a_texcoord3
$output v_pos, v_worldPos, v_texcoord0, v_texcoord1, v_color0, v_normal, v_tangent, v_lightpos_fog, v_view_depth, v_poslightspace, v_edgedist, v_reflection

#include "common.sh"

#ifdef CLASSIC
#define PIN_SURFACE
#define ALREADY_HAS_PS

#define SPEC_EXPON 4.0
#define KS 0.1

#define WoodContrast vec3(0.1, 0.1, 0.1)
#define Ks1 0.32
#define Ks2 0.16
#define RingScale 4.0
#define AmpScale 1.0
#define LightRingEnd 0.4
#define DarkRingStart 0.8
#define DarkRingEnd 0.83
#define NormMapScale 0.6
#define NoiseScale 64.0
#define MixedColorRatio 0.315 
#define AAFreqMultiplier 12.0

SAMPLER2D(s_normalMap, 6);
SAMPLER3D(s_specularMap, 7);
SAMPLER2D(s_lightMap, 1);
SAMPLER2D(s_lightMapLookup, 2);
SAMPLER2D(s_shadowMap, 3);

vec3 CalcBevel(vec4 edgeDistances, vec3 normal, float viewDepth);
void Shade(vec3 albedo, vec3 nn, vec3 vn, float ks, float specExpon, vec4 light, out vec3 diffuse, out vec3 specular);

void DefaultPS()
{
    vec3 nn = normalize(v_normal);

    mat3 normalMatrix = mat3(
        v_tangent.xyz,
        cross(nn, v_tangent.xyz),
        nn
    );

    vec4 WoodPos = vec4(v_pos.xyz, mix(0.0, 0.92, abs(dot(vec3(1.0, 0.0, 0.0), nn))));

    vec2 NormalUV = v_texcoord0.xy * NormMapScale * 3.0;
    float singularityAttenuation = WoodPos.w;
    
    float noiseval = texture3D(s_specularMap, WoodPos.xyz / NoiseScale).x;
    vec3 tn = texture2D(s_normalMap, NormalUV).xyz - 0.5;
    tn = mix(tn, vec3(0.0, 0.0, 0.5), singularityAttenuation);
    vec4 edgeDistances = v_edgedist;
    tn = CalcBevel(edgeDistances, tn, v_view_depth.w);
    
    nn = mul(tn, normalMatrix);

    float signalfreq = length(vec4(dFdx(WoodPos.y), dFdx(WoodPos.z), dFdy(WoodPos.y), dFdy(WoodPos.z)));
    float aa_attn = clamp(signalfreq * AAFreqMultiplier - 1.0, 0.0, 1.0);
    
    vec3 Pwood = WoodPos.xyz + (AmpScale * noiseval);
    float r = RingScale * length(Pwood.yz);
    r = r + texture3D(s_specularMap, vec3(r) / 32.0).x;
    r = r - floor(r);
    r = smoothstep(LightRingEnd, DarkRingStart, r) - smoothstep(DarkRingEnd, 1.0, r);
    r = mix(r, MixedColorRatio, aa_attn);
    
    vec3 albedo = v_color0.xyz + WoodContrast * (MixedColorRatio - r);
    float Ks = mix(Ks1, Ks2, r);
    
    vec3 vn = normalize(v_view_depth.xyz);
    vec4 light = lgridSample(s_lightMap, s_lightMapLookup, v_lightpos_fog.xyz);
    light.a = shadowSample(s_shadowMap, v_poslightspace, light.a);

    vec3 diffuse;
    vec3 specular;
    Shade(albedo, nn, vn, Ks, SPEC_EXPON, light, diffuse, specular);
    gl_FragColor = vec4(diffuse + specular, v_color0.w);
    
    float fogAlpha = clamp((u_fogParams.z - length(v_view_depth.xyz)) * u_fogParams.w, 0.0, 1.0);
    gl_FragColor.xyz = mix(u_fogColor, gl_FragColor.rgb, fogAlpha);

#ifdef PIN_GBUFFER
    // gl_FragData[1] = gbufferPack(v_view_depth.w, diffuse.rgb, specular.rgb, fogAlpha);
#endif
}

#else
#define CFG_TEXTURE_TILING 1.0

#define CFG_DIFFUSE_SCALE 1.0
#define CFG_SPECULAR_SCALE 2.0
#define CFG_GLOSS_SCALE 256.0
#define CFG_REFLECTION_SCALE 0.0

#define CFG_NORMAL_SHADOW_SCALE 0.3

#define CFG_SPECULAR_LOD 0.25
#define CFG_GLOSS_LOD 32.0

#define CFG_NORMAL_DETAIL_TILING 7.0
#define CFG_NORMAL_DETAIL_SCALE 0.6

#define CFG_FAR_TILING 0.0
#define CFG_FAR_DIFFUSE_CUTOFF 0.0
#define CFG_FAR_NORMAL_CUTOFF 0.0
#define CFG_FAR_SPECULAR_CUTOFF 0.0

#include "material.sc"
#endif
