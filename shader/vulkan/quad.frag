#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_GOOGLE_include_directive : require

#include "test.glsl"

/*layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inViewVec;
layout (location = 3) in vec3 inLightVec;*/

layout (binding = 10) uniform sampler2D diffuseS;
layout (binding = 11) uniform sampler2D specularS;
layout (binding = 12) uniform sampler2D normalS;
layout (binding = 13) uniform sampler2D additionalS;

layout (location = 0) in vec2 inTC;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	/*vec3 N = normalize(texture(texSampler, inTc).rgb);
	vec3 L = normalize(inLightVec);
	vec3 V = normalize(inViewVec);
	vec3 R = reflect(-L, N);
	vec3 diffuse = texture(diffuseS, inTc.xy).rgb;
	vec3 specular = texture(specularS, inTc.xy).rgb;*/
	outFragColor = vec4(1.f,0.f,1.f,1.f);//vec4(texture(diffuseS, inTC).rgb, 1.f);//vec4(diffuse  + specular, 1.0);		
}
