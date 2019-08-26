#version 450

#extension GL_GOOGLE_include_directive : require



layout(binding = 11) uniform sampler2D diffuseTexture;
layout(binding = 12) uniform sampler2D specularTexture;
layout(binding = 13) uniform sampler2D normalTexture;
layout(binding = 14) uniform sampler2D additionalTexture;
layout(binding = 15) uniform sampler2D depthexture;

layout (binding = 16) uniform UBO2 
{
//	mat4 projection;
	mat4 view;
	vec4 lightPos;
} ubo;


layout (location = 0) out vec4 outColor;

layout(location=0) in VertexData
{
    vec2 tc;
} inData;


void main() 
{
	
	vec3 diffuseColor = texture(diffuseTexture, inData.tc).rgb;
	vec4 specularAndRoughness = texture(diffuseTexture, inData.tc);
	vec4 additional = texture(additionalTexture, inData.tc); // <-- currently unused
	gl_FragDepth = texture(depthexture, inData.tc).r;

	vec3 N = normalize(texture(normalTexture, inData.tc).rgb);
	vec4 L4 = ubo.view * ubo.lightPos;
	vec3 L = normalize(mat3(ubo.view) * ubo.lightPos.xyz);
	vec3 V = vec3(0.f,0.f,1.f);
	vec3 R = reflect(-L, N);
	
	//float n_dot_l = clamp(dot(N, normalize(L)), 0.f, 1.f);
	
	vec3 diffuse = max(dot(N, L), 0.0) * diffuseColor;
	vec3 specular = pow(max(dot(R, V), 0.0), specularAndRoughness.a * 256.f) * specularAndRoughness.rgb;
	//vec3 specular = pow(max(dot(R, V), 0.0), 16.f) * vec3(0.75);// specularAndRoughness.rgb;
	
	outColor = vec4(diffuse + specular , 1.f);

    //outColor = texture(diffuseTexture,inData.tc) * texture(normalTexture,inData.tc) * max(1.f,texture(specularTexture,inData.tc).r + 0.75f) * texture(additionalTexture,inData.tc);
	//outColor = ubo.lightPos;
	
}


