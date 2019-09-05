#version 450

#extension GL_GOOGLE_include_directive : require



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
	bool debug;
} ubo;


layout (location = 0) out vec4 outColor;

layout(location=0) in VertexData
{
    vec2 tc;
} inData;

vec3 reconstructPosition(float d, vec2 tc){
    vec4 p = vec4(tc.x,tc.y,d,1)*2.0f - 1.0f;
    p = inverse(ubo.proj) * p; //TODO outsource inverse to cpu?
    return p.xyz/p.w;
}

void main() 
{
	
	vec3 diffuseColor = texture(diffuseTexture, inData.tc).rgb;
	vec4 specularAndRoughness = texture(specularTexture, inData.tc);
	vec4 additional = texture(additionalTexture, inData.tc); // <-- currently unused //w contains information if light calculation should be applied: 1 = no lighting
	float depth = texture(depthexture, inData.tc).r;
	gl_FragDepth = depth;

	vec3 N = normalize(texture(normalTexture, inData.tc).rgb);
	vec4 L4 = ubo.view * ubo.lightPos;
	vec3 v = normalize(additional.rgb);//vec3(0.f,0.f,1.f);
	vec3 V = -normalize(reconstructPosition(depth, inData.tc));
	vec3 L = normalize((mat3(ubo.view) * ubo.lightPos.xyz) - V);
	vec3 R = reflect(-L, N);

	//float n_dot_l = clamp(dot(N, normalize(L)), 0.f, 1.f);
	
	vec3 diffuse = max(dot(N, L), 0.1) * diffuseColor;
	vec3 specular = pow(max(dot(R, V), 0.0), specularAndRoughness.a * 256.f) * specularAndRoughness.rgb;
	//vec3 specular = pow(max(dot(R, V), 0.0), 16.f) * vec3(0.75);// specularAndRoughness.rgb;
	
	outColor = vec4(diffuse + specular , 1.f);

	if(additional.w > 0.99f) {
		outColor = vec4(diffuseColor, 1.f);
	}
		
	if(ubo.debug) {
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
	}
	
	
	//outColor = vec4(N, 1.f);
    //outColor = texture(diffuseTexture,inData.tc) * texture(normalTexture,inData.tc) * max(1.f,texture(specularTexture,inData.tc).r + 0.75f) * texture(additionalTexture,inData.tc);
	//outColor = ubo.lightPos;
	
}



