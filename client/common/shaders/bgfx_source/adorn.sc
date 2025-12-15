$input a_position, a_texcoord0, a_normal
$output v_texcoord0, v_color0, v_texcoord1, v_worldPos, v_position, v_start, v_end, v_centerRadius

#include "common.sh"

uniform vec4 u_color;
uniform vec4 u_pixelInfo; // x -> Fov * 0.5f / screenSize.y; y -> ScreenWidth; z -> ScreenWidth / ScreenHeight; w -> Line thickness

SAMPLER2D(s_diffuseMap, 0);

// Vertex Shaders
void AdornSelfLitVS()
{
    vec4 worldPos = mul(u_worldMatrix, vec4(a_position, 1.0));
    vec3 worldNormal = normalize(mul(u_worldMatrix, vec4(a_normal, 0.0)).xyz);

    vec3 light = normalize(u_cameraPosition - worldPos.xyz);
    float ndotl = saturate(dot(worldNormal, light));

    float lighting = 0.5 + (1.0 - 0.5) * ndotl;
    float specular = pow(ndotl, 64.0);

    gl_Position = mul(u_viewProjection, worldPos);
    
    v_texcoord0 = a_texcoord0;
    v_color0 = vec4(u_color.rgb * lighting + vec3(specular), u_color.a);
    v_texcoord1.x = (u_fogParams.z - gl_Position.w) * u_fogParams.w;
}

void AdornSelfLitHighlightVS()
{
    vec4 worldPos = mul(u_worldMatrix, vec4(a_position, 1.0));
    vec3 worldNormal = normalize(mul(u_worldMatrix, vec4(a_normal, 0.0)).xyz);

    vec3 light = normalize(u_cameraPosition - worldPos.xyz);
    float ndotl = saturate(dot(worldNormal, light));

    float lighting = 0.75 + (1.0 - 0.75) * ndotl;
    float specular = pow(ndotl, 64.0);

    gl_Position = mul(u_viewProjection, worldPos);
    
    v_texcoord0 = a_texcoord0;
    v_color0 = vec4(u_color.rgb * lighting + vec3(specular), u_color.a);
    v_texcoord1.x = (u_fogParams.z - gl_Position.w) * u_fogParams.w;
}

void AdornVS()
{
    vec4 worldPos = mul(u_worldMatrix, vec4(a_position, 1.0));

#ifdef PIN_LIGHTING
    vec3 worldNormal = normalize(mul(u_worldMatrix, vec4(a_normal, 0.0)).xyz);
    float ndotl = dot(worldNormal, -u_lamp0Dir);
    vec3 lighting = u_ambientColor + saturate(ndotl) * u_lamp0Color + saturate(-ndotl) * u_lamp1Color;
#else
    vec3 lighting = vec3(1.0);
#endif

    gl_Position = mul(u_viewProjection, worldPos);
    
    v_texcoord0 = a_texcoord0;
    v_color0 = vec4(u_color.rgb * lighting, u_color.a);
    v_texcoord1.x = (u_fogParams.z - gl_Position.w) * u_fogParams.w;
}

void AdornAALineVS()
{
    vec4 worldPos = mul(u_worldMatrix, vec4(a_position, 1.0));
    vec3 worldNormal = normalize(mul(u_worldMatrix, vec4(a_normal, 0.0)).xyz);

    // line start and end position in world space
    vec4 startPosW = mul(u_worldMatrix, vec4(1.0, 0.0, 0.0, 1.0));
    vec4 endPosW = mul(u_worldMatrix, vec4(-1.0, 0.0, 0.0, 1.0));

    // Compute view-space w
    float w = dot(u_viewProjection[3], vec4(worldPos.xyz, 1.0));

    // radius in pixels + constant because line has to be little bit bigger to perform anti aliasing
    float radius = u_pixelInfo.w + 2.0;

    // scale the way that line has same size on screen
    if (length(worldPos.xyz - startPosW.xyz) < length(worldPos.xyz - endPosW.xyz))
    {
        float w = dot(u_viewProjection[3], vec4(startPosW.xyz, 1.0));
        float pixel_radius = radius * w * u_pixelInfo.x;
        worldPos.xyz = startPosW.xyz + worldNormal * pixel_radius;
    }
    else
    {
        float w = dot(u_viewProjection[3], vec4(endPosW.xyz, 1.0));
        float pixel_radius = radius * w * u_pixelInfo.x;
        worldPos.xyz = endPosW.xyz + worldNormal * pixel_radius;
    }

    // output for PS
    gl_Position = mul(u_viewProjection, worldPos);
    v_position = gl_Position.xyw;
    v_start = mul(u_viewProjection, startPosW);
    v_end = mul(u_viewProjection, endPosW);
    v_texcoord1.x = (u_fogParams.z - gl_Position.w) * u_fogParams.w;

    // screen ratio
    v_position.y *= u_pixelInfo.z;
    v_start.y *= u_pixelInfo.z;
    v_end.y *= u_pixelInfo.z;
}

