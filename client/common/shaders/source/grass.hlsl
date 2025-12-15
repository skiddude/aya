#ifdef CLASSIC
#define PIN_SURFACE
#define ALREADY_HAS_PS
#include "default.hlsl"

#define SPEC_EXPON 50
#define KS .1

#define NOISE_SCALE float3(0.09, 0.02, 0.004);
#define SPREAD .3
#define GRASS_THRESHOLD .95

#define FADE_DIST 300//133.33 fix!!!!!!!!!!
#define FADE_DIST_START 20

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

	float fade = saturate(1 - (IN.View_Depth.w - FADE_DIST_START) / FADE_DIST);

	float2 uv = IN.Uv.xy;

    float3 shiftPos = DisplaceCoord(IN.Pos.xyz);
	float3 ns = NOISE_SCALE;
    float noiseval2 = tex3D(SpecularMap,shiftPos.xyz*ns.x).x * 0.4;
	float noiseval = tex3D(SpecularMap,shiftPos.zyx*ns.y).x * 0.6;
	noiseval -= noiseval2;
	float noiseval3 = tex3D(SpecularMap,shiftPos.xyz*ns.z).x * 0.3;
	noiseval += noiseval3;

    float interp = (noiseval - GRASS_THRESHOLD + SPREAD)/2/SPREAD+0.5;
	interp = clamp(interp,0,1);

    float3 grassColor = tex2D(DiffuseMap, uv).xyz;
	float3 dirt = tex2D(NormalMap,uv).xyz;

    float3 albedo = IN.Color.xyz;
    albedo = lerp(grassColor + IN.Color.xyz - float3(0.31,0.43,0.146), dirt, interp * fade);

	float3 grassNorm = tex2D(StudsMap,uv).xyz;
	float3 dirtNorm = float3(0.5,0.5,1);
	float3 tn = lerp(grassNorm, dirtNorm, interp) - 0.5;
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

#define CFG_SPECULAR_LOD				0.17
#define CFG_GLOSS_LOD					18

#define CFG_NORMAL_DETAIL_TILING		0
#define CFG_NORMAL_DETAIL_SCALE			0

#define CFG_FAR_TILING					0.25
#define CFG_FAR_DIFFUSE_CUTOFF			0.6
#define CFG_FAR_NORMAL_CUTOFF			0
#define CFG_FAR_SPECULAR_CUTOFF			0

#define CFG_OPT_BLEND_COLOR

#include "material.hlsl"
#endif