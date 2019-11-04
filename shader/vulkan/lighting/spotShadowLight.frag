#version 450

#extension GL_GOOGLE_include_directive : require

#include "light_helper.glsl"


layout(binding = 11) uniform sampler2D diffuseTexture;
layout(binding = 12) uniform sampler2D specularTexture;
layout(binding = 13) uniform sampler2D normalTexture;
layout(binding = 14) uniform sampler2D additionalTexture;
layout(binding = 15) uniform sampler2D depthexture;

layout(binding = 16) uniform sampler2D shadowmap;

layout (binding = 17) uniform UBO2
{
	mat4 proj;
	mat4 view;
	bool debug;
} ubo;

layout (push_constant) uniform PushConstants {
	mat4 model;
	mat4 depthBiasMV;
	vec4 lightPos;
	vec4 attenuation;
	vec4 lightDir;
	vec4 lightSpecularCol;
	vec4 lightDiffuseCol;
	float openingAngle;
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
	//gl_FragDepth = depth;

	vec4 viewLightPos = ubo.view * pushConstants.lightPos;
	vec3 viewLightDir = mat3(ubo.view) * (-pushConstants.lightDir).xyz;
	//vec4 viewLightPos = ubo.view * vec4(5.f,5.f,5.f,1.f);
	//vec3 viewLightDir = (ubo.view * vec4(1.f,1.f,1.f,0.f)).xyz;
	vec4 P = vec4(reconstructPosition(depth, tc, ubo.proj), 1.f);
	vec4 N = vec4(normalize(texture(normalTexture, tc).rgb), 1.f);
	vec3 L = viewLightPos.xyz - P.xyz;
	vec3 R = reflect(normalize(L), N.xyz);
	vec3 V = normalize(P.xyz);
	
	float intensity = getAttenuation(pushConstants.attenuation, length(L)) * pushConstants.lightDiffuseCol.w;
	
	vec3 diffuse = max(dot(normalize(N.xyz), normalize(L)) * intensity, 0.f) * diffuseColor * pushConstants.lightDiffuseCol.xyz;
	vec3 specular = pow(max(dot(R,V), 0.f), specularAndRoughness.a * 256.f) * specularAndRoughness.rgb * intensity * pushConstants.lightSpecularCol.xyz;
	outColor = vec4(diffuse + specular, 1.f);
	
	if(acos(dot(normalize(L), normalize(viewLightDir))) > ((pushConstants.openingAngle/4.f)/180.f)*6.26f) outColor = vec4(0.f);
	float angle = acos(dot(normalize(L), normalize(viewLightDir)));
	float alpha = (clamp((angle/6.26) * 180.f, (pushConstants.openingAngle/4.f) - 5.f, (pushConstants.openingAngle/4.f))- ((pushConstants.openingAngle/4.f) - 5.f)) / 5.f;
	outColor = mix(vec4(vec3(0.f), 1.f), vec4(diffuse + specular, 1.f) ,1.f- alpha);
	
	vec4 vLight = pushConstants.depthBiasMV * P;
    vLight = vLight / vLight.w;
    bool fragmentInShadowMap = false;
    if(vLight.x>0 && vLight.x<1 && vLight.y>0 && vLight.y<1&& vLight.z>0 && vLight.z<1)
        fragmentInShadowMap = true;
    if(fragmentInShadowMap) outColor = mix(vec4(vec3(0.f), 1.f), vec4(filterPCF(vLight, shadowmap, 0.f) * (diffuse + specular), 1.f) ,1.f- alpha);
	
	if(additional.w > 0.99f) {
		outColor = vec4(0.f);
	}

	
	if(ubo.debug) {
		float angleDegree = (angle/6.26f) * 360.f;
		outColor = vec4(pushConstants.openingAngle/360.f);
		float diff = abs((pushConstants.openingAngle/2.f) - angleDegree);
		 if(angleDegree < (pushConstants.openingAngle/2.f)-20.f)outColor = vec4(vec3(angleDegree),1.f);
		 else if(angleDegree < (pushConstants.openingAngle/2.f)-10.f) outColor = vec4(1.f,0.f,0.f,1.f);
		 else if(angleDegree < (pushConstants.openingAngle/2.f)) outColor = vec4(0.f,1.f,0.f,1.f);
		 else if(angleDegree < (pushConstants.openingAngle/2.f)+10.f) outColor = vec4(0.f,0.f,1.f,1.f);
		 else if(angleDegree < (pushConstants.openingAngle/2.f)+20.f) outColor = vec4(1.f,0.f,1.f,1.f);
		 if(diff < 0.5f)outColor = vec4(1.f,1.f,0.f,1.f);
		 if(intensity < 0.001f) outColor = vec4(0.f,1.f,1.f,1.f);
	}
	//outColor = vec4(diffuseColor, 1);
		
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



