$input a_position, a_normal, a_texcoord0, a_texcoord1
$output v_pos, v_color0, v_color1, v_texcoord0, v_texcoord1, v_texcoord2, v_lightpos_fog, v_poslightspace, v_normal, v_view_depth

#include "common.sh"
#include "globals.sh"

uniform vec4 u_layerScale;

SAMPLER2D(s_albedoMap, 0);
SAMPLER2D(s_normalMap, 1);
SAMPLER2D(s_specularMap, 2);
SAMPLERCUBE(s_envMap, 3);
SAMPLER2D(s_lightMap, 4);
SAMPLER2D(s_lightMapLookup, 5);
SAMPLER2D(s_shadowMap, 6);

vec4 getUV(vec3 position, int material, int projection, float seed)
{
    vec3 u = u_worldMatrixArray[1 + projection].xyz;
    vec3 v = u_worldMatrixArray[19 + projection].xyz;

    vec4 m = u_worldMatrixArray[37 + material];

    vec2 uv = vec2(dot(position, u), dot(position, v)) * m.x + m.y * vec2(seed, floor(seed * 2.6651441));

    return vec4(uv, m.zw);
}

void TerrainVS()
{
    vec3 posWorld = a_position.xyz * u_worldMatrixArray[0].w + u_worldMatrixArray[0].xyz;
    vec3 normalWorld = a_normal.xyz * (1.0 / 127.0) - 1.0;

    v_pos = mul(u_viewProjection, vec4(posWorld, 1.0));

    v_lightpos_fog = vec4(lgridPrepareSample(lgridOffset(posWorld, normalWorld)), (u_fogParams.z - v_pos.w) * u_fogParams.w);

    v_poslightspace = shadowPrepareSample(posWorld);

    v_texcoord0 = getUV(posWorld, int(a_texcoord0.x), int(a_texcoord1.x), a_normal.w / 255.0);
    v_texcoord1 = getUV(posWorld, int(a_texcoord0.y), int(a_texcoord1.y), a_texcoord0.w / 255.0);
    v_texcoord2 = getUV(posWorld, int(a_texcoord0.z), int(a_texcoord1.z), a_texcoord1.w / 255.0);

#if defined(GLSLES) && !defined(GL3) // iPad2 workaround
    v_color0.xyz = vec3(
        abs(a_position.w - 0.0) < 0.1 ? 1.0 : 0.0,
        abs(a_position.w - 1.0) < 0.1 ? 1.0 : 0.0,
        abs(a_position.w - 2.0) < 0.1 ? 1.0 : 0.0
    );
#else
    v_color0.xyz = vec3(
        a_position.w == 0.0 ? 1.0 : 0.0,
        a_position.w == 1.0 ? 1.0 : 0.0,
        a_position.w == 2.0 ? 1.0 : 0.0
    );
#endif

#ifdef PIN_HQ
    v_normal = normalWorld;
    v_view_depth = vec4(u_cameraPosition - posWorld, v_pos.w);
    v_color1.xyz = vec3(
        a_texcoord1.x > 7.5 ? 1.0 : 0.0,
        a_texcoord1.y > 7.5 ? 1.0 : 0.0,
        a_texcoord1.z > 7.5 ? 1.0 : 0.0
    ); // side vs top
#else
    float ndotl = dot(normalWorld, -u_lamp0Dir);
    vec3 diffuse = max(ndotl, 0.0) * u_lamp0Color + max(-ndotl, 0.0) * u_lamp1Color;

    v_color1.xyz = diffuse;
#endif

    gl_Position = v_pos;
}

vec4 sampleMap(sampler2D s, vec4 uv)
{
#ifdef PIN_HQ
    vec2 uvs = uv.xy * u_layerScale.xy;

    return texture2DGrad(s, fract(uv.xy) * u_layerScale.xy + uv.zw, dFdx(uvs), dFdy(uvs));
#else
    return texture2D(s, fract(uv.xy) * u_layerScale.xy + uv.zw);
#endif
}

vec4 sampleBlend(sampler2D s, vec4 uv0, vec4 uv1, vec4 uv2, vec3 w)
{
    return
        sampleMap(s, uv0) * w.x + 
        sampleMap(s, uv1) * w.y +
        sampleMap(s, uv2) * w.z;
}

vec3 sampleNormal(sampler2D s, vec4 uv0, vec4 uv1, vec4 uv2, vec3 w, vec3 normal, vec3 tsel)
{
    return terrainNormal(sampleMap(s, uv0), sampleMap(s, uv1), sampleMap(s, uv2), w, normal, tsel);
}

void TerrainPS()
{
    vec4 light = lgridSample(s_lightMap, s_lightMapLookup, v_lightpos_fog.xyz);
    float shadow = shadowSample(s_shadowMap, v_poslightspace, light.a);

    vec3 w = v_color0.xyz;

    vec4 albedo = sampleBlend(s_albedoMap, v_texcoord0, v_texcoord1, v_texcoord2, w);

#ifdef PIN_HQ
    float fade = saturate0(1.0 - v_view_depth.w * u_fadeDistance_GlowFactor.y);

#ifndef PIN_GBUFFER
    vec3 normal = v_normal;
#else
    vec3 normal = sampleNormal(s_normalMap, v_texcoord0, v_texcoord1, v_texcoord2, w, v_normal, v_color1.xyz);
#endif

    vec4 params = sampleBlend(s_specularMap, v_texcoord0, v_texcoord1, v_texcoord2, w);

    float ndotl = dot(normal, -u_lamp0Dir);

    // Compute diffuse term
    vec3 diffuse = (u_ambientColor + (saturate0(ndotl) * u_lamp0Color + max(-ndotl, 0.0) * u_lamp1Color) * shadow + light.rgb + params.b * 2.0) * albedo.rgb;

    // Compute specular term
    float specularIntensity = step(0.0, ndotl) * params.r * fade;
    float specularPower = params.g * 128.0 + 0.01;

    vec3 specular = u_lamp0Color * (specularIntensity * shadow * pow(saturate0(dot(normal, normalize(-u_lamp0Dir + normalize(v_view_depth.xyz)))), specularPower));
#else
    // Compute diffuse term
    vec3 diffuse = (u_ambientColor + v_color1.xyz * shadow + light.rgb) * albedo.rgb;

    // Compute specular term
    vec3 specular = vec3(0.0, 0.0, 0.0);
#endif

    // Combine
    gl_FragColor.rgb = diffuse + specular;
    gl_FragColor.a = 1.0;

    float fogAlpha = saturate0(v_lightpos_fog.w);

    gl_FragColor.rgb = mix(u_fogColor, gl_FragColor.rgb, fogAlpha);
    
#ifdef PIN_GBUFFER
    gl_FragColor = gbufferPack(v_view_depth.w, diffuse.rgb, specular.rgb, fogAlpha);
#endif
}
