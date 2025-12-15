$input a_position, a_texcoord0, a_color0
$output v_texcoord0, v_color0

#include "common.sh"

SAMPLER2D(s_diffuseMap, 0);

void ProfilerVS()
{
    gl_Position = mul(u_viewProjection, vec4(a_position, 1.0));
    gl_Position.y = -gl_Position.y;

    v_texcoord0 = a_texcoord0;
    v_color0 = a_color0;
}

void ProfilerPS()
{
    vec4 c0 = texture2D(s_diffuseMap, v_texcoord0);
    vec4 c1 = texture2D(s_diffuseMap, v_texcoord0 + vec2(0.0, 1.0 / 9.0));

    gl_FragColor = c0.a < 0.5 ? vec4(0.0, 0.0, 0.0, c1.a) : c0 * v_color0;
}

void ImGuiPS()
{
    vec4 texColor = texture2D(s_diffuseMap, v_texcoord0);
    vec4 finalColor = texColor * v_color0;
    
    gl_FragColor = finalColor;
}
