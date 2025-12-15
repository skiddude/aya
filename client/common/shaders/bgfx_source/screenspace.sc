$input a_position
$output v_pos, v_texcoord0, v_texcoord1, v_texcoord2

#include "common.sh"
#include "globals.sh"

SAMPLER2D(s_texture, 0);
SAMPLER2D(s_mask, 1);

// .xy = gbuffer width/height, .zw = inverse gbuffer width/height
uniform vec4 u_textureSize;
uniform vec4 u_params1;
uniform vec4 u_params2;

vec4 convertPosition(vec4 p, float scale)
{
#if defined(GLSL) || defined(DX11)
    return p;
#else
    // half-pixel offset
    return p + vec4(-u_textureSize.z, u_textureSize.w, 0.0, 0.0) * scale;
#endif
}

vec2 convertUv(vec4 p)
{
#ifndef GLSL
    return p.xy * vec2(0.5, -0.5) + 0.5;
#else
    return p.xy * 0.5 + 0.5;
#endif
}

// ===== Pass Through =====
void passThrough_vs()
{
    v_pos = convertPosition(vec4(a_position.xyz, 1.0), 1.0);
    v_texcoord0.xy = convertUv(vec4(a_position.xyz, 1.0));

    gl_Position = v_pos;
}

void passThrough_ps()
{
    gl_FragColor = texture2D(s_texture, v_texcoord0.xy);
}

// ===== Image Processing =====
void imageProcess_ps()
{
    vec3 color = texture2D(s_texture, v_texcoord0.xy).rgb;

    vec4 tintColor = vec4(u_params2.xyz, 1.0);
    float contrast = u_params1.y;
    float brightness = u_params1.x;
    float grayscaleLvl = u_params1.z;

    color = contrast * (color - 0.5) + 0.5 + brightness;
    float grayscale = (color.r + color.g + color.g) / 3.0;

    gl_FragColor = mix(vec4(color.rgb, 1.0), vec4(grayscale, grayscale, grayscale, 1.0), grayscaleLvl) * tintColor;
}

// ===== Gaussian Blur =====
vec4 gauss(float samples, vec2 uv)
{
    vec2 step = u_params1.xy;
    float sigma = u_params1.z;

    float sigmaN1 = 1.0 / sqrt(2.0 * 3.1415926 * sigma * sigma);
    float sigmaN2 = 1.0 / (2.0 * sigma * sigma);

    // First sample is in the center and accounts for our pixel
    vec4 result = texture2D(s_texture, uv) * sigmaN1;
    float weight = sigmaN1;

    // Every loop iteration computes impact of 4 pixels
    // Each sample computes impact of 2 neighbor pixels, starting with the next one to the right
    // Note that we sample exactly in between pixels to leverage bilinear filtering
    for (int i = 0; i < int(samples); ++i)
    {
        float ix = 2.0 * float(i) + 1.5;
        float iw = 2.0 * exp(-ix * ix * sigmaN2) * sigmaN1;

        result += (texture2D(s_texture, uv + step * ix) + texture2D(s_texture, uv - step * ix)) * iw;
        weight += 2.0 * iw;
    }

    // Since the above is an approximation of the integral with step functions, normalization compensates for the error
    return (result / weight);
}

void blur3_ps()
{
    gl_FragColor = gauss(3.0, v_texcoord0.xy);
}

void blur5_ps()
{
    gl_FragColor = gauss(5.0, v_texcoord0.xy);
}

void blur7_ps()
{
    gl_FragColor = gauss(7.0, v_texcoord0.xy);
}

