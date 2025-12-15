#ifdef CLASSIC
#include "common.h"

#define LQMAT_FADE_FACTOR (1.0f/300.0f)
#define BEVEL_WIDTH .06
#define PI 3.14159265359

#define BEVEL_FADE_DIST 120
#define BEVEL_FADE_DIST_START 0

struct Appdata
{
    float4 Position     : POSITION;
    float3 Normal       : NORMAL;
    float2 Uv           : TEXCOORD0;
    float2 UvStuds      : TEXCOORD1;

    float4 Color : COLOR0;
    ATTR_INT4 Extra : COLOR1;

    float3 Tangent       : TEXCOORD2;

    float4 EdgeDistances : TEXCOORD3;
};

struct VertexOutput
{
    float2 Uv : TEXCOORD0;
	float2 UvStuds : TEXCOORD1;

    float4 Color : COLOR0;
    float3 Normal : TEXCOORD2;
    float3 Tangent : TEXCOORD3;
    float3 Pos : TEXCOORD4;
    float3 WorldPos : TEXCOORD5;
    float4 LightPosition_Fog : TEXCOORD6;

    float4 View_Depth : TEXCOORD7;
  
    float3 PosLightSpace : TEXCOORD8;

    float4 EdgeDistances : TEXCOORD9;
    float Reflection : TEXCOORD10;
#ifdef CLASSIC_GOURAUD
    float3 Diffuse : TEXCOORD11;
    float3 Specular : TEXCOORD12;
#endif
};

#ifdef PIN_SKINNED
    WORLD_MATRIX_ARRAY(WorldMatrixArray, MAX_BONE_COUNT * 3);
#endif

#ifdef PIN_DEBUG
uniform float4 DebugColor;
#endif

void Shade(float3 albedo, float3 nn, float3 vn, float ks, float specExpon, float4 light, out float3 diffuse, out float3 specular)
{
    float3 ln = normalize(-G(Lamp0Dir));
	float3 hn = normalize(ln + vn);
	float ndl = dot(nn, ln);
	float ndh = dot(nn, hn);

    float2 lit0 = lit(ndl, ndh, specExpon).yz;
    float3 lampColor = G(Lamp0Color);
	diffuse = (light.a * (lampColor * lit0.x) + G(AmbientColor) + light.xyz);
    //diffuse += light.a * max(-ndl, 0) * G(Lamp1Color); //the bitch ass light that shines on the backfacing normals remove for less ugly but less robloxeses accuracy
    diffuse *= albedo;
	specular = (lampColor * lit0.y) * ks * light.a;
}

float3 CalcBevel(float4 edgeDistances, float3 normal, float viewDepth)
{
    float4 bevelMultiplier = saturate(BEVEL_WIDTH - edgeDistances) / BEVEL_WIDTH;
    float fade = saturate(1 - (viewDepth - BEVEL_FADE_DIST_START) / BEVEL_FADE_DIST);
    bevelMultiplier *= fade;

    normal += bevelMultiplier.x * float3(.5,0,0);
    normal += bevelMultiplier.y * float3(-.5,0,0);
    normal += bevelMultiplier.z * float3(0,.5,0);
    normal += bevelMultiplier.w * float3(0,-.5,0);

	return normalize(normal);
}

float3 DisplaceCoord(float3 pos) //theres a fourth component but later idk
{
    float cfactor = 0.980066578; 	//cos(0.2);
    float sfactor = 0.198669331; 	//sin(0.2);
    float cfactor2 = 0.955336489;	//cos(0.3);
    float sfactor2 = 0.295520207; 	//sin(0.3);
    float cfactor3 = 0.921060994;	//cos(0.4);
    float sfactor3 = 0.389418342;	//sin(0.4);
    float3 p = pos.xyz;
    float3 shiftPos = p;

    shiftPos.x += p.x * cfactor + p.z * sfactor;
    shiftPos.z += p.x * -sfactor + p.z * cfactor;
    
    shiftPos.x += p.x * cfactor2 - p.y * sfactor2;
    shiftPos.y += p.x * sfactor2 + p.y * cfactor2;
    
    shiftPos.y += p.y * cfactor3 - p.z * sfactor3;
    shiftPos.z += p.y * sfactor3 + p.z * cfactor3;
    
    return shiftPos; //return float4(shiftPos,IN.TexPos3D.w);
}

