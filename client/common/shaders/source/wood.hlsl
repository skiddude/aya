#ifdef CLASSIC
#define PIN_SURFACE
#define ALREADY_HAS_PS
#include "default.hlsl"

#define SPEC_EXPON 4
#define KS .1

#define WoodContrast float3(0.1, 0.1, 0.1)
#define Ks1 0.32
#define Ks2 0.16
#define RingScale 4
#define AmpScale 1
#define LightRingEnd 0.4
#define DarkRingStart 0.8
#define DarkRingEnd 0.83
#define NormMapScale 0.6
#define NoiseScale 64
#define MixedColorRatio 0.315 
#define AAFreqMultiplier 12

void DefaultPS(VertexOutput IN,
#ifdef PIN_GBUFFER
    out float4 oColor1: COLOR1,
#endif
    out float4 oColor0: COLOR0)
{
    float3 nn = normalize(IN.Normal);

    float3x3 normalMatrix = {
        IN.Tangent.xyz,
        cross(nn, IN.Tangent.xyz),
        nn
    };

    float4 WoodPos = float4(IN.Pos.xyz, lerp(0, .92, abs(dot(float3(1,0,0), nn))));

    float2 NormalUV = IN.Uv.xy * NormMapScale * 3;
    float singularityAttenuation = WoodPos.w;
    
    float noiseval = tex3D(SpecularMap, WoodPos.xyz / NoiseScale).x;
    float3 tn = tex2D(NormalMap, NormalUV).xyz - .5;
    tn = lerp(tn, float3(0, 0, .5), singularityAttenuation);
    float4 edgeDistances = IN.EdgeDistances;
    tn = CalcBevel(edgeDistances, tn, IN.View_Depth.w);
    
    nn = mul(tn, normalMatrix);

    float signalfreq = length(float4(ddx(WoodPos.y), ddx(WoodPos.z), ddy(WoodPos.y), ddy(WoodPos.z))); // seems to be angle based thing
    float aa_attn = saturate(signalfreq*AAFreqMultiplier - 1.0f);
    float3 Pwood = WoodPos.xyz + (AmpScale * noiseval);
    float r = RingScale * length(Pwood.yz);
    r = r + tex3D(SpecularMap,r.xxx/32.0).x;
    r = r - floor(r); // stepping in the rings in formed through this here
    r = smoothstep(LightRingEnd, DarkRingStart, r) - smoothstep(DarkRingEnd,1.0,r);
    // apply anti-aliasing
    r = lerp(r, MixedColorRatio, aa_attn);
    
    float3 albedo = IN.Color.xyz + WoodContrast * (MixedColorRatio - r);
    float Ks = lerp(Ks1,Ks2,r);
    
    float3 vn = normalize(IN.View_Depth.xyz);
    float4 light = lgridSample(TEXTURE(LightMap), TEXTURE(LightMapLookup), IN.LightPosition_Fog.xyz);
    light.a = shadowSample(TEXTURE(ShadowMap), IN.PosLightSpace.xyz, light.a);

    float3 diffuse;
    float3 specular;
    Shade(albedo, nn, vn, Ks, SPEC_EXPON, light, diffuse, specular);
    oColor0 = float4(diffuse + specular, IN.Color.w);
    
    float fogAlpha = saturate((G(FogParams).z - length(IN.View_Depth.xyz)) * G(FogParams).w);
    oColor0.xyz = lerp(G(FogColor), oColor0.rgb, fogAlpha);

    #ifdef PIN_GBUFFER
        oColor1 = gbufferPack(IN.View_Depth.w, diffuse.rgb, specular.rgb, fogAlpha);
    #endif
}
#else
#define CFG_TEXTURE_TILING              1

#define CFG_DIFFUSE_SCALE               1
#define CFG_SPECULAR_SCALE              2
#define CFG_GLOSS_SCALE                 256
#define CFG_REFLECTION_SCALE          	0

#define CFG_NORMAL_SHADOW_SCALE         0.3

#define CFG_SPECULAR_LOD				0.25
#define CFG_GLOSS_LOD					32

#define CFG_NORMAL_DETAIL_TILING		7
#define CFG_NORMAL_DETAIL_SCALE			0.6

#define CFG_FAR_TILING					0
#define CFG_FAR_DIFFUSE_CUTOFF			0
#define CFG_FAR_NORMAL_CUTOFF			0
#define CFG_FAR_SPECULAR_CUTOFF			0

#include "material.hlsl"
#endif