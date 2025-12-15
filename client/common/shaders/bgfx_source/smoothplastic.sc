$input a_position, a_normal, a_texcoord0, a_texcoord1, a_color0, a_color1, a_texcoord2, a_texcoord3
$output v_pos, v_worldPos, v_texcoord0, v_texcoord1, v_color0, v_normal, v_tangent, v_lightpos_fog, v_view_depth, v_poslightspace, v_edgedist, v_reflection

#include "common.sh"

#ifdef CLASSIC
#define PIN_SURFACE
#define ALREADY_HAS_PS

#define SPEC_EXPON 81.0
#define KS 0.75

#define FADE_DIST 500.0
#define FADE_DIST_START 0.0

#define BEVEL_WIDTH 0.06

SAMPLER2D(s_lightMap, 1);
SAMPLER2D(s_lightMapLookup, 2);
SAMPLERCUBE(s_environmentMap, 4);

vec3 CalcBevel(vec4 edgeDistances, vec3 normal, float viewDepth);
void Shade(vec3 albedo, vec3 nn, vec3 vn, float ks, float specExpon, vec4 light, out vec3 diffuse, out vec3 specular);

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

void DefaultPS()
{
    float normalStrength = 0.4;
    float fade = clamp(normalStrength - (v_view_depth.w - FADE_DIST_START) / FADE_DIST, 0.0, 1.0);
    vec3 nn = normalize(v_normal);
    vec4 edgeDistances = v_edgedist;

    vec4 albedo = v_color0;

    // lets generate this matrix in the vertex shader and pass it in later
    mat3 normalMatrix = mat3(
        v_tangent.xyz,
        cross(nn, v_tangent.xyz),
        nn
    );

    vec2 uv = v_texcoord0;
    vec3 tn = vec3(0.0, 0.0, 0.5);
    tn = mix(vec3(0.0, 0.0, 0.5), tn, fade);
    tn = CalcBevel(edgeDistances, tn, v_view_depth.w);
    nn = mul(tn, normalMatrix);

    vec3 vn = normalize(v_view_depth.xyz);
    vec4 light = lgridSample(s_lightMap, s_lightMapLookup, v_lightpos_fog.xyz);

    vec3 diffuse;
    vec3 specular;
    Shade(albedo.rgb, nn, vn, KS, SPEC_EXPON, light, diffuse, specular);
    
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
