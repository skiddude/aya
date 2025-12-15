$input a_position, a_color1
$output v_texcoord0

#include "common.sh"

void ShadowVS()
{
    // Transform position to world space
#ifdef PIN_SKINNED
    int boneIndex = int(a_color1.r);

    vec4 worldRow0 = u_worldMatrixArray[boneIndex * 3 + 0];
    vec4 worldRow1 = u_worldMatrixArray[boneIndex * 3 + 1];
    vec4 worldRow2 = u_worldMatrixArray[boneIndex * 3 + 2];
        
    vec3 posWorld = vec3(dot(worldRow0, vec4(a_position, 1.0)), 
                         dot(worldRow1, vec4(a_position, 1.0)), 
                         dot(worldRow2, vec4(a_position, 1.0)));
#else
    vec3 posWorld = a_position;
#endif

    gl_Position = mul(u_viewProjection, vec4(posWorld, 1.0));

    v_texcoord0 = shadowPrepareSample(posWorld);
}

void ShadowPS()
{
    float depth = shadowDepth(v_texcoord0);

    gl_FragColor = vec4(depth, 1.0, 0.0, 0.0);
}