TEX_DECLARE2D(StudsMap, 0);

LGRID_SAMPLER(LightMap, 1);
TEX_DECLARE2D(LightMapLookup, 2);

TEX_DECLARE2D(ShadowMap, 3);

TEX_DECLARECUBE(EnvironmentMap, 4);

TEX_DECLARE2D(DiffuseMap, 5);
TEX_DECLARE2D(NormalMap, 6);
TEX_DECLARE3D(SpecularMap, 7);

#ifndef GLSLES
TEX_DECLARE2D(NormalDetailMap, 8);
#endif

VertexOutput DefaultVS(Appdata IN, out float4 HPosition: POSITION)
{
    VertexOutput OUT = (VertexOutput)0;

    // Transform position and normal to world space
#ifdef PIN_SKINNED
    int boneIndex = IN.Extra.r;

    float4 worldRow0 = WorldMatrixArray[boneIndex * 3];
    float4 worldRow1 = WorldMatrixArray[boneIndex * 3 + 1];
    float4 worldRow2 = WorldMatrixArray[boneIndex * 3 + 2];

    float3 posWorld = float3(dot(worldRow0, IN.Position), dot(worldRow1, IN.Position), dot(worldRow2, IN.Position));
    float3 normalWorld = float3(dot(worldRow0.xyz, IN.Normal), dot(worldRow1.xyz, IN.Normal), dot(worldRow2.xyz, IN.Normal));
    OUT.Tangent = float3(dot(worldRow0.xyz, IN.Tangent), dot(worldRow1.xyz, IN.Tangent), dot(worldRow2.xyz, IN.Tangent));
#else
    //OUT.Pos = mul(transpose(worldMatrix), IN.Position).xyz; // IN.position
	float3 posWorld = IN.Position.xyz;
    float3 normalWorld = IN.Normal;
    OUT.Tangent = IN.Tangent;
#endif
    OUT.Pos = IN.Position.xyz;
    OUT.WorldPos = posWorld;

    //posWorld.xy += sin(posWorld.yx * .05) * 5;
    //float scale = 6;
    //posWorld.y *= 6;
    //posWorld.y -= G(CameraPosition).y * (scale - 1);
    float4 color = IN.Color;

	HPosition = mul(G(ViewProjection), float4(posWorld, 1));
    OUT.Normal = normalWorld;
	OUT.Uv = IN.Uv;
	OUT.UvStuds.xy = IN.UvStuds;
	
    OUT.Color = color;
    OUT.LightPosition_Fog = float4(lgridPrepareSample(lgridOffset(posWorld, normalWorld)), (G(FogParams).z - HPosition.w) * G(FogParams).w);

	OUT.View_Depth = float4(G(CameraPosition).xyz - posWorld, HPosition.w);

#ifdef CLASSIC_GOURAUD
    float4 albedo = color;

    float4 light = lgridSample(TEXTURE(LightMap), TEXTURE(LightMapLookup), OUT.LightPosition_Fog.xyz);
    Shade(float3(1,1,1), normalize(OUT.Normal), normalize(OUT.View_Depth.xyz), .71, 81,  light, OUT.Diffuse, OUT.Specular);
#endif
    #if defined(PIN_HQ) || defined(PIN_REFLECTION)
	    //float4 edgeDistances = IN.EdgeDistances*G(FadeDistance_GlowFactor).z + 0.5 * OUT.View_Depth.w  * G(FadeDistance_GlowFactor).y;

        OUT.Reflection = IN.Extra.a / 255.f;
        OUT.EdgeDistances = IN.EdgeDistances;
    #endif

    OUT.PosLightSpace.xyz = shadowPrepareSample(posWorld);

	return OUT;
}

#ifdef PIN_SURFACE
struct SurfaceInput
{
    float4 Color;
    float2 Uv;
    float2 UvStuds;

#ifdef PIN_REFLECTION
    float Reflectance;
#endif
};

struct Surface
{
    float3 albedo;
    float3 normal;
    float specular;
    float gloss;
    float reflectance;
};

Surface surfaceShader(SurfaceInput IN, float2 fade);

