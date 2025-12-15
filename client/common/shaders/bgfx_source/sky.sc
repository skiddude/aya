$input a_position, a_texcoord0, a_color0
$output v_texcoord0, v_color0

#include "common.sh"

uniform vec4 u_color;
uniform vec4 u_color2;

SAMPLER2D(s_diffuseMap, 0);

void SkyVS()
{
    vec4 wpos = mul(u_worldMatrix, vec4(a_position, 1.0));

    gl_Position = mul(u_viewProjection, wpos);

#ifndef GLSLES
    // snap to far plane to prevent scene-sky intersections
    // small offset is needed to prevent 0/0 division in case w=0, which causes rasterization issues
    // some mobile chips (hello, Vivante!) don't like it
    gl_Position.z = gl_Position.w - 1.0 / 16.0;
#endif

#if BGFX_SHADER_LANGUAGE_GLSL
    gl_PointSize = 2.0; // star size
#endif

    v_texcoord0 = a_texcoord0;
    v_color0 = a_color0 * mix(u_color2, u_color, wpos.y / 1700.0);
}

void SkyPS()
{
    gl_FragColor = texture2D(s_diffuseMap, v_texcoord0) * v_color0;
}
