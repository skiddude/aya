$input a_position, a_normal, a_texcoord0, a_texcoord1, a_color0, a_color1, a_texcoord2, a_texcoord3
$output v_pos, v_worldPos, v_texcoord0, v_texcoord1, v_color0, v_normal, v_tangent, v_lightpos_fog, v_view_depth, v_poslightspace, v_edgedist, v_reflection

#include "common.sh"

#define LQMAT_FADE_FACTOR (1.0/300.0)
#define BEVEL_WIDTH 0.06
#define PI 3.14159265359

#define BEVEL_FADE_DIST 120.0
#define BEVEL_FADE_DIST_START 0.0

#define SPEC_EXPON 81.0
#define KS 0.75

#define FADE_DIST 500.0
#define FADE_DIST_START 0.0

uniform vec4 u_worldMatrixArray[MAX_BONE_COUNT * 3];
uniform vec4 u_debugColor;

SAMPLER2D(s_studsMap, 0);
SAMPLER2D(s_lightMap, 1);
SAMPLER2D(s_lightMapLookup, 2);
SAMPLER2D(s_shadowMap, 3);
SAMPLERCUBE(s_environmentMap, 4);
SAMPLER2D(s_diffuseMap, 5);
SAMPLER2D(s_normalMap, 6);
SAMPLER2D(s_specularMap, 7);

#ifndef GLSLES
SAMPLER2D(s_normalDetailMap, 8);
#endif

uniform vec4 u_lqmatFarTilingFactor;

void Shade(vec3 albedo, vec3 nn, vec3 vn, float ks, float specExpon, vec4 light, out vec3 diffuse, out vec3 specular)
{
    vec3 ln = normalize(-u_lamp0Dir);
    vec3 hn = normalize(ln + vn);
    float ndl = dot(nn, ln);
    float ndh = dot(nn, hn);

    vec2 lit0 = vec2(max(0.0, ndl), pow(max(0.0, ndh), specExpon) * step(0.0, ndl));
    vec3 lampColor = u_lamp0Color;
    diffuse = (light.a * (lampColor * lit0.x) + u_ambientColor + light.xyz);
    diffuse *= albedo;
    specular = (lampColor * lit0.y) * ks * light.a;
}

vec3 CalcBevel(vec4 edgeDistances, vec3 normal, float viewDepth)
{
    vec4 bevelMultiplier = clamp((BEVEL_WIDTH - edgeDistances) / BEVEL_WIDTH, 0.0, 1.0);
    float fade = clamp(1.0 - (viewDepth - BEVEL_FADE_DIST_START) / BEVEL_FADE_DIST, 0.0, 1.0);
    bevelMultiplier *= fade;

    normal += bevelMultiplier.x * vec3(0.5, 0.0, 0.0);
    normal += bevelMultiplier.y * vec3(-0.5, 0.0, 0.0);
    normal += bevelMultiplier.z * vec3(0.0, 0.5, 0.0);
    normal += bevelMultiplier.w * vec3(0.0, -0.5, 0.0);

    return normalize(normal);
}

vec3 DisplaceCoord(vec3 pos)
{
    float cfactor = 0.980066578;
    float sfactor = 0.198669331;
    float cfactor2 = 0.955336489;
    float sfactor2 = 0.295520207;
    float cfactor3 = 0.921060994;
    float sfactor3 = 0.389418342;
    vec3 p = pos.xyz;
    vec3 shiftPos = p;

    shiftPos.x += p.x * cfactor + p.z * sfactor;
    shiftPos.z += p.x * -sfactor + p.z * cfactor;
    
    shiftPos.x += p.x * cfactor2 - p.y * sfactor2;
    shiftPos.y += p.x * sfactor2 + p.y * cfactor2;
    
    shiftPos.y += p.y * cfactor3 - p.z * sfactor3;
    shiftPos.z += p.y * sfactor3 + p.z * cfactor3;
    
    return shiftPos;
}

#ifdef CLASSIC

