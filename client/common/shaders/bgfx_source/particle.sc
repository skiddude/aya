$input a_position, a_texcoord0, a_texcoord1, a_texcoord2, a_texcoord3
$output v_texcoord0, v_texcoord1, v_color0

#include "common.sh"

SAMPLER2D(s_tex, 0);
SAMPLER2D(s_cstrip, 1);
SAMPLER2D(s_astrip, 2);

uniform vec4 u_throttleFactor; // .x = alpha cutoff, .y = alpha boost (clamp), .w - additive/alpha ratio for Crazy shaders
uniform vec4 u_modulateColor;
uniform vec4 u_zOffset;

vec4 rotScale(vec4 scaleRotLife)
{
    float cr = cos(scaleRotLife.z);
    float sr = sin(scaleRotLife.z);

    vec4 r;
    r.x = cr * scaleRotLife.x;
    r.y = -sr * scaleRotLife.x;
    r.z = sr * scaleRotLife.y;
    r.w = cr * scaleRotLife.y;
    
    return r;
}

vec4 mulq(vec4 a, vec4 b)
{
    vec3 i = cross(a.xyz, b.xyz) + a.w * b.xyz + b.w * a.xyz;
    float r = a.w * b.w - dot(a.xyz, b.xyz);
    return vec4(i, r);
}

vec4 conj(vec4 a) 
{ 
    return vec4(-a.xyz, a.w); 
}

vec4 rotate(vec4 v, vec4 q) 
{
    return mulq(mulq(q, v), conj(q));
}

vec4 axis_angle(vec3 axis, float angle)
{
    return vec4(sin(angle/2.0) * axis, cos(angle/2.0));
}

void vs()
{
    vec4 pos = vec4(a_position, 1.0);
    vec2 disp = a_texcoord1.xy * 2.0 - 1.0; // -1..1

    vec4 scaleRotLifeFlt = a_texcoord0 * vec4(1.0/256.0, 1.0/256.0, 2.0 * 3.1415926 / 32767.0, 1.0 / 32767.0);
    scaleRotLifeFlt.xy += 127.0;
    
    vec4 rs = rotScale(scaleRotLifeFlt);

    pos += u_viewRight * dot(disp, rs.xy);
    pos += u_viewUp * dot(disp, rs.zw);

    vec4 pos2 = pos + u_viewDir * u_zOffset.x; // Z-offset position in world space

    gl_Position = mul(u_viewProjection, pos);
    
    v_texcoord0.xy = a_texcoord1.xy;
    v_texcoord0.y = 1.0 - v_texcoord0.y;
    v_texcoord0.z = (u_fogParams.z - gl_Position.w) * u_fogParams.w;
    
    v_texcoord1.x = 1.0 - max(0.0, min(1.0, scaleRotLifeFlt.w));
    v_texcoord1.y = a_texcoord2.x * (1.0 / 32767.0);

    pos2 = mul(u_viewProjection, pos2); // Z-offset position in clip space
    gl_Position.z = pos2.z * gl_Position.w / pos2.w; // Only need z
}

void psAdd()
{
    vec4 texcolor = texture2D(s_tex, v_texcoord0.xy);
    vec4 vcolor = texture2D(s_cstrip, v_texcoord1.xy);
    vcolor.a = texture2D(s_astrip, v_texcoord1.xy).r;
    
    vec4 result;

    result.rgb = (texcolor.rgb + vcolor.rgb) * u_modulateColor.rgb;
    result.a = texcolor.a * vcolor.a;
    result.rgb *= result.a;

    result.rgb = mix(vec3(0.0), result.rgb, saturate(v_texcoord0.zzz));
    gl_FragColor = result;
}

void psModulate()
{
    vec4 texcolor = texture2D(s_tex, v_texcoord0.xy);
    vec4 vcolor = texture2D(s_cstrip, v_texcoord1.xy) * u_modulateColor;
    vcolor.a = texture2D(s_astrip, v_texcoord1.xy).r * u_modulateColor.a;

    vec4 result;

    result.rgb = texcolor.rgb * vcolor.rgb;
    result.a = texcolor.a * vcolor.a;
    
    result.rgb = mix(u_fogColor.rgb, result.rgb, saturate(v_texcoord0.zzz));
    gl_FragColor = result;
}

