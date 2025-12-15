$input a_position, a_normal, a_texcoord0, a_texcoord1
$output v_pos, v_color0, v_color1, v_texcoord0, v_texcoord1, v_texcoord2, v_lightpos_fog, v_normal, v_view_depth, v_poslightspace

#include "common.sh"
#include "globals.sh"

// Tunables
#define CFG_TEXTURE_TILING 0.2
#define CFG_TEXTURE_DETILING 0.1

#define CFG_SPECULAR 2.0
#define CFG_GLOSS 900.0

#define CFG_NORMAL_STRENGTH 0.25

#define CFG_REFRACTION_STRENGTH 0.05

#define CFG_FRESNEL_OFFSET 0.3

#define CFG_SSR_STEPS 8
#define CFG_SSR_START_DISTANCE 1.0
#define CFG_SSR_STEP_CLAMP 0.2
#define CFG_SSR_DEPTH_CUTOFF 10.0

uniform vec4 u_waveParams; // .x = frequency  .y = phase  .z = height  .w = lerp
uniform vec4 u_waterColor; // deep water color
uniform vec4 u_waterParams; // .x = refraction depth scale, .y = refraction depth offset

SAMPLER2D(s_normalMap1, 0);
SAMPLER2D(s_normalMap2, 1);
SAMPLERCUBE(s_envMap, 2);
SAMPLER2D(s_lightMap, 3);
SAMPLER2D(s_lightMapLookup, 4);
SAMPLER2D(s_gbufferColor, 5);
SAMPLER2D(s_gbufferDepth, 6);

vec3 displacePosition(vec3 position, float waveFactor)
{
    float x = sin((position.z - position.x) * u_waveParams.x - u_waveParams.y);
    float z = sin((position.z + position.x) * u_waveParams.x + u_waveParams.y);
    float p = (x + z) * u_waveParams.z;

    vec3 result = position;
    
    result.y += p * waveFactor;

    return result;
}

vec4 clipToScreen(vec4 pos)
{
#ifdef GLSL
    pos.xy = pos.xy * 0.5 + 0.5 * pos.w;
#else
    pos.xy = pos.xy * vec2(0.5, -0.5) + 0.5 * pos.w;
#endif
    return pos;
}

vec2 getUV(vec3 position, int projection, float seed)
{
    vec3 u = u_worldMatrixArray[1 + projection].xyz;
    vec3 v = u_worldMatrixArray[19 + projection].xyz;

    vec2 uv = vec2(dot(position, u), dot(position, v)) * (0.25 * CFG_TEXTURE_TILING) + CFG_TEXTURE_DETILING * vec2(seed, floor(seed * 2.6651441));

    return uv;
}

void WaterVS()
{
    vec3 posWorld = a_position.xyz * u_worldMatrixArray[0].w + u_worldMatrixArray[0].xyz;
    vec3 normalWorld = a_normal.xyz * (1.0 / 127.0) - 1.0;

#if defined(GLSLES) && !defined(GL3) // iPad2 workaround
    vec3 weights = vec3(
        abs(a_position.w - 0.0) < 0.1 ? 1.0 : 0.0,
        abs(a_position.w - 1.0) < 0.1 ? 1.0 : 0.0,
        abs(a_position.w - 2.0) < 0.1 ? 1.0 : 0.0
    );
#else
    vec3 weights = vec3(
        a_position.w == 0.0 ? 1.0 : 0.0,
        a_position.w == 1.0 ? 1.0 : 0.0,
        a_position.w == 2.0 ? 1.0 : 0.0
    );
#endif

    float waveFactor = dot(weights, a_texcoord0.xyz) * (1.0 / 255.0);

#ifdef PIN_HQ
    float fade = saturate0(1.0 - dot(posWorld - u_cameraPosition, -u_viewDir.xyz) * u_fadeDistance_GlowFactor.y);

    posWorld = displacePosition(posWorld, waveFactor * fade);
#endif

    v_pos = mul(u_viewProjection, vec4(posWorld, 1.0));

    v_lightpos_fog = vec4(lgridPrepareSample(lgridOffset(posWorld, normalWorld)), (u_fogParams.z - v_pos.w) * u_fogParams.w);

    v_texcoord0.xy = getUV(posWorld, int(a_texcoord1.x), a_normal.w / 255.0);
    v_texcoord1.xy = getUV(posWorld, int(a_texcoord1.y), a_texcoord0.w / 255.0);
    v_texcoord2.xy = getUV(posWorld, int(a_texcoord1.z), a_texcoord1.w / 255.0);

    v_color0.xyz = weights;
    v_color0.w = waveFactor;

    v_normal = normalWorld;
    v_view_depth = vec4(u_cameraPosition - posWorld, v_pos.w);
    v_color1.xyz = vec3(
        a_texcoord1.x > 7.5 ? 1.0 : 0.0,
        a_texcoord1.y > 7.5 ? 1.0 : 0.0,
        a_texcoord1.z > 7.5 ? 1.0 : 0.0
    ); // side vs top

#ifdef PIN_HQ
    v_poslightspace = clipToScreen(v_pos).xyz;
#endif

    gl_Position = v_pos;
}

