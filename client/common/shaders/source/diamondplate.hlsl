#ifdef CLASSIC
#define PIN_SURFACE
#define ALREADY_HAS_PS
#include "default.hlsl"

#define SPEC_EXPON 25
#define KS .9
#define MIN_BACKLIGHT .9

void DefaultPS(VertexOutput IN,
#ifdef PIN_GBUFFER
	out float4 oColor1: COLOR1,
#endif
	out float4 oColor0: COLOR0)
{
	float3 normal = normalize(IN.Normal);

	float3x3 normalMatrix = {
		IN.Tangent.xyz,
		cross(normal, IN.Tangent.xyz),
		normal
	};

	float2 uv = IN.Uv.xy * 6; // * 5 * 1.2

	float3 tn = tex2D(NormalMap, uv).yxz - .5;

    tn = lerp(tn, float3(0,0,.5), .7);
	
	tn = CalcBevel(IN.EdgeDistances, tn, IN.View_Depth.w);

	float3 nn = normalize(mul(tn, normalMatrix));

	float3 vn = normalize(IN.View_Depth.xyz);
	float4 light = lgridSample(TEXTURE(LightMap), TEXTURE(LightMapLookup), IN.LightPosition_Fog.xyz);
	light.a = shadowSample(TEXTURE(ShadowMap), IN.PosLightSpace.xyz, light.a);

	float3 diffuse;
	float3 specular;
	Shade(IN.Color.xyz, nn, vn, KS, SPEC_EXPON, light, diffuse, specular);

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
#define CFG_SPECULAR_SCALE              2.7
#define CFG_GLOSS_SCALE                 256
#define CFG_REFLECTION_SCALE          	0

#define CFG_NORMAL_SHADOW_SCALE         0.5

#define CFG_SPECULAR_LOD				0.9
#define CFG_GLOSS_LOD					160

#define CFG_NORMAL_DETAIL_TILING		0
#define CFG_NORMAL_DETAIL_SCALE			0

#define CFG_FAR_TILING					0
#define CFG_FAR_DIFFUSE_CUTOFF			0
#define CFG_FAR_NORMAL_CUTOFF			0
#define CFG_FAR_SPECULAR_CUTOFF			0

#include "material.hlsl"
#endif