// - this shader is crazy
// - used instead of additive particles to help see bright particles (e.g. fire) on top of extremely bright backgrounds 
// - requires ONE | INVSRCALPHA blend mode, useless otherwise
// - does not use color strip texture
// - outputs a blend between additive blend and alpha blend in fragment alpha
// - ratio multiplier is in throttleFactor.w
void psCrazy()
{
    vec4 texcolor = texture2D(s_tex, v_texcoord0.xy);
    vec4 vcolor = vec4(1.0, 0.0, 0.0, 0.0);
    vcolor.a = texture2D(s_astrip, v_texcoord1.xy).r;
    float blendRatio = u_throttleFactor.w;
    
    vec4 result;

    result.rgb = (texcolor.rgb) * u_modulateColor.rgb * vcolor.a * texcolor.a;
    result.a = blendRatio * texcolor.a * vcolor.a;

    result = mix(vec4(0.0), result, saturate(v_texcoord0.zzzz));
    gl_FragColor = result;
}

void psCrazySparkles()
{
    vec4 texcolor = texture2D(s_tex, v_texcoord0.xy);
    vec4 vcolor = texture2D(s_cstrip, v_texcoord1.xy);
    vcolor.a = texture2D(s_astrip, v_texcoord1.xy).r;
    float blendRatio = u_throttleFactor.w;
    
    vec4 result;

    if (texcolor.a < 0.5)
    {
        result.rgb = vcolor.rgb * u_modulateColor.rgb * (2.0 * texcolor.a);
    }
    else
    {
        result.rgb = mix(vcolor.rgb * u_modulateColor.rgb, texcolor.rgb, 2.0 * texcolor.a - 1.0);
    }
    
    result.rgb *= vcolor.a;
    result.a = blendRatio * texcolor.a * vcolor.a;
    
    result = mix(vec4(0.0), result, saturate(v_texcoord0.zzzz));
    gl_FragColor = result;
}

// Custom particle shader
void vsCustom()
{
    vec4 pos = vec4(a_position, 1.0);
    vec2 disp = a_texcoord1.xy * 2.0 - 1.0; // -1..1

    vec4 scaleRotLifeFlt = a_texcoord0 * vec4(1.0/256.0, 1.0/256.0, 2.0 * 3.1415926 / 32767.0, 1.0 / 32767.0);
    scaleRotLifeFlt.xy += 127.0;

    vec4 rs = rotScale(scaleRotLifeFlt);

    pos += u_viewRight * dot(disp, rs.xy);
    pos += u_viewUp * dot(disp, rs.zw);
    
    vec4 pos2 = pos + u_viewDir * u_zOffset.x; // Z-offset position in world space

    gl_Position = mul(u_viewProjection, pos);
    
    v_texcoord0.xy = a_texcoord1.xy;
    v_texcoord0.y = 1.0 - v_texcoord0.y;
    v_texcoord0.z = (u_fogParams.z - gl_Position.w) * u_fogParams.w;
    
    v_color0 = a_texcoord3 * (1.0/255.0);
       
    pos2 = mul(u_viewProjection, pos2); // Z-offset position in clip space
    gl_Position.z = pos2.z * gl_Position.w / pos2.w; // Only need z
}

void psCustom()
{
    vec4 texcolor = texture2D(s_tex, v_texcoord0.xy);
    vec4 vcolor = v_color0;

    float blendRatio = u_throttleFactor.w;
    
    vec4 result;

    result.rgb = texcolor.rgb * vcolor.rgb * vcolor.a * texcolor.a;
    result.a = blendRatio * texcolor.a * vcolor.a;

    result = mix(vec4(0.0), result, saturate(v_texcoord0.zzzz));
    gl_FragColor = result;
}