float fresnel(float ndotv)
{
    return saturate0(0.78 - 2.5 * abs(ndotv)) + CFG_FRESNEL_OFFSET;
}

vec4 sampleMix(vec2 uv)
{
#ifdef PIN_HQ
    return mix(texture2D(s_normalMap1, uv), texture2D(s_normalMap2, uv), u_waveParams.w);
#else
    return texture2D(s_normalMap1, uv);
#endif
}

vec3 sampleNormal(vec2 uv0, vec2 uv1, vec2 uv2, vec3 w, vec3 normal, vec3 tsel)
{
    return terrainNormal(sampleMix(uv0), sampleMix(uv1), sampleMix(uv2), w, normal, tsel);
}

vec3 sampleNormalSimple(vec2 uv0, vec2 uv1, vec2 uv2, vec3 w)
{
    vec4 data = sampleMix(uv0) * w.x + sampleMix(uv1) * w.y + sampleMix(uv2) * w.z;

    return nmapUnpack(data).xzy;
}

float unpackDepth(vec2 uv)
{
    vec4 geomTex = texture2D(s_gbufferDepth, uv);
    float d = geomTex.z * (1.0 / 256.0) + geomTex.w;
    return d * GBUFFER_MAX_DEPTH;
}

vec3 getRefractedColor(vec4 cpos, vec3 N, vec3 waterColor)
{
    vec2 refruv0 = cpos.xy / cpos.w;
    vec2 refruv1 = refruv0 + N.xz * CFG_REFRACTION_STRENGTH;

    vec4 refr0 = texture2D(s_gbufferColor, refruv0);
    refr0.w = unpackDepth(refruv0);

    vec4 refr1 = texture2D(s_gbufferColor, refruv1);
    refr1.w = unpackDepth(refruv1);

    vec4 result = mix(refr0, refr1, saturate0(refr1.w - cpos.w));

    // Estimate water absorption by a scaled depth difference
    float depthfade = saturate0((result.w - cpos.w) * u_waterParams.x + u_waterParams.y);

    // Since GBuffer depth is clamped we tone the refraction down after half of the range for a smooth fadeout
    float gbuffade = saturate0(cpos.w * (2.0 / GBUFFER_MAX_DEPTH) - 1.0);

    float fade = saturate0(depthfade + gbuffade);

    return mix(result.rgb, waterColor, fade);
}

