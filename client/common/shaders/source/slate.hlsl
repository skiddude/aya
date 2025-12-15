#ifdef CLASSIC
#define PIN_SURFACE
#define ALREADY_HAS_PS
#include "default.hlsl"

#define SPEC_EXPON 40
#define KS .1

#define NOISE_SCALE 7
#define SPREAD .3
#define GRASS_THRESHOLD .95

#define FADE_DIST 290
#define FADE_DIST_START 0

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
		normal,
	};

	float fade = saturate(1 - (IN.View_Depth.w - FADE_DIST_START) / FADE_DIST);

	float2 uv = IN.Uv.xy;
    float3 shiftPos = IN.Pos.xyz;

    float noiseval = tex3D(SpecularMap,shiftPos.xyz / NOISE_SCALE * .04).x;
	float noiseval2 = tex3D(SpecularMap,shiftPos.xyz / NOISE_SCALE * .3).x + .2;
	noiseval *= noiseval2;

	float3 albedo = IN.Color.xyz + 0.8 * fade * (noiseval * 0.5 - 0.1);

    float3 tn = tex2D(NormalMap,uv).xyz - .5;
	float tNormSum = 0.9+0.4*(tn.x + tn.y + tn.z);
	tn = lerp(tn, float3(0,0,.5), .9);
	albedo *= ((1-fade) + (fade*tNormSum));

    tn = CalcBevel(IN.EdgeDistances, tn, IN.View_Depth.w);
	float3 nn = mul(tn, normalMatrix);
    //nn *= fade;
	//KS *= fade; //ks is a constant and idk if this dumb shit is worth keeping anyways

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
#define CFG_SPECULAR_SCALE              0.9
#define CFG_GLOSS_SCALE                 128
#define CFG_REFLECTION_SCALE          	0

#define CFG_NORMAL_SHADOW_SCALE         0.5

#define CFG_SPECULAR_LOD				0.14
#define CFG_GLOSS_LOD					20

#define CFG_NORMAL_DETAIL_TILING		5
#define CFG_NORMAL_DETAIL_SCALE			1

#define CFG_FAR_TILING					0.25
#define CFG_FAR_DIFFUSE_CUTOFF			0.75
#define CFG_FAR_NORMAL_CUTOFF			0
#define CFG_FAR_SPECULAR_CUTOFF			0

#include "material.hlsl"
#endif