Surface surfaceShaderExec(VertexOutput IN)
{
    SurfaceInput SIN;
    SIN.Color = IN.Color;
    SIN.Uv = IN.Uv.xy;
    SIN.UvStuds = IN.UvStuds.xy;

    #ifdef PIN_REFLECTION
        SIN.Reflectance = IN.Reflection;
    #endif

    float2 fade;
	fade.x = saturate0(1 - IN.View_Depth.w * LQMAT_FADE_FACTOR );
	fade.y = saturate0(1 - IN.View_Depth.w * G(FadeDistance_GlowFactor).y );

    return surfaceShader(SIN, fade);
}
#endif

uniform float4 LqmatFarTilingFactor; // material tiling factor for low-quality shader, must be the same as CFG_FAR_TILING


float4 sampleFar1(TEXTURE_IN_2D(s), float2 uv, float fade, float cutoff)
{
#ifdef GLSLES
    return tex2D(s, uv);
#else
    if (cutoff == 0)
        return tex2D(s, uv);
    else
    {
        float cscale = 1 / (1 - cutoff);

        return lerp(tex2D(s, uv * (LqmatFarTilingFactor.xy) ), tex2D(s, uv ), saturate0(fade * cscale - cutoff * cscale));
    }
#endif
}

//#if defined(PIN_WANG) || defined(PIN_WANG_FALLBACK)
float4 sampleWangSimple(TEXTURE_IN_2D(s), float2 uv)
{
    float2 wangUv;
    float4 wangUVDerivatives;
    getWang(TEXTURE_WANG(NormalDetailMap), uv, 1, wangUv, wangUVDerivatives);
    return sampleWang(TEXTURE(s), wangUv, wangUVDerivatives);
}
//#endif


#ifndef ALREADY_HAS_PS

#define SPEC_EXPON 81 //81
#define KS .75

#define FADE_DIST 500
#define FADE_DIST_START 0

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

	#ifndef PIN_MESH
		uv = IN.Uv;
		
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
		
		float4 colorTex = tex2D(DiffuseMap, uv);
        albedo *= colorTex;

		//albedo = lerp(albedo, colorTex.xyz, colorTex.w);//studs normal
		//albedo += (colorTex.x * 2 - 1) * .07;//studs normal
	#else
		uv = IN.Uv;
		float4 colorTex = tex2D(DiffuseMap, uv);
		albedo *= colorTex;
	#endif

	float3 vn = normalize(IN.View_Depth.xyz);
	float4 light = lgridSample(TEXTURE(LightMap), TEXTURE(LightMapLookup), IN.LightPosition_Fog.xyz);

	float3 diffusePhong;
	float3 diffuse;
	float3 specularPhong;
	float3 specular;
#ifdef CLASSIC_GOURAUD
    diffuse = lerp(diffusePhong, IN.Diffuse * albedo.rgb, 1.0);
    specular = lerp(specularPhong, IN.Specular, 1.0);
#else
	Shade(albedo.rgb, nn, vn, KS, SPEC_EXPON, light, diffusePhong, specularPhong);
    diffuse = diffusePhong;
    specular = specularPhong;
#endif

	#ifndef PIN_MESH
		//diffuse = lerp(diffuse, colorTex.xyz, colorTex.w);
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

#endif
#else
#include "common.h"

#define LQMAT_FADE_FACTOR (1.0f/300.0f)

struct Appdata
{
    float4 Position     : POSITION;
    float3 Normal       : NORMAL;
    float2 Uv           : TEXCOORD0;
    float2 UvStuds      : TEXCOORD1;

    float4 Color : COLOR0;
    ATTR_INT4 Extra : COLOR1;

#ifdef PIN_SURFACE
    float3 Tangent       : TEXCOORD2;
#endif
    float4 EdgeDistances : TEXCOORD3;
};

struct VertexOutput
{
    float4 Uv_EdgeDistance1 : TEXCOORD0;
	float4 UvStuds_EdgeDistance2 : TEXCOORD1;

    float4 Color             : COLOR0;
    float4 LightPosition_Fog : TEXCOORD2;

    float4 View_Depth : TEXCOORD3;
	#if defined(PIN_HQ) || defined(PIN_REFLECTION)
        float4 Normal_SpecPower  : TEXCOORD4;
    #endif
  
