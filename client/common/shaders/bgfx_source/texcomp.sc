$input a_position, a_texcoord0
$output v_texcoord0

#include "common.sh"

SAMPLER2D(s_diffuseMap, 0);

uniform vec4 u_color;

void TexCompVS()
{
    gl_Position = mul(u_viewProjection, vec4(a_position, 1.0));
    v_texcoord0 = a_texcoord0;
}

void TexCompPS()
{
    gl_FragColor = texture2DLod(s_diffuseMap, v_texcoord0, -10.0) * u_color;
}

void TexCompPMAPS()
{
    vec4 tex = texture2DLod(s_diffuseMap, v_texcoord0, -10.0);
    
    gl_FragColor = vec4(tex.rgb * tex.a * u_color.rgb, tex.a * u_color.a);
}
