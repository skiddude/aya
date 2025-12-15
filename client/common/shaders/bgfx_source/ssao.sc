$input a_position
$output v_pos, v_texcoord0, v_texcoord1, v_texcoord2

#include "common.sh"
#include "globals.sh"

// ao sampling
#define SSAO_NUM_PAIRS 8
#define SSAO_RADIUS 2.0
#define SSAO_MIN_PIXELS 10.0
#define SSAO_MAX_PIXELS 100.0
#define SSAO_MIP_OFFSET 2.0

// ao tuning
#define SSAO_OVERSHADOW 0.75
#define SSAO_ANGLELIMIT 0.1
#define SSAO_BOOST 1.0

// blur tuning
#define SSAO_BLUR_SAMPLES 3
#define SSAO_BLUR_STRENGTH 1.5
#define SSAO_BLUR_DEPTH_THRESHOLD 0.4

// composite tuning
#define SSAO_DEPTH_THRESHOLD_CENTER 0.02
#define SSAO_DEPTH_THRESHOLD_ESTIMATE 0.4

SAMPLER2D(s_depthBuffer, 0);
SAMPLER2D(s_randMap, 1);
SAMPLER2D(s_map, 2);
SAMPLER2D(s_geomMap, 3);

uniform vec4 u_textureSize;
uniform vec4 u_params;

vec4 convertPosition(vec4 p, float scale)
{
#if defined(GLSL) || defined(DX11)
    return p;
#else
    // half-pixel offset
    return p + vec4(-u_textureSize.z, u_textureSize.w, 0.0, 0.0) * scale;
#endif
}

vec2 convertUv(vec4 p)
{
#ifndef GLSL
    return p.xy * vec2(0.5, -0.5) + 0.5;
#else
    return p.xy * 0.5 + 0.5;
#endif
}

// ===== Pass Through =====
void passThrough_vs()
{
    v_pos = convertPosition(vec4(a_position.xyz, 1.0), 1.0);
    v_texcoord0.xy = convertUv(vec4(a_position.xyz, 1.0));
    gl_Position = v_pos;
}

// ===== SSAO Depth Downsample =====
void SSAODepthDownPS()
{
    float d0 = texture2D(s_geomMap, v_texcoord0.xy + u_textureSize.zw * vec2(-0.25, -0.25)).r;
    float d1 = texture2D(s_geomMap, v_texcoord0.xy + u_textureSize.zw * vec2(+0.25, -0.25)).r;
    float d2 = texture2D(s_geomMap, v_texcoord0.xy + u_textureSize.zw * vec2(-0.25, +0.25)).r;
    float d3 = texture2D(s_geomMap, v_texcoord0.xy + u_textureSize.zw * vec2(+0.25, +0.25)).r;

    gl_FragColor = vec4(min(min(d0, d3), min(d1, d2)));
}

// ===== SSAO Calculation =====
float getSampleLength(float i)
{
    return (i + 1.0) / (float(SSAO_NUM_PAIRS) + 2.0);
}

vec2 getSampleRotation(float i)
{
    float pi = 3.1415926;
    return vec2(cos(i / float(SSAO_NUM_PAIRS) * 2.0 * pi), sin(i / float(SSAO_NUM_PAIRS) * 2.0 * pi));
}

void SSAOPS()
{
    float baseDepth = texture2DLod(s_depthBuffer, vec4(v_texcoord0.xy, 0.0, 0.0)).r;

    vec4 noiseTex = texture2D(s_randMap, fract(v_texcoord0.xy * u_textureSize.xy / 4.0)) * 2.0 - 1.0;
    
    mat2 rotation = mat2(
        noiseTex.y, noiseTex.x,
        -noiseTex.x, noiseTex.y
    );
    
    const float sphereRadiusZB = SSAO_RADIUS / GBUFFER_MAX_DEPTH;
    
    vec2 radiusTex = clamp(sphereRadiusZB / baseDepth * u_params.xy, SSAO_MIN_PIXELS * u_textureSize.zw, SSAO_MAX_PIXELS * u_textureSize.zw);
    
    float lod = log2(getSampleLength(0.0) * length(radiusTex * u_textureSize.xy)) - SSAO_MIP_OFFSET;

    float result = 1.0; // center pixel
    float weight = 2.0;

    for (int i = 0; i < SSAO_NUM_PAIRS; i++)
    {
        const float offsetLength = getSampleLength(float(i));
        const vec2 offsetVector = getSampleRotation(float(i)) * offsetLength;
        const float segmentDiff = sphereRadiusZB * sqrt(1.0 - offsetLength * offsetLength);
        const float angleLimit = offsetLength * SSAO_ANGLELIMIT;

        vec2 offset = mul(rotation, offsetVector) * radiusTex;
    
        vec2 offsetDepth;
        offsetDepth.x = texture2DLod(s_depthBuffer, vec4(v_texcoord0.xy + offset, 0.0, lod)).r;
        offsetDepth.y = texture2DLod(s_depthBuffer, vec4(v_texcoord0.xy - offset, 0.0, lod)).r;
        
        vec2 diff = offsetDepth - vec2(baseDepth, baseDepth);
        
        // 0 is the near surface of the sphere, 1 is the far surface, 0.5 is the middle
        vec2 normalizedDiff = diff * (1.0 / segmentDiff * 0.5) + 0.5;

        // only add sample contribution if both samples are visible - if one is invisible we estimate the twin as 1-s so sum is 1
        float sampleadd = saturate0(min(normalizedDiff.x, normalizedDiff.y) + SSAO_OVERSHADOW);

        result += (saturate0(normalizedDiff.x + angleLimit) + saturate0(normalizedDiff.y + angleLimit)) * sampleadd;
        weight += 2.0 * sampleadd;
    }
    
    // rescale result from 0..0.5 to 0..1 and apply a power function
    float finalocc = (baseDepth > 0.99) ? 1.0 : pow(saturate0(result / weight * 2.0), SSAO_BOOST);
    
    gl_FragColor = vec4(finalocc, baseDepth, 0.0, 1.0);
}

