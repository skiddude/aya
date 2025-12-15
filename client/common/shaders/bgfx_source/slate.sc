$input a_position, a_normal, a_texcoord0, a_texcoord1, a_color0, a_color1, a_texcoord2, a_texcoord3
$output v_pos, v_worldPos, v_texcoord0, v_texcoord1, v_color0, v_normal, v_tangent, v_lightpos_fog, v_view_depth, v_poslightspace, v_edgedist, v_reflection

#include "common.sh"

#ifdef CLASSIC
#define PIN_SURFACE
#define ALREADY_HAS_PS

#define SPEC_EXPON 40.0
#define KS 0.1

#define NOISE_SCALE 7.0
#define SPREAD 0.3
#define GRASS_THRESHOLD 0.95

#define FADE_DIST 290.0
#define FADE_DIST_START 0.0

SAMPLER2D(s_normalMap, 6);
SAMPLER3D(s_specularMap, 7);
SAMPLER2D(s_lightMap, 1);
SAMPLER2D(s_lightMapLookup, 2);
SAMPLER2D(s_shadowMap, 3);

vec3 CalcBevel(vec4 edgeDistances, vec3 normal, float viewDepth);
void Shade(vec3 albedo, vec3 nn, vec3 vn, float ks, float specExpon, vec4 light, out vec3 diffuse, out vec3 specular);

void DefaultPS()
{
    vec3 normal = normalize(v_normal);

    mat3 normalMatrix = mat3(
        v_tangent.xyz,
        cross(normal, v_tangent.xyz),
        normal
    );

    float fade = clamp(1.0 - (v_view_depth.w - FADE_DIST_START) / FADE_DIST, 0.0, 1.0);

    vec2 uv = v_texcoord0;
    vec3 shiftPos = v_pos;

    float noiseval = texture3D(s_specularMap, shiftPos.xyz / NOISE_SCALE * 0.04).x;
    float noiseval2 = texture3D(s_specularMap, shiftPos.xyz / NOISE_SCALE * 0.3).x + 0.2;
    noiseval *= noiseval2;

    vec3 albedo = v_color0.xyz + 0.8 * fade * (noiseval * 0.5 - 0.1);

    vec3 tn = texture2D(s_normalMap, uv).xyz - 0.5;
    float tNormSum = 0.9 + 0.4 * (tn.x + tn.y + tn.z);
    tn = mix(tn, vec3(0.0, 0.0, 0.5), 0.9);
    albedo *= ((1.0 - fade) + (fade * tNormSum));

    tn = CalcBevel(v_edgedist, tn, v_view_depth.w);
    vec3 nn = mul(tn, normalMatrix);

    vec3 vn = normalize(v_view_depth.xyz);
    vec4 light = lgridSample(s_lightMap, s_lightMapLookup, v_lightpos_fog.xyz);
    light.a = shadowSample(s_shadowMap, v_poslightspace, light.a);

    vec3 diffuse;
    vec3 specular;
    Shade(albedo, nn, vn, KS, SPEC_EXPON, light, diffuse, specular);

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
#define CFG_SPECULAR_SCALE 0.9
#define CFG_GLOSS_SCALE 128.0
#define CFG_REFLECTION_SCALE 0.0

#define CFG_NORMAL_SHADOW_SCALE 0.5

#define CFG_SPECULAR_LOD 0.14
#define CFG_GLOSS_LOD 20.0

#define CFG_NORMAL_DETAIL_TILING 5.0
#define CFG_NORMAL_DETAIL_SCALE 1.0

#define CFG_FAR_TILING 0.25
#define CFG_FAR_DIFFUSE_CUTOFF 0.75
#define CFG_FAR_NORMAL_CUTOFF 0.0
#define CFG_FAR_SPECULAR_CUTOFF 0.0

#include "material.sc"
#endif
