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
	vec4 lightPos;
	float intensity;
	bool debug;
} ubo;

layout (push_constant) uniform PushConstants {
	vec4 lightPos;
	vec4 attenuation;
} pushConstants;

layout (location = 0) out vec4 outColor;

layout(location=0) in VertexData
{
    vec4 pos;
} inData;

vec3 reconstructPosition(float d, vec2 tc){
    vec4 p = vec4(tc.xy * 2.0f - 1.0f,d,1);
    p = inverse(ubo.proj) * p; //TODO outsource inverse to cpu?
    return p.xyz/p.w;
}

void main() 
{
	
	vec3 diffuseColor = texture(diffuseTexture, inData.pos.xy).rgb;
	vec4 specularAndRoughness = texture(specularTexture, inData.pos.xy);
	vec4 additional = texture(additionalTexture, inData.pos.xy); // <-- currently unused //w contains information if light calculation should be applied: 1 = no lighting
	float depth = texture(depthexture, inData.pos.xy).r;
	gl_FragDepth = depth;

	vec4 viewLightPos = ubo.view * pushConstants.lightPos;
	//vec3 viewLightDir = mat3(ubo.view) * (-ubo.lightDir).xyz;
	//vec4 viewLightPos = ubo.view * vec4(5.f,5.f,5.f,1.f);
	//vec3 viewLightDir = (ubo.view * vec4(1.f,1.f,1.f,0.f)).xyz;
	vec4 P = vec4(reconstructPosition(depth, inData.pos.xy), 1.f);
	vec4 N = vec4(normalize(texture(normalTexture, inData.pos.xy).rgb), 1.f);
	vec3 L = viewLightPos.xyz - P.xyz;
	vec3 R = reflect(normalize(L), N.xyz);
	vec3 V = normalize(P.xyz);
	
	float intensity = getAttenuation(pushConstants.attenuation, length(L));
	
	vec3 diffuse = max(dot(normalize(N.xyz), normalize(L)) * intensity, 0.f) * diffuseColor;
	vec3 specular = pow(max(dot(R,V), 0.f), specularAndRoughness.a * 256.f) * specularAndRoughness.rgb * intensity;
	outColor = vec4(diffuse + specular, 1.f);
	//if(acos(dot(normalize(L), normalize(viewLightDir))) > ((ubo.lightAngle/4.f)/180.f)*6.26f) outColor = vec4(0.05f * diffuseColor, 1.f);
	//float angle = acos(dot(normalize(L), normalize(viewLightDir)));
	//float alpha = (clamp((angle/6.26) * 180.f, (ubo.lightAngle/2.f) - 2.5f, (ubo.lightAngle/2.f) + 2.5f)- ((ubo.lightAngle/2.f) - 2.5f)) / 5.f;
	//outColor = mix(vec4(0.05f * diffuseColor, 1.f), vec4(diffuse + specular, 1.f) ,1.f- alpha);
	
	
	if(additional.w > 0.99f) {
		outColor = vec4(0.f);
	}
	outColor = vec4(1);
//	outColor = vec4();
		
	//outColor = vec4 ( 0.f, 0.f, 0.f, 0.25f);
	/*if(ubo.debug) {
		vec2 tc2 = inData.tc * 2.f;
		if (inData.tc.x < 0.5 && inData.tc.y >= 0.5) //linker unterer bereich der anzeige
				//outColor = vec4(vec3(depth), 1);
				outColor = vec4(texture(specularTexture, tc2 -vec2(0,1)).rgb, 1);
				//outColor = vec4(vec3(dot(v, V)),1.f);
		else if (inData.tc.x >= 0.5 && inData.tc.y >= 0.5) //rechter unterer bereich der anzeige
				outColor = vec4(texture(additionalTexture, tc2 - vec2(1,1)).rgb, 1);
		else if (inData.tc.x < 0.5 && inData.tc.y < 0.5) //linker oberer
				outColor = vec4(texture(diffuseTexture, tc2).rgb, 1);
		else //rechter oberer
				outColor = vec4(texture(normalTexture, tc2 - vec2(1,0)).rgb, 1);
	}*/
	
	
	//outColor = vec4(N, 1.f);
    //outColor = texture(diffuseTexture,inData.tc) * texture(normalTexture,inData.tc) * max(1.f,texture(specularTexture,inData.tc).r + 0.75f) * texture(additionalTexture,inData.tc);
	//outColor = ubo.lightPos;
	
}



