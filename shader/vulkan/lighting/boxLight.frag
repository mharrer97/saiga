#version 450

#extension GL_GOOGLE_include_directive : require

#include "light_helper.glsl"


layout(binding = 11) uniform sampler2D diffuseTexture;
layout(binding = 12) uniform sampler2D specularTexture;
layout(binding = 13) uniform sampler2D normalTexture;
layout(binding = 14) uniform sampler2D additionalTexture;
layout(binding = 15) uniform sampler2D depthexture;

layout (binding = 16) uniform UBO2 
{
	mat4 proj;
	mat4 view;
} ubo;

layout (push_constant) uniform PushConstants {
	mat4 model;
	mat4 depthBiasMV;
	vec4 lightSpecularCol;
	vec4 lightDiffuseCol;
} pushConstants;

layout (location = 0) out vec4 outColor;

layout(location=0) in VertexData
{
    vec3 pos;
} inData;


void main() 
{
	vec2 tc = (inData.pos.xy/inData.pos.z) * 0.5f + 0.5f;
	vec3 diffuseColor = texture(diffuseTexture, tc).rgb;
	vec4 specularAndRoughness = texture(specularTexture, tc);
	vec4 additional = texture(additionalTexture, tc); // <-- currently unused //w contains information if light calculation should be applied: 1 = no lighting
	float depth = texture(depthexture, tc).r;

	vec4 P = vec4(reconstructPosition(depth, tc, ubo.proj), 1.f);
	vec4 N = vec4(normalize(texture(normalTexture, tc).rgb), 1.f);
	vec3 L = normalize(mat3(ubo.view) * normalize(vec3(pushConstants.model[2])));//viewLightPos.xyz - P.xyz;
	vec3 R = reflect(normalize(L), N.xyz);
	vec3 V = normalize(P.xyz);
	
	
	vec4 vLight = pushConstants.depthBiasMV * P;
    vLight = vLight / vLight.w;
    float fragmentInLight = 0;
    if(vLight.x>0 && vLight.x<1 && vLight.y>0 && vLight.y<1&& vLight.z>0 && vLight.z<1)
        fragmentInLight = 1;
        
    //intensity calculation for smooth edges
    float smoothModifier = mix(0.f, 1.f, min(vLight.x * 20.f,1.f));
	smoothModifier *= mix(0.f, 1.f, min(vLight.y * 20.f, 1.f));
	smoothModifier *= mix(0.f, 1.f, min(vLight.z * 20.f, 1.f));
	smoothModifier *= mix(0.f, 1.f, min((1.f-vLight.x) * 20.f, 1.f));
	smoothModifier *= mix(0.f, 1.f, min((1.f-vLight.y) * 20.f, 1.f));
	smoothModifier *= mix(0.f, 1.f, min((1.f-vLight.z) * 20.f, 1.f));


	float intensity = smoothModifier * fragmentInLight * pushConstants.lightDiffuseCol.w; //  getAttenuation(pushConstants.attenuation, length(L)) * 
	
	vec3 diffuse = max(dot(normalize(N.xyz), normalize(L)) * intensity, 0.f) * diffuseColor * pushConstants.lightDiffuseCol.xyz;
	vec3 specular = pow(max(dot(R,V), 0.f), specularAndRoughness.a * 256.f) * specularAndRoughness.rgb * intensity * pushConstants.lightSpecularCol.xyz;
	outColor = vec4(diffuse + specular, 1.f);
	

	
	if(additional.w > 0.99f) {
		outColor = vec4(0.f);
	}	
}



