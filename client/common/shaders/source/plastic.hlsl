#ifdef CLASSIC
#ifdef PIN_HQ
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

uniform float4 WaveParams; // .x = frequency  .y = phase  .z = height  .w = lerp

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
	float2 uv;

	float wt = 1.0;
	#ifndef PIN_MESH
		uv = IN.UvStuds;
		
		float3x3 normalMatrix = {
			IN.Tangent.xyz,
			cross(nn, IN.Tangent.xyz),
			nn
		};
		//tn = tex2D(NormalMap, uv) - .5;//studs normal
		float3 tn = float3(0,0,.5);
		tn = lerp(float3(0, 0, .5), tn, fade);
		tn = CalcBevel(edgeDistances, tn, IN.View_Depth.w);
    	nn = mul(tn, normalMatrix);
		wt = 1.0 - abs(length(tn.xy));
		float4 colorTex = tex2D(DiffuseMap, uv);

		//albedo = lerp(albedo, colorTex.xyz, colorTex.w);//studs normal
		//albedo += (colorTex.x * 2 - 1) * .07;//studs normal
	#else
		uv = IN.Uv;
		float4 colorTex = tex2D(DiffuseMap, uv);
		albedo *= colorTex;
	#endif

	float3 vn = normalize(IN.View_Depth.xyz);
	float4 light = lgridSample(TEXTURE(LightMap), TEXTURE(LightMapLookup), IN.LightPosition_Fog.xyz);

	float3 diffuse;
	float3 specular;

#ifdef CLASSIC_GOURAUD
	float3 diffusePhong;
	float3 specularPhong;

	Shade(albedo.rgb, nn, vn, KS, SPEC_EXPON, light, diffusePhong, specularPhong);
    diffuse = lerp(diffusePhong, IN.Diffuse * albedo.rgb, wt);
    specular = lerp(specularPhong, IN.Specular, wt);
#else
	Shade(albedo.rgb, nn, vn, KS, SPEC_EXPON, light, diffuse, specular);
#endif
	#ifndef PIN_MESH
		diffuse = lerp(diffuse, colorTex.xyz, colorTex.w);
	#endif
	
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
#else

#if defined(PIN_HQ)
#define PIN_SURFACE
#include "default.hlsl"

#define CFG_TEXTURE_TILING              1

#define CFG_BUMP_INTENSITY              0.5

#define CFG_SPECULAR					0.4
#define CFG_GLOSS						9

#define CFG_NORMAL_SHADOW_SCALE         0.1

Surface surfaceShader(SurfaceInput IN, float2 fade2)
{
	float fade = fade2.y;
	
    float4 studs = tex2D(DiffuseMap, IN.UvStuds);
    float3 normal = nmapUnpack(tex2D(NormalMap, IN.UvStuds));

#ifdef GLSLES
    float3 noise = float3(0, 0, 1);
#else
    float3 noise = nmapUnpack(tex2D(NormalDetailMap, IN.Uv * (CFG_TEXTURE_TILING)));
#endif

    float noiseScale = saturate0(IN.Color.a * 2 * (CFG_BUMP_INTENSITY) - 1 * (CFG_BUMP_INTENSITY));

#ifdef PIN_REFLECTION
    noiseScale *= saturate(1 - 2 * IN.Reflectance);
#endif

    normal.xy += noise.xy * noiseScale;

    normal.xy *= fade;

    Surface surface = (Surface)0;
    surface.albedo = IN.Color.rgb * (studs.r * 2);
    surface.normal = normal;
    surface.specular = (CFG_SPECULAR);
    surface.gloss = (CFG_GLOSS);

#ifdef PIN_REFLECTION
    surface.reflectance = IN.Reflectance;
#endif

	return surface;
}
#else
#define PIN_PLASTIC
#include "default.hlsl"
#endif
#endif