    #ifdef PIN_SURFACE
        float3 Tangent  : TEXCOORD5;
    #else
        float4 Diffuse_Specular  : COLOR1;
    #endif

    float4 PosLightSpace_Reflectance: TEXCOORD6;
};

#ifdef PIN_SKINNED
    WORLD_MATRIX_ARRAY(WorldMatrixArray, MAX_BONE_COUNT * 3);
#endif

#ifdef PIN_DEBUG
uniform float4 DebugColor;
#endif

VertexOutput DefaultVS(Appdata IN, out float4 HPosition: POSITION)
{
    VertexOutput OUT = (VertexOutput)0;

    // Transform position and normal to world space
#ifdef PIN_SKINNED
    int boneIndex = IN.Extra.r;

    float4 worldRow0 = WorldMatrixArray[boneIndex * 3 + 0];
    float4 worldRow1 = WorldMatrixArray[boneIndex * 3 + 1];
    float4 worldRow2 = WorldMatrixArray[boneIndex * 3 + 2];
		
	float3 posWorld = float3(dot(worldRow0, IN.Position), dot(worldRow1, IN.Position), dot(worldRow2, IN.Position));
    float3 normalWorld = float3(dot(worldRow0.xyz, IN.Normal), dot(worldRow1.xyz, IN.Normal), dot(worldRow2.xyz, IN.Normal));
#else
	float3 posWorld = IN.Position.xyz;
    float3 normalWorld = IN.Normal;
#endif

    // Decode diffuse/specular parameters; encoding depends on the skinned flag due to vertex declaration differences
#if defined(PIN_DEBUG)
    float4 color = DebugColor;
#else
    float4 color = IN.Color;
#endif

    float specularIntensity = IN.Extra.g / 255.f;
    float specularPower = IN.Extra.b;

    float ndotl = dot(normalWorld, -G(Lamp0Dir));

#ifdef PIN_HQ
    // We'll calculate specular in pixel shader
    float2 lt = float2(saturate(ndotl), (ndotl > 0));
#else
    // Using lit here improves performance on software vertex shader implementations
    float2 lt = lit(ndotl, dot(normalize(-G(Lamp0Dir) + normalize(G(CameraPosition).xyz - posWorld.xyz)), normalWorld), specularPower).yz;
#endif

	HPosition = mul(G(ViewProjection), float4(posWorld, 1));

	OUT.Uv_EdgeDistance1.xy = IN.Uv;
	OUT.UvStuds_EdgeDistance2.xy = IN.UvStuds;
	
    OUT.Color = color;
    OUT.LightPosition_Fog = float4(lgridPrepareSample(lgridOffset(posWorld, normalWorld)), (G(FogParams).z - HPosition.w) * G(FogParams).w);

	OUT.View_Depth = float4(G(CameraPosition).xyz - posWorld, HPosition.w);

    #if defined(PIN_HQ) || defined(PIN_REFLECTION)
	    float4 edgeDistances = IN.EdgeDistances*G(FadeDistance_GlowFactor).z + 0.5 * OUT.View_Depth.w  * G(FadeDistance_GlowFactor).y;
		
	    OUT.Uv_EdgeDistance1.zw = edgeDistances.xy;
	    OUT.UvStuds_EdgeDistance2.zw = edgeDistances.zw;
        OUT.Normal_SpecPower = float4(normalWorld, specularPower);
        OUT.PosLightSpace_Reflectance.w = IN.Extra.a / 255.f;
    #endif

    #ifdef PIN_SURFACE
        #ifdef PIN_SKINNED
            float3 tangent = float3(dot(worldRow0.xyz, IN.Tangent), dot(worldRow1.xyz, IN.Tangent), dot(worldRow2.xyz, IN.Tangent));
        #else
            float3 tangent = IN.Tangent;
        #endif

        OUT.Tangent = tangent;
    #else
        float3 diffuse = lt.x * G(Lamp0Color) + max(-ndotl, 0) * G(Lamp1Color);

        OUT.Diffuse_Specular = float4(diffuse, lt.y * specularIntensity);
    #endif

    OUT.PosLightSpace_Reflectance.xyz = shadowPrepareSample(posWorld);

	return OUT;
}

