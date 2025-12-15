#include "common.h"

TEX_DECLARE2D(Texture, 0);
TEX_DECLARE2D(Mask, 1);

// .xy = gbuffer width/height, .zw = inverse gbuffer width/height
uniform float4 TextureSize;
uniform float4 Params1;
uniform float4 Params2;

#if defined(GLSL) || defined(DX11)
float4 convertPosition(float4 p, float scale)
{
	return p;
}    
#else
float4 convertPosition(float4 p, float scale)
{
	// half-pixel offset
	return p + float4(-TextureSize.z, TextureSize.w, 0, 0) * scale;
}
#endif

#ifndef GLSL
float2 convertUv(float4 p)
{
	return p.xy * float2(0.5, -0.5) + 0.5;
}
#else
float2 convertUv(float4 p)
{
	return p.xy * 0.5 + 0.5;
}
#endif


// simple pass through structure
struct VertexOutput
{
    float4 p    : POSITION;
    float2 uv   : TEXCOORD0;
};

// position and tex coord + 4 additional tex coords
struct VertexOutput_4uv
{
    float4 p    : POSITION;
    float2 uv   : TEXCOORD0;
	float4 uv12 : TEXCOORD1;
	float4 uv34 : TEXCOORD2;
};

// position and tex coord + 8 additional tex coords
struct VertexOutput_8uv
{
    float4 p    : POSITION;
    float2 uv   : TEXCOORD0;
	float4 uv12 : TEXCOORD1;
	float4 uv34 : TEXCOORD2;
	float4 uv56 : TEXCOORD3;
	float4 uv78 : TEXCOORD4;
};

VertexOutput passThrough_vs(float4 p: POSITION)
{
    VertexOutput OUT;
    OUT.p = convertPosition(p, 1);
    OUT.uv = convertUv(p);

    return OUT;
}

float4 passThrough_ps( VertexOutput IN ) : COLOR0
{
	return tex2D(Texture, IN.uv);
}

float4 imageProcess_ps( VertexOutput IN ) : COLOR0
{
	float3 color = tex2D(Texture, IN.uv).rgb;

	float4 tintColor = float4(Params2.xyz,1);
	//float4 tintColor = float4(18.0 / 255.0, 58.0 / 255.0, 80.0 / 255.0, 1);
	float contrast = Params1.y;
	float brightness = Params1.x;
	float grayscaleLvl = Params1.z;

	color = contrast*(color - 0.5) + 0.5 + brightness;
	float grayscale = (color.r + color.g + color.g) / 3.0;

	return lerp(float4(color.rgb,1), float4(grayscale.xxx,1), grayscaleLvl) * tintColor;
}

float4 gauss(float samples, float2 uv)
{
	float2 step = Params1.xy;
	float sigma = Params1.z;

	float sigmaN1 = 1 / sqrt(2 * 3.1415926 * sigma * sigma);
	float sigmaN2 = 1 / (2 * sigma * sigma);

	// First sample is in the center and accounts for our pixel
	float4 result = tex2D(Texture, uv) * sigmaN1;
	float weight = sigmaN1;

	// Every loop iteration computes impact of 4 pixels
	// Each sample computes impact of 2 neighbor pixels, starting with the next one to the right
	// Note that we sample exactly in between pixels to leverage bilinear filtering
	for (int i = 0; i < samples; ++i)
	{
		float ix = 2 * i + 1.5;
		float iw = 2 * exp(-ix * ix * sigmaN2) * sigmaN1;

		result += (tex2D(Texture, uv + step * ix) + tex2D(Texture, uv - step * ix)) * iw;
		weight += 2 * iw;
	}

	// Since the above is an approximation of the integral with step functions, normalization compensates for the error
	return (result / weight);
}