void AdornOutlineVS()
{
    vec4 worldPos = mul(u_worldMatrix, vec4(a_position, 1.0));

    gl_Position = mul(u_viewProjection, worldPos);

    v_color0 = u_color;
    v_worldPos = worldPos.xyz;
    v_centerRadius = vec4(mul(u_worldMatrix, vec4(0.0, 0.0, 0.0, 1.0)).xyz, 
                          length(mul(u_worldMatrix, vec4(1.0, 0.0, 0.0, 0.0)).xyz));
}

// Pixel/Fragment Shaders
void AdornPS()
{
    vec4 result = texture2D(s_diffuseMap, v_texcoord0) * v_color0;
    result.rgb = mix(u_fogColor, result.rgb, saturate(v_texcoord1.x));
    gl_FragColor = result;
}

void AdornAALinePS()
{
    vec3 position = v_position / v_position.z;
    vec4 start = v_start / v_start.w;
    vec4 end = v_end / v_end.w;

    vec2 lineDir = normalize(end.xy - start.xy);
    vec2 fragToPoint = position.xy - start.xy;

    // tips of the line are not Anti-Aliased, they are just cut
    // discard as soon as we can
    float startDist = dot(lineDir, fragToPoint);
    float endDist = dot(lineDir, -position.xy + end.xy);
    
    if (startDist < 0.0)
        discard;

    if (endDist < 0.0)
        discard;

    vec2 perpLineDir = vec2(lineDir.y, -lineDir.x);
    float dist = abs(dot(perpLineDir, fragToPoint));

    // high point serves to compute the function which is described below
    float highPoint = 1.0 + (u_pixelInfo.w - 1.0) * 0.5;
    
    // this is function that has this shape /¯¯¯\, it is symmetric, centered around 0 on X axis
    // slope parts are +- 45 degree and are 1px thick. Area of the shape sums to line thickness in pixels
    // function for 1px would be /\, func for 2px is /¯\ and so on...
    vec4 result = vec4(1.0);
    result.a = saturate(highPoint - (dist * 0.5 * u_pixelInfo.y));
    result *= u_color;

    // convert to sRGB, its not perfect for non-black backgrounds, but its the best we can get
    result.a = pow(saturate(1.0 - result.a), 1.0/2.2);
    result.a = 1.0 - result.a;

    result.rgb = mix(u_fogColor, result.rgb, saturate(v_texcoord1.x));
    gl_FragColor = result;
}

void AdornOutlinePS()
{
    vec3 rayO = v_worldPos - v_centerRadius.xyz;
    vec3 rayD = normalize(v_worldPos - u_cameraPosition);

    // magnitude(rayO + t * rayD) = radius
    // t^2 + bt + c = radius
    float thickness = 1.0;

    float r0 = v_centerRadius.w;
    float r1 = max(0.0, v_centerRadius.w - thickness);

    float b = 2.0 * dot(rayO, rayD);
    float c0 = dot(rayO, rayO) - r0 * r0;
    float c1 = dot(rayO, rayO) - r1 * r1;

    if (b * b < 4.0 * c0)
        discard;

    if (b * b > 4.0 * c1)
        discard;

    gl_FragColor = v_color0;
}
