$input a_position, a_normal, a_texcoord0, a_texcoord1, a_tangent
$output v_pos, v_texcoord0, v_texcoord1, v_lightpos_fog, v_view_depth, v_normal, v_tangent, v_color0, v_poslightspace

#include "common.sh"
#include "globals.sh"

SAMPLER2D(s_diffuseHighMap, 0);
SAMPLER2D(s_diffuseLowMap, 1);
SAMPLER2D(s_normalMap, 2);
SAMPLER2D(s_specularMap, 3);
SAMPLER2D(s_lightMap, 4);
SAMPLER2D(s_lightMapLookup, 5);
SAMPLER2D(s_shadowMap, 6);

void MegaClusterVS()
{
    // Decode vertex data
    vec3 Normal = (a_normal.xyz - 127.0) / 127.0;
    vec4 UV = a_texcoord0 / 2048.0;
    
    // Transform position and normal to world space
    // Note: world matrix does not contain rotation/scale for static geometry so we can avoid transforming normal
    vec3 posWorld = mul(u_worldMatrix, vec4(a_position.xyz, 1.0)).xyz;
    vec3 normalWorld = Normal;

    v_pos = mul(u_viewProjection, vec4(posWorld, 1.0));

    float blend = v_pos.w / 200.0;

    v_lightpos_fog = vec4(lgridPrepareSample(lgridOffset(posWorld, normalWorld)), (u_fogParams.z - v_pos.w) * u_fogParams.w);

    v_texcoord0.xy = UV.xy;
    v_texcoord1.xy = UV.zw;

#ifdef PIN_HQ
    v_view_depth = vec4(posWorld, v_pos.w * u_fadeDistance_GlowFactor.y);
    vec4 edgeDistances = a_texcoord1 * u_fadeDistance_GlowFactor.z + 0.5 * v_view_depth.w;

    v_texcoord0.zw = edgeDistances.xy;
    v_texcoord1.zw = edgeDistances.zw;

    v_view_depth.xyz = u_cameraPosition.xyz - posWorld;
    v_normal = vec4(Normal, blend);
    // decode tangent
    v_tangent = (a_tangent.xyz - 127.0) / 127.0;
#else
    // IF LQ shading is performed in VS
    float ndotl = dot(normalWorld, -u_lamp0Dir);
    vec3 diffuse = saturate0(ndotl) * u_lamp0Color + max(-ndotl, 0.0) * u_lamp1Color;

    v_color0 = vec4(diffuse, blend);
#endif

    v_poslightspace = shadowPrepareSample(posWorld);

    gl_Position = v_pos;
}

void MegaClusterPS()
{
    vec4 high = texture2D(s_diffuseHighMap, v_texcoord0.xy);
    vec4 low = texture2D(s_diffuseLowMap, v_texcoord1.xy);

    vec4 light = lgridSample(s_lightMap, s_lightMapLookup, v_lightpos_fog.xyz);
    float shadow = shadowSample(s_shadowMap, v_poslightspace, light.a);

#ifdef PIN_HQ
    vec3 albedo = mix(high.rgb, low.rgb, saturate1(v_normal.a));

    // sample normal map and specular map
    vec4 normalMapSample = texture2D(s_normalMap, v_texcoord0.xy);
    vec4 specularMapSample = texture2D(s_specularMap, v_texcoord0.xy);

    // compute bitangent and world space normal
    vec3 bitangent = cross(v_normal.xyz, v_tangent.xyz);
#ifdef CLASSIC
    vec3 nmap = vec3(0.0, 0.0, 0.5);
#else
    vec3 nmap = nmapUnpack(normalMapSample);
#endif
    vec3 normal = normalize(nmap.x * v_tangent.xyz + nmap.y * bitangent + nmap.z * v_normal.xyz);

    float ndotl = dot(normal, -u_lamp0Dir);
    vec3 diffuseIntensity = saturate0(ndotl) * u_lamp0Color + max(-ndotl, 0.0) * u_lamp1Color;
    float specularIntensity = step(0.0, ndotl) * specularMapSample.r;
    float specularPower = specularMapSample.g * 255.0 + 0.01;

    // Compute diffuse and specular and combine them
    vec3 diffuse = (u_ambientColor + diffuseIntensity * shadow + light.rgb) * albedo.rgb;
    vec3 specular = u_lamp0Color * (specularIntensity * shadow * pow(saturate0(dot(normal, normalize(-u_lamp0Dir + normalize(v_view_depth.xyz)))), specularPower));
    gl_FragColor.rgb = diffuse + specular;

    // apply outlines
    float outlineFade = saturate1(v_view_depth.w * u_outlineBrightness_ShadowInfo.x + u_outlineBrightness_ShadowInfo.y);
    vec2 minIntermediate = min(v_texcoord0.wz, v_texcoord1.wz);
    float minEdgesPlus = min(minIntermediate.x, minIntermediate.y) / v_view_depth.w;
    gl_FragColor.rgb *= saturate1(outlineFade * (1.5 - minEdgesPlus) + minEdgesPlus);

    gl_FragColor.a = 1.0;

#else
    vec3 albedo = mix(high.rgb, low.rgb, saturate1(v_color0.a));

    // Compute diffuse term
    vec3 diffuse = (u_ambientColor + v_color0.rgb * shadow + light.rgb) * albedo.rgb;

    // Combine
    gl_FragColor.rgb = diffuse;
    gl_FragColor.a = 1.0;

#endif

    float fogAlpha = saturate0(v_lightpos_fog.w);

    gl_FragColor.rgb = mix(u_fogColor, gl_FragColor.rgb, fogAlpha);

#ifdef CLASSIC
    gl_FragColor.rgb = v_texcoord0.xyy;
    gl_FragColor.rgb = u_outlineBrightness_ShadowInfo.xxx;
#endif
}