// ===== SSAO Blur =====
vec2 ssaoBlur(vec2 uv, vec2 offset, sampler2D map)
{
    float sigmaN = 1.0 / (2.0 * SSAO_BLUR_STRENGTH * SSAO_BLUR_STRENGTH);

    float baseDepth = texture2D(map, uv).g;

    const float sphereRadiusZB = SSAO_BLUR_DEPTH_THRESHOLD / GBUFFER_MAX_DEPTH;

    float depthTolerance = clamp((baseDepth * 80.0) * sphereRadiusZB, 0.1 * sphereRadiusZB, 10.0 * sphereRadiusZB);

    float result = 0.0;
    float weight = 0.0;

    for (int i = -SSAO_BLUR_SAMPLES; i <= SSAO_BLUR_SAMPLES; ++i)
    {
        const float ix = float(i);
        const float iw = exp(-ix * ix * sigmaN);

        vec4 data = texture2D(map, uv + offset * ix);

        float w = iw * (abs(data.g - baseDepth) < depthTolerance ? 1.0 : 0.0);

        result += data.r * w;
        weight += w;
    }

    return vec2(result / weight, baseDepth);
}

void SSAOBlurXPS()
{
    vec2 o = vec2(u_textureSize.z, 0.0);

    vec2 ssaoTerm = ssaoBlur(v_texcoord0.xy, o, s_map);

    gl_FragColor = vec4(ssaoTerm, 0.0, 1.0);
}

void SSAOBlurYPS()
{
    vec2 o = vec2(0.0, u_textureSize.w);

    vec2 ssaoTerm = ssaoBlur(v_texcoord0.xy, o, s_map);

    gl_FragColor = vec4(ssaoTerm, 0.0, 1.0);
}

// ===== SSAO Composite =====
void SSAOCompositVS()
{
    vec2 uv = convertUv(vec4(a_position.xyz, 1.0));

    v_pos = convertPosition(vec4(a_position.xyz, 1.0), 1.0);
    v_texcoord0.xy = uv;

    vec2 uvOffset = u_textureSize.zw * 2.0;

    v_texcoord0.zw = uv + vec2(uvOffset.x, 0.0);
    v_texcoord1.xy = uv - vec2(uvOffset.x, 0.0);
    v_texcoord1.zw = uv + vec2(0.0, uvOffset.y);
    v_texcoord2.xy = uv - vec2(0.0, uvOffset.y);

    gl_Position = v_pos;
}
 
void SSAOCompositPS()
{
    vec4 geom = texture2D(s_geomMap, v_texcoord0.xy);
    vec4 mapC = texture2D(s_map, v_texcoord0.xy);
    vec4 map0 = texture2D(s_map, v_texcoord0.zw);
    vec4 map1 = texture2D(s_map, v_texcoord1.xy);
    vec4 map2 = texture2D(s_map, v_texcoord1.zw);
    vec4 map3 = texture2D(s_map, v_texcoord2.xy);

    float baseDepth = geom.r;

    float ssaoC = mapC.r;
    float depthC = mapC.g;

    vec4 ssaoEst = vec4(map0.r, map1.r, map2.r, map3.r);
    vec4 depthEst = vec4(map0.g, map1.g, map2.g, map3.g);

    // can we trust the neighbors? 1 - yes, 0 - no
    vec4 checkEst = vec4(
        abs(depthEst.x - baseDepth) < SSAO_DEPTH_THRESHOLD_ESTIMATE / GBUFFER_MAX_DEPTH ? 1.0 : 0.0,
        abs(depthEst.y - baseDepth) < SSAO_DEPTH_THRESHOLD_ESTIMATE / GBUFFER_MAX_DEPTH ? 1.0 : 0.0,
        abs(depthEst.z - baseDepth) < SSAO_DEPTH_THRESHOLD_ESTIMATE / GBUFFER_MAX_DEPTH ? 1.0 : 0.0,
        abs(depthEst.w - baseDepth) < SSAO_DEPTH_THRESHOLD_ESTIMATE / GBUFFER_MAX_DEPTH ? 1.0 : 0.0
    );
    
    float checkEstSum = dot(checkEst, vec4(1.0, 1.0, 1.0, 1.0));
    float ssaoTermEst = dot(ssaoEst, checkEst) / checkEstSum;

    // the final decision: pick the estimate sample if there are good neighbors and base depth is not trustworthy
    float ssaoTerm = abs(depthC - baseDepth) * checkEstSum > SSAO_DEPTH_THRESHOLD_CENTER / GBUFFER_MAX_DEPTH ? ssaoTermEst : ssaoC;

    // AO reduction for high specular and diffuse values. Computed in gbufferPack in common.h
    float slope = geom.g;

    gl_FragColor = vec4((1.0 - slope) * vec3(ssaoTerm, ssaoTerm, ssaoTerm) + slope, 1.0);
}
