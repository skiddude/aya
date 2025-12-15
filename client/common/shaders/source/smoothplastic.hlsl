#ifdef CLASSIC
#define PIN_SURFACE
#define ALREADY_HAS_PS
#include "default.hlsl"

#define SPEC_EXPON 81 //81
#define KS .75

#define FADE_DIST 500
#define FADE_DIST_START 0

float3 CalcBevelDir(float4 edgeDistances)
{
    float3 dir = float3(0, 0, 0);
    float4 bevelMultiplier = edgeDistances <= BEVEL_WIDTH;

    dir += bevelMultiplier.x * float3(1, 0, 0);
    dir += bevelMultiplier.y * float3(-1, 0, 0);
    dir += bevelMultiplier.z * float3(0, 1, 0);
    dir += bevelMultiplier.w * float3(0, -1, 0);

    return dir;
}

void DefaultPS(VertexOutput IN,
#ifdef PIN_GBUFFER
    out float4 oColor1: COLOR1,
#endif
    out float4 oColor0: COLOR0)
{
    float normalStrength = .4;
    float fade = saturate(normalStrength - (IN.View_Depth.w - FADE_DIST_START) / FADE_DIST);
    float3 nn = normalize(IN.Normal);
    float4 edgeDistances = IN.EdgeDistances;

    float4 albedo = IN.Color;

    // lets generate this matrix in the vertex shader and pass it in later
    float3x3 normalMatrix = {
        IN.Tangent.xyz,
        cross(nn, IN.Tangent.xyz),
        nn
    };

    float2 uv = IN.Uv;
    float3 tn = float3(0,0,.5);
    tn = lerp(float3(0, 0, .5), tn, fade);
    tn = CalcBevel(edgeDistances, tn, IN.View_Depth.w);
    nn = mul(tn, normalMatrix);

    float3 vn = normalize(IN.View_Depth.xyz);
    float4 light = lgridSample(TEXTURE(LightMap), TEXTURE(LightMapLookup), IN.LightPosition_Fog.xyz);

    float3 diffuse;
    float3 specular;
    Shade(albedo.rgb, nn, vn, KS, SPEC_EXPON, light, diffuse, specular);
    
    float3 result = diffuse + specular;

    #ifdef PIN_REFLECTION
        float3 reflection = texCUBE(EnvironmentMap, reflect(-vn, nn)).rgb;
        result = lerp(result, reflection, IN.Reflection);
    #endif

    float fogAlpha = saturate((G(FogParams).z - length(IN.View_Depth.xyz)) * G(FogParams).w);
    result = lerp(G(FogColor), result, fogAlpha);
    oColor0 = float4(result, albedo.a);

    #ifdef PIN_GBUFFER
        oColor1 = gbufferPack(IN.View_Depth.w, diffuse.rgb, specular.rgb, fogAlpha);
    #endif
}
#else
#define PIN_PLASTIC
#include "default.hlsl"
#endif