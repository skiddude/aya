$input a_position, a_texcoord0, a_color0
$output v_texcoord0, v_color0, v_texcoord1

#include "common.sh"

uniform vec4 u_uiParams; // x = luminance sampling on/off, w = z offset
SAMPLER2D(s_diffuseMap, 0);

void UIVS()
{
    gl_Position = mul(u_viewProjection, vec4(a_position, 1.0));
    gl_Position.z -= u_uiParams.w; // against z-fighting

    v_texcoord0 = a_texcoord0;
    v_color0 = a_color0;

#if defined(PIN_FOG)
    v_texcoord1.x = (u_fogParams.z - gl_Position.w) * u_fogParams.w;
#endif
}

void UIPS()
{
    vec4 base;

    if (u_uiParams.x > 0.5)
        base = vec4(1.0, 1.0, 1.0, texture2D(s_diffuseMap, v_texcoord0).r);
    else
        base = texture2D(s_diffuseMap, v_texcoord0);

    vec4 result = v_color0 * base;

#if defined(PIN_FOG)
    result.rgb = mix(u_fogColor, result.rgb, saturate(v_texcoord1.x));
#endif

    gl_FragColor = result;
}