void DefaultVS()
{
    // Transform position and normal to world space
#ifdef PIN_SKINNED
    int boneIndex = int(a_color1.r);

    vec4 worldRow0 = u_worldMatrixArray[boneIndex * 3];
    vec4 worldRow1 = u_worldMatrixArray[boneIndex * 3 + 1];
    vec4 worldRow2 = u_worldMatrixArray[boneIndex * 3 + 2];

    vec3 posWorld = vec3(dot(worldRow0, vec4(a_position, 1.0)), 
                         dot(worldRow1, vec4(a_position, 1.0)), 
                         dot(worldRow2, vec4(a_position, 1.0)));
    vec3 normalWorld = vec3(dot(worldRow0.xyz, a_normal), 
                            dot(worldRow1.xyz, a_normal), 
                            dot(worldRow2.xyz, a_normal));
    v_tangent = vec3(dot(worldRow0.xyz, a_texcoord2), 
                     dot(worldRow1.xyz, a_texcoord2), 
                     dot(worldRow2.xyz, a_texcoord2));
#else
    vec3 posWorld = a_position;
    vec3 normalWorld = a_normal;
    v_tangent = a_texcoord2;
#endif
    v_pos = a_position;
    v_worldPos = posWorld;

    vec4 color = a_color0;

    gl_Position = mul(u_viewProjection, vec4(posWorld, 1.0));
    v_normal = normalWorld;
    v_texcoord0 = a_texcoord0;
    v_texcoord1 = a_texcoord1;
    
    v_color0 = color;
    v_lightpos_fog = vec4(lgridPrepareSample(lgridOffset(posWorld, normalWorld)), 
                          (u_fogParams.z - gl_Position.w) * u_fogParams.w);

    v_view_depth = vec4(u_cameraPosition - posWorld, gl_Position.w);

#ifdef CLASSIC_GOURAUD
    vec4 light = lgridSample(s_lightMap, s_lightMapLookup, v_lightpos_fog.xyz);
    vec3 diffuse, specular;
    Shade(vec3(1.0), normalize(v_normal), normalize(v_view_depth.xyz), 0.71, 81.0, light, diffuse, specular);
    // Store in unused varyings
#endif

#if defined(PIN_HQ) || defined(PIN_REFLECTION)
    v_reflection = a_color1.a / 255.0;
    v_edgedist = a_texcoord3;
#endif

    v_poslightspace = shadowPrepareSample(posWorld);
}

void DefaultPS()
{
    float normalStrength = 0.4;
    float fade = clamp(normalStrength - (v_view_depth.w - FADE_DIST_START) / FADE_DIST, 0.0, 1.0);
    vec3 nn = normalize(v_normal);
    vec4 edgeDistances = v_edgedist;

    vec4 albedo = v_color0;
    vec2 uv;

#ifndef PIN_MESH
    uv = v_texcoord0;
    
    mat3 normalMatrix = mat3(
        v_tangent.xyz,
        cross(nn, v_tangent.xyz),
        nn
    );
    
    vec3 tn = vec3(0.0, 0.0, 0.5);
    tn = mix(vec3(0.0, 0.0, 0.5), tn, fade);
    tn = CalcBevel(edgeDistances, tn, v_view_depth.w);
    nn = mul(tn, normalMatrix);
    
    vec4 colorTex = texture2D(s_diffuseMap, uv);
    albedo *= colorTex;
#else
    uv = v_texcoord0;
    vec4 colorTex = texture2D(s_diffuseMap, uv);
    albedo *= colorTex;
#endif

    vec3 vn = normalize(v_view_depth.xyz);
    vec4 light = lgridSample(s_lightMap, s_lightMapLookup, v_lightpos_fog.xyz);

    vec3 diffusePhong, specularPhong;
    vec3 diffuse, specular;
    
#ifdef CLASSIC_GOURAUD
    // Use pre-computed lighting from vertex shader
    diffuse = albedo.rgb; // Should use stored diffuse from VS
    specular = vec3(0.0); // Should use stored specular from VS
#else
    Shade(albedo.rgb, nn, vn, KS, SPEC_EXPON, light, diffusePhong, specularPhong);
    diffuse = diffusePhong;
    specular = specularPhong;
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
    // Output to second render target
    // gl_FragData[1] = gbufferPack(v_view_depth.w, diffuse.rgb, specular.rgb, fogAlpha);
#endif
}

#else // !CLASSIC - Modern rendering path