// ===== FXAA =====
void fxaa_ps()
{
    vec2 uv = v_texcoord0.xy;
    vec2 rcpFrame = u_textureSize.zw;
    
    // Luma conversion weights
    const vec3 luma = vec3(0.299, 0.587, 0.114);
    
    // Sample 3x3 neighborhood
    vec3 rgbM = texture2D(s_texture, uv).rgb;
    vec3 rgbN = texture2D(s_texture, uv + vec2(0.0, -rcpFrame.y)).rgb;
    vec3 rgbS = texture2D(s_texture, uv + vec2(0.0, rcpFrame.y)).rgb;
    vec3 rgbE = texture2D(s_texture, uv + vec2(rcpFrame.x, 0.0)).rgb;
    vec3 rgbW = texture2D(s_texture, uv + vec2(-rcpFrame.x, 0.0)).rgb;
    vec3 rgbNW = texture2D(s_texture, uv + vec2(-rcpFrame.x, -rcpFrame.y)).rgb;
    vec3 rgbNE = texture2D(s_texture, uv + vec2(rcpFrame.x, -rcpFrame.y)).rgb;
    vec3 rgbSW = texture2D(s_texture, uv + vec2(-rcpFrame.x, rcpFrame.y)).rgb;
    vec3 rgbSE = texture2D(s_texture, uv + vec2(rcpFrame.x, rcpFrame.y)).rgb;
    
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
        gl_FragColor = vec4(rgbM, 1.0);
        return;
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
    vec2 posB = uv;
    vec2 offNP;
    offNP.x = (!isHorizontal) ? rcpFrame.x : 0.0;
    offNP.y = (isHorizontal) ? rcpFrame.y : 0.0;
    
    if (!isHorizontal)
        posB.x += lengthSign * 0.5 * rcpFrame.x;
    else
        posB.y += lengthSign * 0.5 * rcpFrame.y;
    
    // Extended edge search
    vec2 posN = posB - offNP;
    vec2 posP = posB + offNP;
    
    float lumaEndN = dot(texture2D(s_texture, posN).rgb, luma) - lumaM * 0.5;
    float lumaEndP = dot(texture2D(s_texture, posP).rgb, luma) - lumaM * 0.5;
    
    bool doneN = abs(lumaEndN) >= gradientScaled;
    bool doneP = abs(lumaEndP) >= gradientScaled;
    
    float stepSizes[12];
    stepSizes[0] = 1.0; stepSizes[1] = 1.0; stepSizes[2] = 1.5; stepSizes[3] = 1.5;
    stepSizes[4] = 2.0; stepSizes[5] = 2.0; stepSizes[6] = 4.0; stepSizes[7] = 4.0;
    stepSizes[8] = 8.0; stepSizes[9] = 8.0; stepSizes[10] = 16.0; stepSizes[11] = 16.0;
    
    for (int i = 0; i < 12; ++i)
    {
        if (doneN && doneP) break;
        
        if (!doneN) 
        {
            posN -= offNP * stepSizes[i];
            lumaEndN = dot(texture2D(s_texture, posN).rgb, luma) - lumaM * 0.5;
            doneN = abs(lumaEndN) >= gradientScaled;
        }
        
        if (!doneP) 
        {
            posP += offNP * stepSizes[i];
            lumaEndP = dot(texture2D(s_texture, posP).rgb, luma) - lumaM * 0.5;
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
    float lumaAverage = (1.0 / 12.0) * (2.0 * (lumaN + lumaS + lumaE + lumaW) + 
                                       lumaNW + lumaNE + lumaSW + lumaSE);
    
    float subPixelOffset1 = saturate0(abs(lumaAverage - lumaM) / range);
    subPixelOffset1 = (-2.0 * subPixelOffset1 + 3.0) * subPixelOffset1 * subPixelOffset1;
    subPixelOffset1 = subPixelOffset1 * subPixelOffset1 * 0.85;
    float pixelOffsetFinal = max(pixelOffset, subPixelOffset1);
    
    // Apply offset
    vec2 finalUv = uv;
    if (!isHorizontal)
        finalUv.x += pixelOffsetFinal * lengthSign * rcpFrame.x;
    else
        finalUv.y += pixelOffsetFinal * lengthSign * rcpFrame.y;
    
    // Additional edge blending for remaining jaggies
    vec3 rgbFinal = texture2D(s_texture, finalUv).rgb;
    
    // If we're on a strong edge, do additional smart blending
    if (range > 0.1)
    {
        float edgeBlend = saturate0(range * 2.0 - 0.2);
        vec3 rgbBlur = (rgbN + rgbS + rgbE + rgbW) * 0.25;
        rgbFinal = mix(rgbFinal, rgbBlur, edgeBlend * 0.15);
    }
    
    gl_FragColor = vec4(rgbFinal, 1.0);
}

// ===== Glow Apply =====
void glowApply_ps()
{
    vec4 color = texture2D(s_texture, v_texcoord0.xy);
    gl_FragColor = vec4(color.rgb * u_params1.x, color.a);
}

// ===== Downsample 4x4 =====
void downsample4x4_vs()
{
    vec2 uv = convertUv(vec4(a_position.xyz, 1.0));

    v_pos = convertPosition(vec4(a_position.xyz, 1.0), 1.0);
    v_texcoord0.xy = uv;

    vec2 uvOffset = u_textureSize.zw * 0.25;

    v_texcoord0.zw = uv + uvOffset * vec2(-1.0, -1.0);
    v_texcoord1.xy = uv + uvOffset * vec2(+1.0, -1.0);
    v_texcoord1.zw = uv + uvOffset * vec2(-1.0, +1.0);
    v_texcoord2.xy = uv + uvOffset * vec2(+1.0, +1.0);

    gl_Position = v_pos;
}

void downSample4x4Glow_ps()
{
    vec4 avgColor = texture2D(s_texture, v_texcoord0.zw);
    avgColor += texture2D(s_texture, v_texcoord1.xy);
    avgColor += texture2D(s_texture, v_texcoord1.zw);
    avgColor += texture2D(s_texture, v_texcoord2.xy);

    avgColor *= 0.25;
    gl_FragColor = vec4(avgColor.rgb, 1.0) * (1.0 - avgColor.a);
}

// ===== Shadow Blur =====
void ShadowBlurPS()
{
#ifdef GLSLES
    int N = 1;
    float sigma = 0.5;
#else
    int N = 3;
    float sigma = 1.5;
#endif

    vec2 step = u_params1.xy;

    float sigmaN1 = 1.0 / sqrt(2.0 * 3.1415926 * sigma * sigma);
    float sigmaN2 = 1.0 / (2.0 * sigma * sigma);

    float depth = 1.0;
    float color = 0.0;
    float weight = 0.0;

    for (int i = -N; i <= N; ++i)
    {
        float ix = float(i);
        float iw = exp(-ix * ix * sigmaN2) * sigmaN1;

        vec4 data = texture2D(s_texture, v_texcoord0.xy + step * ix);

        depth = min(depth, data.x);
        color += data.y * iw;
        weight += iw;
    }

    float mask = texture2D(s_mask, v_texcoord0.xy).r;

    // Since the above is an approximation of the integral with step functions, normalization compensates for the error
    gl_FragColor = vec4(depth, color * mask * (1.0 / weight), 0.0, 0.0);
}