float4 fxaa_ps(VertexOutput IN) : COLOR0
{
    float2 uv = IN.uv;
    float2 rcpFrame = TextureSize.zw;
    
    // Luma conversion weights
    const float3 luma = float3(0.299, 0.587, 0.114);
    
    // Sample 3x3 neighborhood
    float3 rgbM = tex2D(Texture, uv).rgb;
    float3 rgbN = tex2D(Texture, uv + float2(0, -rcpFrame.y)).rgb;
    float3 rgbS = tex2D(Texture, uv + float2(0, rcpFrame.y)).rgb;
    float3 rgbE = tex2D(Texture, uv + float2(rcpFrame.x, 0)).rgb;
    float3 rgbW = tex2D(Texture, uv + float2(-rcpFrame.x, 0)).rgb;
    float3 rgbNW = tex2D(Texture, uv + float2(-rcpFrame.x, -rcpFrame.y)).rgb;
    float3 rgbNE = tex2D(Texture, uv + float2(rcpFrame.x, -rcpFrame.y)).rgb;
    float3 rgbSW = tex2D(Texture, uv + float2(-rcpFrame.x, rcpFrame.y)).rgb;
    float3 rgbSE = tex2D(Texture, uv + float2(rcpFrame.x, rcpFrame.y)).rgb;
    
    float lumaM = dot(rgbM, luma);
    float lumaN = dot(rgbN, luma);
    float lumaS = dot(rgbS, luma);
    float lumaE = dot(rgbE, luma);
    float lumaW = dot(rgbW, luma);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    
    // Find local contrast including corners
    float lumaMin = min(lumaM, min(min(min(lumaN, lumaS), min(lumaE, lumaW)), 
                                    min(min(lumaNW, lumaNE), min(lumaSW, lumaSE))));
    float lumaMax = max(lumaM, max(max(max(lumaN, lumaS), max(lumaE, lumaW)), 
                                    max(max(lumaNW, lumaNE), max(lumaSW, lumaSE))));
    float range = lumaMax - lumaMin;
    
    // Very low threshold - catch almost all edges
    if (range < max(0.0156, lumaMax * 0.0625))
    {
        return float4(rgbM, 1.0);
    }
    
    // Determine edge orientation
    float edgeVert = abs((0.25 * lumaNW) + (-0.5 * lumaN) + (0.25 * lumaNE)) +
                     abs((0.50 * lumaW) + (-1.0 * lumaM) + (0.50 * lumaE)) +
                     abs((0.25 * lumaSW) + (-0.5 * lumaS) + (0.25 * lumaSE));
    
    float edgeHorz = abs((0.25 * lumaNW) + (-0.5 * lumaW) + (0.25 * lumaSW)) +
                     abs((0.50 * lumaN) + (-1.0 * lumaM) + (0.50 * lumaS)) +
                     abs((0.25 * lumaNE) + (-0.5 * lumaE) + (0.25 * lumaSE));
    
    bool isHorizontal = (edgeHorz >= edgeVert);
    
    // Select samples along edge
    float luma1 = isHorizontal ? lumaS : lumaE;
    float luma2 = isHorizontal ? lumaN : lumaW;
    float gradient1 = luma1 - lumaM;
    float gradient2 = luma2 - lumaM;
    
    // Pick steepest gradient
    bool is1Steepest = abs(gradient1) >= abs(gradient2);
    float gradientScaled = 0.25 * max(abs(gradient1), abs(gradient2));
    float lengthSign = is1Steepest ? -1.0 : 1.0;
    
    if (!isHorizontal)
        lengthSign *= -1.0;
    
    // Setup for edge search
    float2 posB = uv;
    float2 offNP;
    offNP.x = (!isHorizontal) ? rcpFrame.x : 0.0;
    offNP.y = (isHorizontal) ? rcpFrame.y : 0.0;
    
    if (!isHorizontal)
        posB.x += lengthSign * 0.5 * rcpFrame.x;
    else
        posB.y += lengthSign * 0.5 * rcpFrame.y;
    
    // Extended edge search - 12 iterations
    float2 posN = posB - offNP;
    float2 posP = posB + offNP;
    
    float lumaEndN = dot(tex2D(Texture, posN).rgb, luma) - lumaM * 0.5;
    float lumaEndP = dot(tex2D(Texture, posP).rgb, luma) - lumaM * 0.5;
    
    bool doneN = abs(lumaEndN) >= gradientScaled;
    bool doneP = abs(lumaEndP) >= gradientScaled;
    
    // More iterations with finer steps for better accuracy
    float stepSizes[12] = {1.0, 1.0, 1.5, 1.5, 2.0, 2.0, 4.0, 4.0, 8.0, 8.0, 16.0, 16.0};
    
    for (int i = 0; i < 12; ++i)
    {
        if (doneN && doneP) break;
        
        if (!doneN) 
        {
            posN -= offNP * stepSizes[i];
            lumaEndN = dot(tex2D(Texture, posN).rgb, luma) - lumaM * 0.5;
            doneN = abs(lumaEndN) >= gradientScaled;
        }
        
        if (!doneP) 
        {
            posP += offNP * stepSizes[i];
            lumaEndP = dot(tex2D(Texture, posP).rgb, luma) - lumaM * 0.5;
            doneP = abs(lumaEndP) >= gradientScaled;
        }
    }
    
    // Calculate span and offset
    float dstN = isHorizontal ? (uv.x - posN.x) : (uv.y - posN.y);
    float dstP = isHorizontal ? (posP.x - uv.x) : (posP.y - uv.y);
    
    bool directionN = dstN < dstP;
    float dst = min(dstN, dstP);
    
    float spanLength = (dstP + dstN);
    float spanLengthRcp = 1.0 / spanLength;
    float pixelOffset = dst * (-spanLengthRcp) + 0.5;
    
    // Check if we're at a good span
    bool goodSpanN = (lumaEndN < 0.0) != (lumaM < lumaMin);
    bool goodSpanP = (lumaEndP < 0.0) != (lumaM < lumaMin);
    bool goodSpan = directionN ? goodSpanN : goodSpanP;
    
    if (!goodSpan)
        pixelOffset = 0.0;
    
    // sub-pixel antialiasing
    float lumaAverage = (1.0/12.0) * (2.0 * (lumaN + lumaS + lumaE + lumaW) + 
                                       lumaNW + lumaNE + lumaSW + lumaSE);
    
    float subPixelOffset1 = saturate(abs(lumaAverage - lumaM) / range);
    subPixelOffset1 = (-2.0 * subPixelOffset1 + 3.0) * subPixelOffset1 * subPixelOffset1;
    subPixelOffset1 = subPixelOffset1 * subPixelOffset1 * 0.85;
    // Blend edge and subpixel offsets
    float pixelOffsetFinal = max(pixelOffset, subPixelOffset1);
    
    // Apply offset
    float2 finalUv = uv;
    if (!isHorizontal)
        finalUv.x += pixelOffsetFinal * lengthSign * rcpFrame.x;
    else
        finalUv.y += pixelOffsetFinal * lengthSign * rcpFrame.y;
    
    // Additional edge blending for remaining jaggies
    float3 rgbFinal = tex2D(Texture, finalUv).rgb;
    
    // If we're on a strong edge, do additional smart blending
    if (range > 0.1)
    {
        float edgeBlend = saturate(range * 2.0 - 0.2);
        float3 rgbBlur = (rgbN + rgbS + rgbE + rgbW) * 0.25;
        rgbFinal = lerp(rgbFinal, rgbBlur, edgeBlend * 0.15);
    }
    
    return float4(rgbFinal, 1.0);
}