#ifdef PIN_SURFACE
struct SurfaceInput
{
    float4 Color;
    float2 Uv;
    float2 UvStuds;

#ifdef PIN_REFLECTION
    float Reflectance;
#endif
};

struct Surface
{
    float3 albedo;
    float3 normal;
    float specular;
    float gloss;
    float reflectance;
};

Surface surfaceShader(SurfaceInput IN, float2 fade);

Surface surfaceShaderExec(VertexOutput IN)
{
    SurfaceInput SIN;
    SIN.Color = IN.Color;
    SIN.Uv = IN.Uv_EdgeDistance1.xy;
    SIN.UvStuds = IN.UvStuds_EdgeDistance2.xy;

    #ifdef PIN_REFLECTION
        SIN.Reflectance = IN.PosLightSpace_Reflectance.w;
    #endif

    float2 fade;
	fade.x = saturate0(1 - IN.View_Depth.w * LQMAT_FADE_FACTOR );
	fade.y = saturate0(1 - IN.View_Depth.w * G(FadeDistance_GlowFactor).y );

    return surfaceShader(SIN, fade);
}
#endif

TEX_DECLARE2D(StudsMap, 0);

LGRID_SAMPLER(LightMap, 1);
TEX_DECLARE2D(LightMapLookup, 2);

TEX_DECLARE2D(ShadowMap, 3);

TEX_DECLARECUBE(EnvironmentMap, 4);

TEX_DECLARE2D(DiffuseMap, 5);
TEX_DECLARE2D(NormalMap, 6);
TEX_DECLARE2D(SpecularMap, 7);

#ifndef GLSLES
TEX_DECLARE2D(NormalDetailMap, 8);
#endif

uniform float4 LqmatFarTilingFactor; // material tiling factor for low-quality shader, must be the same as CFG_FAR_TILING


float4 sampleFar1(TEXTURE_IN_2D(s), float2 uv, float fade, float cutoff)
{
#ifdef GLSLES
    return tex2D(s, uv);
#else
    if (cutoff == 0)
        return tex2D(s, uv);
    else
    {
        float cscale = 1 / (1 - cutoff);

        return lerp(tex2D(s, uv * (LqmatFarTilingFactor.xy) ), tex2D(s, uv ), saturate0(fade * cscale - cutoff * cscale));
    }
#endif
}

#if defined(PIN_WANG) || defined(PIN_WANG_FALLBACK)
float4 sampleWangSimple(TEXTURE_IN_2D(s), float2 uv)
{
    float2 wangUv;
    float4 wangUVDerivatives;
    getWang(TEXTURE_WANG(NormalDetailMap), uv, 1, wangUv, wangUVDerivatives);
    return sampleWang(TEXTURE(s), wangUv, wangUVDerivatives);
}
#endif

