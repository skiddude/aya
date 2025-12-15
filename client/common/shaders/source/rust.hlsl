#ifdef CLASSIC
#define PIN_SURFACE
#define ALREADY_HAS_PS
#include "default.hlsl"

#define SPEC_EXPON 70
#define KS .12

#define NOISE_SCALE float3(.02083, .0693, .2083)
#define SPREAD .3
#define RUST_THRESHOLD .8
#define CONTRAST float3(.1, .1, .1)
#define STUPID 0.125 * 1 //they do this stupid thing where different colors have assigned values for this coord val!!!!!

#define FADE_DIST 250 //133.33 fix!!!!!!!!!!
#define FADE_DIST_START 0


void DefaultPS(VertexOutput IN,
#ifdef PIN_GBUFFER
	out float4 oColor1: COLOR1,
#endif
	out float4 oColor0: COLOR0)
{
    float fade = saturate(1 - (IN.View_Depth.w - FADE_DIST_START) / FADE_DIST);

	float3 nn = normalize(IN.Normal);

	float3x3 normalMatrix = {
		IN.Tangent.xyz,
		cross(nn, IN.Tangent.xyz),
		nn
	};

	float2 uv = IN.Uv.xy;

    float3 shiftPos = DisplaceCoord(IN.Pos);
	float noiseval = tex3D(SpecularMap,shiftPos.xyz*NOISE_SCALE.x).x * 0.5;
	float noiseval2 = tex3D(SpecularMap,shiftPos.zyx*NOISE_SCALE.y).x * 0.3;
	float noiseval3 = tex3D(SpecularMap,shiftPos.zyx*NOISE_SCALE.z).x * 0.2;
	noiseval += noiseval2+noiseval3;

    float3 metalColor = IN.Color.xyz * 1.3 + CONTRAST * (noiseval - .5);
	float3 rustColor = tex2D(DiffuseMap, float2(STUPID, 1 - noiseval)).xyz;
	
    float3 tn = tex2D(NormalMap,uv).xyz - .5;
	float tnSum = 0.65 + 0.35 * (tn.x + tn.y + tn.z);

    float interp = (noiseval - RUST_THRESHOLD + SPREAD) / 2 / SPREAD + .5;
	interp = clamp(interp, 0, 1);
	tn = lerp(tn, float3(0,0,.5), interp - .4);
	float3 albedo = lerp(rustColor, metalColor,interp);
	float3 albedo2 = albedo * tnSum;
	albedo = lerp(albedo, albedo2, interp);
	tn = lerp(tn, float3(0,0,.5), interp);

    tn = CalcBevel(IN.EdgeDistances, tn, IN.View_Depth.w);
	nn = mul(tn, normalMatrix);
    
	float3 vn = normalize(IN.View_Depth.xyz);
	float4 light = lgridSample(TEXTURE(LightMap), TEXTURE(LightMapLookup), IN.LightPosition_Fog.xyz);
	light.a = shadowSample(TEXTURE(ShadowMap), IN.PosLightSpace.xyz, light.a);

	float3 diffuse;
	float3 specular;
	Shade(albedo, nn, vn, KS, SPEC_EXPON, light, diffuse, specular);

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
#define CFG_SPECULAR_SCALE              1
#define CFG_GLOSS_SCALE                 256
#define CFG_REFLECTION_SCALE          	0

#define CFG_NORMAL_SHADOW_SCALE         0.5

#define CFG_SPECULAR_LOD				0.35
#define CFG_GLOSS_LOD					103

#define CFG_NORMAL_DETAIL_TILING		0
#define CFG_NORMAL_DETAIL_SCALE			0

#define CFG_FAR_TILING					0.5
#define CFG_FAR_DIFFUSE_CUTOFF			0.6
#define CFG_FAR_NORMAL_CUTOFF			0
#define CFG_FAR_SPECULAR_CUTOFF			0

#define CFG_OPT_BLEND_COLOR

#include "material.hlsl"
#endif