vec3 getReflectedColor(vec4 cpos, vec3 wpos, vec3 R)
{
    vec3 result = vec3(0.0, 0.0, 0.0);
    float inside = 0.0;

    float distance = CFG_SSR_START_DISTANCE;
    float diff = 0.0;
    float diffclamp = cpos.w * CFG_SSR_STEP_CLAMP;

    vec4 Pproj = cpos;
    vec4 Rproj = clipToScreen(mul(u_viewProjection, vec4(R, 0.0)));

    for (int i = 0; i < CFG_SSR_STEPS; ++i)
    {
        distance += clamp(diff, -diffclamp, diffclamp);

        vec4 cposi = Pproj + Rproj * distance;
        vec2 uv = cposi.xy / cposi.w;
        float depth = unpackDepth(uv);

        diff = depth - cposi.w;
    }

    vec4 cposi = Pproj + Rproj * distance;
    vec2 uv = cposi.xy / cposi.w;

    // Ray hit has to be inside the screen bounds
    float ufade = abs(uv.x - 0.5) < 0.5 ? 1.0 : 0.0;
    float vfade = abs(uv.y - 0.5) < 0.5 ? 1.0 : 0.0;

    // Fade reflections out with distance
    float wfade = saturate0((4.0 - 0.1) - max(cpos.w, cposi.w) * (4.0 / GBUFFER_MAX_DEPTH));

    // Ray hit has to be reasonably close to where we started
    float dfade = abs(diff) < CFG_SSR_DEPTH_CUTOFF ? 1.0 : 0.0;

    // Avoid back-projection 
    float Vfade = Rproj.w > 0.0 ? 1.0 : 0.0;

    float fade = ufade * vfade * wfade * dfade * Vfade;

    return mix(textureCube(s_envMap, R).rgb, texture2D(s_gbufferColor, uv).rgb, fade);
}

void WaterPS()
{
    vec4 light = lgridSample(s_lightMap, s_lightMapLookup, v_lightpos_fog.xyz);
    float shadow = light.a;

    vec3 w = v_color0.xyz;

    // Use simplified normal reconstruction for LQ mobile (assumes flat water surface)
#if defined(GLSLES) && !defined(PIN_HQ)
    vec3 normal = sampleNormalSimple(v_texcoord0.xy, v_texcoord1.xy, v_texcoord2.xy, w);
#else
    vec3 normal = sampleNormal(v_texcoord0.xy, v_texcoord1.xy, v_texcoord2.xy, w, v_normal, v_color1.xyz);
#endif

    // Flatten the normal for Fresnel and for reflections to make them less chaotic
    vec3 flatNormal = mix(v_normal, normal, CFG_NORMAL_STRENGTH);

    vec3 waterColor = u_waterColor.rgb;

#ifdef PIN_HQ
    float fade = saturate0(1.0 - v_view_depth.w * u_fadeDistance_GlowFactor.y);

    vec3 view = normalize(v_view_depth.xyz);

    float fre = fresnel(dot(flatNormal, view)) * v_color0.w;

    vec3 position = u_cameraPosition - v_view_depth.xyz;

#ifdef PIN_GBUFFER
    vec3 refr = getRefractedColor(vec4(v_poslightspace, v_view_depth.w), normal, waterColor);
    vec3 refl = getReflectedColor(vec4(v_poslightspace, v_view_depth.w), position, reflect(-view, flatNormal));
#else
    vec3 refr = waterColor;
    vec3 refl = textureCube(s_envMap, reflect(-view, flatNormal)).rgb;
#endif

    float specularIntensity = CFG_SPECULAR * fade;
    float specularPower = CFG_GLOSS;

    vec3 specular = u_lamp0Color * (specularIntensity * shadow * pow(saturate0(dot(normal, normalize(-u_lamp0Dir + view))), specularPower));
#else
    vec3 view = normalize(v_view_depth.xyz);

    float fre = fresnel(dot(flatNormal, view));

    vec3 refr = waterColor;

    vec3 refl = textureCube(s_envMap, reflect(-v_view_depth.xyz, flatNormal)).rgb;

    vec3 specular = vec3(0.0, 0.0, 0.0);
#endif

    // Combine
    vec4 result;
    result.rgb = mix(refr, refl, fre) * (u_ambientColor.rgb + u_lamp0Color.rgb * shadow + light.rgb) + specular;
    result.a = 1.0;

    float fogAlpha = saturate0(v_lightpos_fog.w);

    result.rgb = mix(u_fogColor, result.rgb, fogAlpha);

    gl_FragColor = result;
}