void DefaultPS(VertexOutput IN,
#ifdef PIN_GBUFFER
    out float4 oColor1: COLOR1,
#endif
    out float4 oColor0: COLOR0)
{
    // Compute albedo term
#ifdef PIN_SURFACE
    Surface surface = surfaceShaderExec(IN);

    float4 albedo = float4(surface.albedo, IN.Color.a);

    float3 bitangent = cross(IN.Normal_SpecPower.xyz, IN.Tangent.xyz);
    float3 normal = normalize(surface.normal.x * IN.Tangent.xyz + surface.normal.y * bitangent + surface.normal.z * IN.Normal_SpecPower.xyz);

    float ndotl = dot(normal, -G(Lamp0Dir));

    float3 diffuseIntensity = saturate0(ndotl) * G(Lamp0Color) + max(-ndotl, 0) * G(Lamp1Color);
    float specularIntensity = step(0, ndotl) * surface.specular;
    float specularPower = surface.gloss;

    float reflectance = surface.reflectance;
#elif PIN_LOWQMAT

#ifndef CFG_FAR_DIFFUSE_CUTOFF
#define CFG_FAR_DIFFUSE_CUTOFF (0.6f)
#endif

#if defined(PIN_WANG) || defined(PIN_WANG_FALLBACK)
	float4 albedo = sampleWangSimple(TEXTURE(DiffuseMap), IN.Uv_EdgeDistance1.xy);
#else
    float fade = saturate0(1 - IN.View_Depth.w * LQMAT_FADE_FACTOR);
    float4 albedo = sampleFar1(TEXTURE(DiffuseMap), IN.Uv_EdgeDistance1.xy, fade, CFG_FAR_DIFFUSE_CUTOFF);
#endif

	albedo.rgb = lerp(float3(1, 1, 1), IN.Color.rgb, albedo.a ) * albedo.rgb;
	albedo.a   = IN.Color.a;

	float3 diffuseIntensity = IN.Diffuse_Specular.xyz;
    float specularIntensity = IN.Diffuse_Specular.w;
	float reflectance = 0;

#else
    #ifdef PIN_PLASTIC
        float4 studs = tex2D(StudsMap, IN.UvStuds_EdgeDistance2.xy);
        float4 albedo = float4(IN.Color.rgb * (studs.r * 2), IN.Color.a);
	#else
        float4 albedo = tex2D(DiffuseMap, IN.Uv_EdgeDistance1.xy) * IN.Color;
	#endif

    #ifdef PIN_HQ
        float3 normal = normalize(IN.Normal_SpecPower.xyz);
        float specularPower = IN.Normal_SpecPower.w;
    #elif defined(PIN_REFLECTION)
        float3 normal = IN.Normal_SpecPower.xyz;
    #endif

    float3 diffuseIntensity = IN.Diffuse_Specular.xyz;
    float specularIntensity = IN.Diffuse_Specular.w;

    #ifdef PIN_REFLECTION
        float reflectance = IN.PosLightSpace_Reflectance.w;
    #endif

#endif

    float4 light = lgridSample(TEXTURE(LightMap), TEXTURE(LightMapLookup), IN.LightPosition_Fog.xyz);
	float shadow = shadowSample(TEXTURE(ShadowMap), IN.PosLightSpace_Reflectance.xyz, light.a);

    // Compute reflection term
#if defined(PIN_SURFACE) || defined(PIN_REFLECTION)
    float3 reflection = texCUBE(EnvironmentMap, reflect(-IN.View_Depth.xyz, normal)).rgb;

    albedo.rgb = lerp(albedo.rgb, reflection.rgb, reflectance);
#endif
    
    // Compute diffuse term
    float3 diffuse = (G(AmbientColor) + diffuseIntensity * shadow + light.rgb) * albedo.rgb;

    // Compute specular term
#ifdef PIN_HQ
    float3 specular = G(Lamp0Color) * (specularIntensity * shadow * (float)(half)pow(saturate(dot(normal, normalize(-G(Lamp0Dir) + normalize(IN.View_Depth.xyz)))), specularPower));
#else
    float3 specular = G(Lamp0Color) * (specularIntensity * shadow);
#endif

    // Combine
    oColor0.rgb = diffuse.rgb + specular.rgb;
    oColor0.a   = albedo.a;

#ifdef PIN_HQ
	float ViewDepthMul = IN.View_Depth.w * G(FadeDistance_GlowFactor).y;
	float outlineFade = saturate1( ViewDepthMul * G(OutlineBrightness_ShadowInfo).x + G(OutlineBrightness_ShadowInfo).y);
	float2 minIntermediate = min(IN.Uv_EdgeDistance1.wz, IN.UvStuds_EdgeDistance2.wz);
	float minEdgesPlus = min(minIntermediate.x, minIntermediate.y) / ViewDepthMul;
	oColor0.rgb *= saturate1(outlineFade *(1.5 - minEdgesPlus) + minEdgesPlus);
#endif

    float fogAlpha = saturate(IN.LightPosition_Fog.w);

#ifdef PIN_NEON
    oColor0.rgb = IN.Color.rgb * G(FadeDistance_GlowFactor).w;
    oColor0.a = 1 - fogAlpha * IN.Color.a;
    diffuse.rgb = 0;
    specular.rgb = 0;
#endif

    oColor0.rgb = lerp(G(FogColor), oColor0.rgb, fogAlpha);

#ifdef PIN_GBUFFER
    oColor1 = gbufferPack(IN.View_Depth.w, diffuse.rgb, specular.rgb, fogAlpha);
#endif

}
#endif