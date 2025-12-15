$input a_position, a_texcoord0
$output v_texcoord0

#include "common.sh"

uniform vec4 u_textureSize;

SAMPLER2D(s_tex, 0);

vec4 convertPosition(vec4 p)
{
#if defined(GLSL) || defined(DX11)
    return p;
#else
    return p + vec4(-u_textureSize.z, u_textureSize.w, 0.0, 0.0);
#endif
}

vec2 convertUv(vec4 p)
{
#if defined(GLSL)
    return p.xy * 0.5 + 0.5;
#else
    return p.xy * vec2(0.5, -0.5) + 0.5;
#endif
}

void gbufferVS()
{
    gl_Position = convertPosition(vec4(a_position, 1.0));
    v_texcoord0 = convertUv(vec4(a_position, 1.0));
}

void gbufferPS()
{
    gl_FragColor = texture2D(s_tex, v_texcoord0);
}
