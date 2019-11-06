/**
 * Copyright (c) 2017 Darius Rückert 
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * created by Mathias Harrer: mathias.mh.harrer@fau.de
 */
 


 #version 450

#extension GL_GOOGLE_include_directive : require

#include "vulkan/lighting/light_helper.glsl"

layout(binding = 11) uniform sampler2D diffuseTexture;
layout(binding = 12) uniform sampler2D specularTexture;
layout(binding = 13) uniform sampler2D normalTexture;
layout(binding = 14) uniform sampler2D additionalTexture;
layout(binding = 15) uniform sampler2D depthTexture;

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
	vec4 lightSpecularCol;
	vec4 lightDiffuseCol;
	vec4 lightDirection;
	float ambientIntensity;
} pushConstants;


layout (location = 0) out vec4 outColor;

layout(location=0) in VertexData
{
    vec2 tc;
} inData;


void main() 
{
	
    vec2 tc = inData.tc;
    vec3 diffuseColor = texture(diffuseTexture, tc).rgb;
    vec4 specularAndRoughness = texture(specularTexture, tc);
    vec4 additional = texture(additionalTexture, tc); // <-- currently unused //w contains information if light calculation should be applied: 1 = no lighting
    float depth = texture(depthTexture, tc).r;
    gl_FragDepth = depth;

    vec4 P = vec4(reconstructPosition(depth, tc, ubo.proj), 1.f);
    vec4 N = vec4(normalize(texture(normalTexture, tc).rgb), 1.f);
    vec3 L = -normalize(mat3(ubo.view) * pushConstants.lightDirection.xyz);//normalize(vec3(pushConstants.model[2])));//viewLightPos.xyz - P.xyz;
    vec3 R = reflect(normalize(L), N.xyz);
    vec3 V = normalize(P.xyz);




    float intensity = pushConstants.lightDiffuseCol.w; //getAttenuation(pushConstants.attenuation, length(L)) *

    vec3 ambient = pushConstants.ambientIntensity * diffuseColor;
    vec3 diffuse = max(dot(normalize(N.xyz), normalize(L)) * intensity, 0.f) * diffuseColor * pushConstants.lightDiffuseCol.xyz;
    vec3 specular = pow(max(dot(R,V), 0.f), specularAndRoughness.a * 256.f) * specularAndRoughness.rgb * intensity * pushConstants.lightSpecularCol.xyz;
    outColor = vec4(ambient + diffuse + specular, 1.f);


    if(additional.w > 0.99f) {
            outColor = vec4(diffuseColor, 1.f);
    }

    vec4 vLight = pushConstants.depthBiasMV * P;
    vLight = vLight / vLight.w;
    bool fragmentInShadowMap = false;
    if(vLight.x>0 && vLight.x<1 && vLight.y>0 && vLight.y<1&& vLight.z>0 && vLight.z<1)
        fragmentInShadowMap = true;
    //float shadowMapDepth = texture(shadowmap, vLight.xy).r;
    //if( (shadowMapDepth * 0.5 + 0.5  < vLight.z - 0.00005 && fragmentInShadowMap) ) outColor = vec4(ambient , 1.f);
    if(fragmentInShadowMap) outColor = vec4(ambient + filterPCF(vLight, shadowmap, pushConstants.ambientIntensity) * (diffuse + specular), 1.f);
	
    if(additional.w > 0.99f) {
            outColor = vec4(diffuseColor, 1.f);
    }
    //float shadowTex = texture(shadowmap, tc).r;
    //outColor = vec4(vec3(shadowTex),1.f);
    if(!ubo.debug) {
            vec2 tc2 = inData.tc * 2.f;
            if (inData.tc.x < 0.5 && inData.tc.y >= 0.5){ //linker unterer bereich der anzeige
                            //outColor = vec4(vec3(depth), 1);
                            outColor = vec4(texture(specularTexture, tc2 -vec2(0,1)).rgb, 1);
                            //outColor = vec4(vec3(dot(v, V)),1.f);
                            float depth2 = texture(depthTexture, tc2 - vec2(0,1)).r;
                            float z_n = 2.0 * depth2 - 1.0;
                            float zNear = 0.1f;
                            float zFar = 100.f;
                            float z_e = 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));
                            outColor = vec4(vec3(z_e),1.f);
            }
            else if (inData.tc.x >= 0.5 && inData.tc.y >= 0.5){ //rechter unterer bereich der anzeige
                            outColor = vec4(texture(additionalTexture, tc2 - vec2(1,1)).rgb, 1);
                            outColor = vec4(-reconstructPosition(depth, tc2- vec2(1,1), ubo.proj), 1.f);;

            }
            else if (inData.tc.x < 0.5 && inData.tc.y < 0.5) //linker oberer
                            outColor = vec4(texture(diffuseTexture, tc2).rgb, 1);
            else //rechter oberer
                            outColor = vec4(texture(normalTexture, tc2 - vec2(1,0)).rgb, 1);

    }
	

}