void DefaultVS()
{
    // Transform position and normal to world space
#ifdef PIN_SKINNED
    int boneIndex = int(a_color1.r);

    vec4 worldRow0 = u_worldMatrixArray[boneIndex * 3 + 0];
    vec4 worldRow1 = u_worldMatrixArray[boneIndex * 3 + 1];
    vec4 worldRow2 = u_worldMatrixArray[boneIndex * 3 + 2];
        
    vec3 posWorld = vec3(dot(worldRow0, vec4(a_position, 1.0)), 
                         dot(worldRow1, vec4(a_position, 1.0)), 
                         dot(worldRow2, vec4(a_position, 1.0)));
    vec3 normalWorld = vec3(dot(worldRow0.xyz, a_normal), 
                            dot(worldRow1.xyz, a_normal), 
                            dot(worldRow2.xyz, a_normal));
#else
    vec3 posWorld = a_position;
    vec3 normalWorld = a_normal;
#endif

    // Decode diffuse/specular parameters
#ifdef PIN_DEBUG
    vec4 color = u_debugColor;
#else
    vec4 color = a_color0;
#endif

    float specularIntensity = a_color1.g / 255.0;
    float specularPower = float(int(a_color1.b));

    float ndotl = dot(normalWorld, -u_lamp0Dir);

#ifdef PIN_HQ
    // We'll calculate specular in pixel shader
    vec2 lt = vec2(clamp(ndotl, 0.0, 1.0), step(0.0, ndotl));
#else
    // Using lit here improves performance on software vertex shader implementations
    vec2 lt = vec2(max(0.0, ndotl), 
                   pow(max(0.0, dot(normalize(-u_lamp0Dir + normalize(u_cameraPosition - posWorld)), normalWorld)), specularPower) * step(0.0, ndotl));
#endif

    gl_Position = mul(u_viewProjection, vec4(posWorld, 1.0));

    v_texcoord0 = a_texcoord0;
    v_texcoord1 = a_texcoord1;
    
    v_color0 = color;
    v_lightpos_fog = vec4(lgridPrepareSample(lgridOffset(posWorld, normalWorld)), 
                          (u_fogParams.z - gl_Position.w) * u_fogParams.w);

    v_view_depth = vec4(u_cameraPosition - posWorld, gl_Position.w);

#if defined(PIN_HQ) || defined(PIN_REFLECTION)
    vec4 edgeDistances = a_texcoord3 * u_fadeDistance_GlowFactor.z + 0.5 * v_view_depth.w * u_fadeDistance_GlowFactor.y;
    
    v_edgedist = edgeDistances;
    v_normal = normalWorld;
    // Store specular power in normal.w if needed
    v_reflection = a_color1.a / 255.0;
#endif

#ifdef PIN_SURFACE
    #ifdef PIN_SKINNED
        vec3 tangent = vec3(dot(worldRow0.xyz, a_texcoord2), 
                           dot(worldRow1.xyz, a_texcoord2), 
                           dot(worldRow2.xyz, a_texcoord2));
    #else
        vec3 tangent = a_texcoord2;
    #endif

    v_tangent = tangent;
#else
    vec3 diffuse = lt.x * u_lamp0Color + max(-ndotl, 0.0) * u_lamp1Color;
    // Store diffuse and specular in color channels (pack into v_color0.w and other varyings)
#endif

    v_poslightspace = shadowPrepareSample(posWorld);
}

vec4 sampleFar1(sampler2D s, vec2 uv, float fade, float cutoff)
{
#ifdef GLSLES
    return texture2D(s, uv);
#else
    if (cutoff == 0.0)
        return texture2D(s, uv);
    else
    {
        float cscale = 1.0 / (1.0 - cutoff);
        return mix(texture2D(s, uv * u_lqmatFarTilingFactor.xy), 
                   texture2D(s, uv), 
                   saturate0(fade * cscale - cutoff * cscale));
    }
#endif
}

#if defined(PIN_WANG) || defined(PIN_WANG_FALLBACK)
vec4 sampleWangSimple(sampler2D s, vec2 uv)
{
    vec2 wangUv;
    vec4 wangUVDerivatives;
    getWang(s_normalDetailMap, uv, 1.0, wangUv, wangUVDerivatives);
    return sampleWang(s, wangUv, wangUVDerivatives);
}
#endif

