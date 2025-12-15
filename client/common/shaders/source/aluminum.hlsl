#ifdef CLASSIC
#define PIN_SURFACE
#define ALREADY_HAS_PS
#include "default.hlsl"

#define SPEC_EXPON 25
#define KS .9
#define KR .42

#define FADE_DIST 250//133.33 fix!!!!!!!!!!
#define FADE_DIST_START 20

void DefaultPS(VertexOutput IN,
#ifdef PIN_GBUFFER
	out float4 oColor1: COLOR1,
#endif
	out float4 oColor0: COLOR0)
{
	float fade = saturate(1 - (IN.View_Depth.w - FADE_DIST_START) / FADE_DIST);
	float3 nn = normalize(IN.Normal.xyz);

	float3x3 normalMatrix = {
		IN.Tangent.xyz,
		cross(nn, IN.Tangent.xyz),
		nn
	};

	float3 albedo = IN.Color.xyz;
	float2 uv = IN.Uv.xy * 2;
	float3 tn = tex2D(NormalMap, uv);
	float3 tn2 = tex2D(NormalMap, uv * .4);
	tn = lerp(tn, tn2, .5);
	tn2 = tex2D(NormalMap, uv * .1);
	tn = lerp(tn, tn2, .3) -.5;
	tn = lerp(float3(0, 0, .5), tn, fade);

	float tNormSum = 0.4 + 0.6 * (tn.x + tn.y + tn.z);
	albedo *= ((1-fade) + (fade*tNormSum));

	tn = CalcBevel(IN.EdgeDistances, tn, IN.View_Depth.w);

	nn = mul(tn, normalMatrix);

	float3 vn = normalize(IN.View_Depth.xyz);
	float4 light = lgridSample(TEXTURE(LightMap), TEXTURE(LightMapLookup), IN.LightPosition_Fog.xyz);
	light.a = shadowSample(TEXTURE(ShadowMap), IN.PosLightSpace.xyz, light.a);

	float3 diffuse;
	float3 specular;
	Shade(albedo, nn, vn, KS, SPEC_EXPON, light, diffuse, specular);
	float3 enviroment = texCUBE(EnvironmentMap, reflect(-vn, nn)) * KR -.1;

	oColor0 = float4(diffuse + specular + enviroment, IN.Color.w);
	//subtract .1 because thats what dumbass roblox did for whatever reason

	float fogAlpha = saturate((G(FogParams).z - length(IN.View_Depth.xyz)) * G(FogParams).w);
    oColor0.xyz = lerp(G(FogColor), oColor0.rgb, fogAlpha);

	#ifdef PIN_GBUFFER
	    oColor1 = gbufferPack(IN.View_Depth.w, diffuse.rgb, specular.rgb, fogAlpha);
	#endif
}
#else
#define CFG_TEXTURE_TILING              1

#define CFG_DIFFUSE_SCALE               1
#define CFG_SPECULAR_SCALE              1
#define CFG_GLOSS_SCALE                 256
#define CFG_REFLECTION_SCALE          	0.6

#define CFG_NORMAL_SHADOW_SCALE         0

#define CFG_SPECULAR_LOD				0.94
#define CFG_GLOSS_LOD					240

#define CFG_NORMAL_DETAIL_TILING		0
#define CFG_NORMAL_DETAIL_SCALE			0

#define CFG_FAR_TILING					0.25
#define CFG_FAR_DIFFUSE_CUTOFF			0
#define CFG_FAR_NORMAL_CUTOFF			0.75
#define CFG_FAR_SPECULAR_CUTOFF			0

#define CFG_OPT_DIFFUSE_CONST

#include "material.hlsl"
#endif