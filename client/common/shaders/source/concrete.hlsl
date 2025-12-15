#ifdef CLASSIC
#define ALREADY_HAS_PS
#include "default.hlsl"

#define SPEC_EXPON 81
#define KS .75


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

	nn = CalcBevel(IN.EdgeDistances, float3(0,0,.5), IN.View_Depth.w);
	nn = mul(nn, normalMatrix);

	float2 uv = IN.Uv.xy;
	float3 concrete = .5 * (tex2D(DiffuseMap, uv).xyz * 2 - 1);
	
	float3 albedo = IN.Color.xyz;
	float3 vn = normalize(IN.View_Depth.xyz);
	float4 light = lgridSample(TEXTURE(LightMap), TEXTURE(LightMapLookup), IN.LightPosition_Fog.xyz);

	float3 diffuse;
	float3 specular;
	albedo += concrete;

	Shade(albedo, nn, vn, KS, SPEC_EXPON, light, diffuse, specular);
    //diffuse += concrete; //this is accurate but stupid dont know if i should keep

	oColor0 = float4(diffuse, IN.Color.w);

	float fogAlpha = saturate((G(FogParams).z - length(IN.View_Depth.xyz)) * G(FogParams).w);
    oColor0.xyz = lerp(G(FogColor), oColor0.rgb, fogAlpha);

	#ifdef PIN_GBUFFER
	    oColor1 = gbufferPack(IN.View_Depth.w, diffuse.rgb, float3(0,0,0), fogAlpha);
	#endif
}
#else
#define CFG_TEXTURE_TILING              1

#define CFG_DIFFUSE_SCALE               1
#define CFG_SPECULAR_SCALE              1.3
#define CFG_GLOSS_SCALE                 128
#define CFG_REFLECTION_SCALE          	0

#define CFG_NORMAL_SHADOW_SCALE         0

#define CFG_SPECULAR_LOD				0.07
#define CFG_GLOSS_LOD					22

#define CFG_NORMAL_DETAIL_TILING		10
#define CFG_NORMAL_DETAIL_SCALE			1

#define CFG_FAR_TILING					0.25
#define CFG_FAR_DIFFUSE_CUTOFF			0.75
#define CFG_FAR_NORMAL_CUTOFF			0
#define CFG_FAR_SPECULAR_CUTOFF			0

#define CFG_OPT_NORMAL_CONST

#include "material.hlsl"
#endif