void DefaultPS()
{
    // Compute albedo term
#ifdef PIN_SURFACE
    // Surface shader path - would need surface shader implementation
    vec4 albedo = v_color0;
    vec3 normal = normalize(v_normal);
#elif defined(PIN_LOWQMAT)

#ifndef CFG_FAR_DIFFUSE_CUTOFF
#define CFG_FAR_DIFFUSE_CUTOFF 0.6
#endif

#if defined(PIN_WANG) || defined(PIN_WANG_FALLBACK)
    vec4 albedo = sampleWangSimple(s_diffuseMap, v_texcoord0);
#else
    float fade = saturate0(1.0 - v_view_depth.w * LQMAT_FADE_FACTOR);
    vec4 albedo = sampleFar1(s_diffuseMap, v_texcoord0, fade, CFG_FAR_DIFFUSE_CUTOFF);
#endif

    albedo.rgb = mix(vec3(1.0), v_color0.rgb, albedo.a) * albedo.rgb;
    albedo.a = v_color0.a;

    // Diffuse and specular from vertex shader (stored in varyings)
    vec3 diffuseIntensity = vec3(1.0); // Should unpack from varyings
    float specularIntensity = 0.0; // Should unpack from varyings
    float reflectance = 0.0;

#else
    #ifdef PIN_PLASTIC
        vec4 studs = texture2D(s_studsMap, v_texcoord1);
        vec4 albedo = vec4(v_color0.rgb * (studs.r * 2.0), v_color0.a);
    #else
        vec4 albedo = texture2D(s_diffuseMap, v_texcoord0) * v_color0;
    #endif

    #ifdef PIN_HQ
        vec3 normal = normalize(v_normal);
        float specularPower = 81.0; // Should come from varying
    #elif defined(PIN_REFLECTION)
        vec3 normal = v_normal;
    #endif

    vec3 diffuseIntensity = vec3(1.0); // Should unpack from varyings
    float specularIntensity = 0.0; // Should unpack from varyings

    #ifdef PIN_REFLECTION
        float reflectance = v_reflection;
    #endif

#endif

    vec4 light = lgridSample(s_lightMap, s_lightMapLookup, v_lightpos_fog.xyz);
    float shadow = shadowSample(s_shadowMap, v_poslightspace, light.a);

    // Compute reflection term
#if defined(PIN_SURFACE) || defined(PIN_REFLECTION)
    vec3 reflection = textureCube(s_environmentMap, reflect(-v_view_depth.xyz, normal)).rgb;
    albedo.rgb = mix(albedo.rgb, reflection.rgb, reflectance);
#endif
    
    // Compute diffuse term
    vec3 diffuse = (u_ambientColor + diffuseIntensity * shadow + light.rgb) * albedo.rgb;

    // Compute specular term
#ifdef PIN_HQ
    vec3 normal = normalize(v_normal);
    float specularPower = 81.0;
    vec3 specular = u_lamp0Color * (specularIntensity * shadow * 
                    pow(clamp(dot(normal, normalize(-u_lamp0Dir + normalize(v_view_depth.xyz))), 0.0, 1.0), specularPower));
#else
    vec3 specular = u_lamp0Color * (specularIntensity * shadow);
#endif

    // Combine
    vec3 resultColor = diffuse.rgb + specular.rgb;
    float resultAlpha = albedo.a;

#ifdef PIN_HQ
    float ViewDepthMul = v_view_depth.w * u_fadeDistance_GlowFactor.y;
    float outlineFade = saturate1(ViewDepthMul * u_outlineBrightness_ShadowInfo.x + u_outlineBrightness_ShadowInfo.y);
    vec2 minIntermediate = min(v_edgedist.xy, v_edgedist.zw);
    float minEdgesPlus = min(minIntermediate.x, minIntermediate.y) / ViewDepthMul;
    resultColor *= saturate1(outlineFade * (1.5 - minEdgesPlus) + minEdgesPlus);
#endif

    float fogAlpha = clamp(v_lightpos_fog.w, 0.0, 1.0);

#ifdef PIN_NEON
    resultColor = v_color0.rgb * u_fadeDistance_GlowFactor.w;
    resultAlpha = 1.0 - fogAlpha * v_color0.a;
    diffuse.rgb = vec3(0.0);
    specular.rgb = vec3(0.0);
#endif

    resultColor = mix(u_fogColor, resultColor, fogAlpha);

    gl_FragColor = vec4(resultColor, resultAlpha);

#ifdef PIN_GBUFFER
    // Output to second render target
    // gl_FragData[1] = gbufferPack(v_view_depth.w, diffuse.rgb, specular.rgb, fogAlpha);
#endif
}

#endif // CLASSIC
