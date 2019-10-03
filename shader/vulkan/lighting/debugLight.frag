#version 450

#extension GL_GOOGLE_include_directive : require

#include "light_helper.glsl"


layout (binding = 16) uniform UBO2 
{
	mat4 proj;
	mat4 view;
} ubo;

layout (push_constant) uniform PushConstants {
	mat4 model;

} pushConstants;

layout (location = 0) out vec4 outColor;

layout(location=0) in VertexData
{
    vec3 pos;
} inData;

vec3 reconstructPosition(float d, vec2 tc){
    vec4 p = vec4(tc.xy * 2.0f - 1.0f,d,1);
    p = inverse(ubo.proj) * p; //TODO outsource inverse to cpu?
    return p.xyz/p.w;
}

void main() 
{
	
	outColor = vec4(1.f);
	
	
}