float4 blur3_ps(VertexOutput IN): COLOR0
{
	return gauss(3, IN.uv);
}

float4 blur5_ps(VertexOutput IN): COLOR0
{
	return gauss(5, IN.uv);
}

float4 blur7_ps(VertexOutput IN): COLOR0
{
	return gauss(7, IN.uv);
}

float4 glowApply_ps( VertexOutput IN ) : COLOR0
{
	float4 color = tex2D(Texture, IN.uv);
	return float4(color.rgb * Params1.x, color.a);
}

// this is specific glow downsample
float4 downSample4x4Glow_ps( VertexOutput_4uv IN ) : COLOR0
{
	float4 avgColor = tex2D( Texture, IN.uv12.xy );
	avgColor += tex2D( Texture, IN.uv12.zw );
	avgColor += tex2D( Texture, IN.uv34.xy );
	avgColor += tex2D( Texture, IN.uv34.zw );

	avgColor *= 0.25;
	return float4(avgColor.rgb, 1) * (1-avgColor.a);
}

VertexOutput_4uv downsample4x4_vs(float4 p: POSITION)
{
    float2 uv = convertUv(p);

    VertexOutput_4uv OUT;
    OUT.p = convertPosition(p, 1);
    OUT.uv = uv;

	float2 uvOffset = TextureSize.zw * 0.25f;

	OUT.uv12.xy = uv + uvOffset * float2(-1, -1);
	OUT.uv12.zw = uv + uvOffset * float2(+1, -1);
	OUT.uv34.xy = uv + uvOffset * float2(-1, +1);
	OUT.uv34.zw = uv + uvOffset * float2(+1, +1);

    return OUT;
}


float4 ShadowBlurPS(VertexOutput IN): COLOR0
{
#ifdef GLSLES
    int N = 1;
	float sigma = 0.5;
#else
    int N = 3;
	float sigma = 1.5;
#endif

	float2 step = Params1.xy;

	float sigmaN1 = 1 / sqrt(2 * 3.1415926 * sigma * sigma);
	float sigmaN2 = 1 / (2 * sigma * sigma);

    float depth = 1;
    float color = 0;
    float weight = 0;

	for (int i = -N; i <= N; ++i)
	{
        float ix = i;
		float iw = exp(-ix * ix * sigmaN2) * sigmaN1;

        float4 data = tex2D(Texture, IN.uv + step * ix);

        depth = min(depth, data.x);
        color += data.y * iw;
		weight += iw;
	}

    float mask = tex2D(Mask, IN.uv).r;

	// Since the above is an approximation of the integral with step functions, normalization compensates for the error
	return float4(depth, color * mask * (1 / weight), 0, 0);
}