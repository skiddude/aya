#ifndef GLOBALS_SH_HEADER_GUARD
#define GLOBALS_SH_HEADER_GUARD

// Uniform declarations for bgfx
uniform mat4 u_viewProjection;

uniform vec4 u_viewRight;
uniform vec4 u_viewUp;
uniform vec4 u_viewDir;
uniform vec3 u_cameraPosition;

uniform vec3 u_ambientColor;
uniform vec3 u_lamp0Color;
uniform vec3 u_lamp0Dir;
uniform vec3 u_lamp1Color;

uniform vec3 u_fogColor;
uniform vec4 u_fogParams;

uniform vec4 u_lightBorder;
uniform vec4 u_lightConfig0;
uniform vec4 u_lightConfig1;
uniform vec4 u_lightConfig2;
uniform vec4 u_lightConfig3;

uniform vec4 u_fadeDistance_GlowFactor;
uniform vec4 u_outlineBrightness_ShadowInfo;

uniform vec4 u_shadowMatrix0;
uniform vec4 u_shadowMatrix1;
uniform vec4 u_shadowMatrix2;

#endif // GLOBALS_SH_HEADER_GUARD
