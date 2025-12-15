$input a_position, a_normal
$output v_pos, v_texcoord0, v_worldPos, v_normal, v_lightpos_fog, v_color0

#include "common.sh"
#include "globals.sh"

//
// Water shader.
// Big, fat and ugly.
//
// All (most) things considered, I have converged to this particular way of rendering water:
//
//   Vertex waves
//   No transparency. Solid color for deep water.
//   Fresnel law, reflects environment.
//   Phong speculars.
//   Ripples via animated normal map. Adjustable intensity, speed and scale. Affect reflection and speculars.

uniform vec4 u_nmAnimLerp; // ratio between normal map frames
uniform vec4 u_waveParams; // .x = frequency  .y = phase  .z = height
uniform vec4 u_waterColor; // deep water color

SAMPLER2D(s_normalMap1, 0);
SAMPLER2D(s_normalMap2, 1);
SAMPLERCUBE(s_envMap, 2);
SAMPLER2D(s_lightMap, 3);
SAMPLER2D(s_lightMapLookup, 4);

#ifdef PIN_HQ
#    define WATER_LOD 1
#else
#    define WATER_LOD 2
#endif

#define LODBIAS (-1.0)

float fadeFactor(vec3 wspos)
{
    return saturate0(-0.4 + 1.4 * length(u_cameraPosition - wspos.xyz) * u_fadeDistance_GlowFactor.y);
}

float wave(vec4 wspos)
{
    float x = sin((wspos.z - wspos.x - u_waveParams.y) * u_waveParams.x);
    float z = sin((wspos.z + wspos.x + u_waveParams.y) * u_waveParams.x);
    float p = (x + z) * u_waveParams.z;
    return p - p * fadeFactor(wspos.xyz);
}

// perturbs the water mesh and vertex normals
void makeWaves(inout vec4 wspos, inout vec3 wsnrm)
{
#if WATER_LOD == 0 
    float gridSize = 4.0;

    vec4 wspos1 = wspos;
    vec4 wspos2 = wspos;

    wspos1.x += gridSize;
    wspos2.z += gridSize;
    
    wspos.y  += wave(wspos);
    wspos1.y += wave(wspos1);
    wspos2.y += wave(wspos2);
    
    wsnrm = normalize(cross(wspos2.xyz - wspos.xyz, wspos1.xyz - wspos.xyz));
#elif WATER_LOD == 1
    wspos.y += wave(wspos);
#else   /* do nothing */
#endif
}

void water_vs()
{
    // Decode vertex data
    vec3 normal = (a_normal.xyz - 127.0) / 127.0;
    normal = normalize(normal);
    
    vec4 wspos = mul(u_worldMatrix, vec4(a_position.xyz, 1.0));
    vec3 wsnrm = normal;

    wspos.y -= 2.0 * u_waveParams.z;

    makeWaves(wspos, wsnrm);

    v_worldPos = wspos;
    v_normal = wsnrm;

    if (normal.y < 0.01) v_normal = normal;

    // box mapping
    vec2 tcselect;
    vec3 wspostc = vec3(wspos.x, -wspos.y, wspos.z);

    tcselect.x = dot(abs(normal.yxz), wspostc.xzx);
    tcselect.y = dot(abs(normal.yxz), wspostc.zyy);

    v_pos = mul(u_viewProjection, wspos);
    v_texcoord0.xy = tcselect * 0.05;
    v_texcoord0.z = saturate0((u_fogParams.z - v_pos.w) * u_fogParams.w);
    v_texcoord0.w = LODBIAS;

    v_lightpos_fog.xyz = lgridPrepareSample(lgridOffset(wspos.xyz, wsnrm.xyz));

    v_color0.x = fadeFactor(wspos.xyz);
    v_color0.y = (1.0 - v_color0.x) * saturate0(dot(wsnrm, -u_lamp0Dir)) * 100.0;
    v_color0.z = 1.0 - 0.9 * saturate1(exp(-0.005 * length(u_cameraPosition - wspos.xyz)));

    gl_Position = v_pos;
}

vec3 pixelNormal(vec4 tc0)
{
    vec4 nm1 = texture2DLod(s_normalMap1, tc0.xy, tc0.w);
#if WATER_LOD <= 1
    vec4 nm2 = texture2DLod(s_normalMap2, tc0.xy, tc0.w);
    vec4 nm3 = mix(nm1, nm2, u_nmAnimLerp.xxxx);
#else
    vec4 nm3 = nm1;
#endif
    return nmapUnpack(nm3);
}

// Fresnel approximation. N1 and N2 are refractive indices.
// for above water, use n1 = 1, n2 = 1.3, for underwater use n1 = 1.3, n2 = 1
float fresnel(vec3 N, vec3 V, float n1, float n2, float p, float fade)
{
#if WATER_LOD == 0
    float r0 = (n1 - n2) / (n1 + n2);
    r0 *= r0;
    return r0 + (1.0 - r0) * pow(1.0 - abs(dot(normalize(N), V)), p);
#else
    return 0.1 + saturate0(-1.9 * abs(dot(N, V)) + 0.8); // HAXX!
#endif
}

vec4 envColor(vec3 N, vec3 V, float fade)
{
    vec3 dir = reflect(V, N);
    return textureCube(s_envMap, dir) * 0.91;
}

vec4 deepWaterColor(vec4 light)
{
    vec4 tint = 0.8 * vec4(118.0, 143.0, 153.0, 255.0) / 255.0;
    return (light + textureCubeLod(s_envMap, vec3(0.0, 1.0, 0.0), 10.0)) * tint;
}

void water_ps()
{
    vec3 N2 = v_normal;
    vec3 N1 = pixelNormal(v_texcoord0).xzy;
    vec3 N3 = 0.5 * (N2 + N1);

    N3 = mix(N3, N2, v_color0.z);

    vec3 L = -u_lamp0Dir.xyz;
    vec3 E = normalize(u_cameraPosition - v_worldPos.xyz);

    vec4 light = lgridSample(s_lightMap, s_lightMapLookup, v_lightpos_fog.xyz);
    
    float fre = fresnel(N3, E, 1.0, 1.3, 5.0, v_color0.x);
    vec3 diffuse = deepWaterColor(light).rgb;
    vec3 env = envColor(N3, -E, v_color0.x).rgb;

    vec3 R = reflect(-L, N1);  
    
#if WATER_LOD <= 1
    float specular = pow(saturate0(dot(R, E)), 1600.0) * L.y * 100.0; // baseline
#    ifndef GLSLES
        specular = 0.65 * saturate1(specular * saturate0(light.a - 0.4));
#    endif
#else
    float specular = 0.0;
#endif

    vec3 result = mix(diffuse, env, fre) + vec3(specular, specular, specular);
    result = mix(u_fogColor.rgb, result, v_texcoord0.z);
        
    gl_FragColor = vec4(result, 1.0);
}
