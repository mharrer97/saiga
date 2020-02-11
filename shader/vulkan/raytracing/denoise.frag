/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * created by Mathias Harrer: mathias.mh.harrer@fau.de
 */

#version 450

layout(binding = 11) uniform sampler2D rtxTexture;
layout(binding = 12) uniform sampler2D normalTexture;
layout(binding = 13) uniform sampler2D dataTexture;

layout(binding = 14) uniform UBO
{
    int maxKernelSize;
    int width;
    int height;
}
ubo;

layout(location = 0) out vec4 outColor;

layout(location = 0) in VertexData
{
    vec2 tc;
}
inData;

//direction offset array
vec2 dirOffsets[49] = {{0,0},{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,-1},{-1,1}
						
						,{2,2},{2,1},{2,0},{2,-1},{2,-2}
						,{1,2},{2,-2}
						,{0,2},{0,-2}
						,{-1,2},{-1,-2}
						,{-2,2},{-2,1},{-2,0},{-2,-1},{-2,-2}
						
						,{3,3},{3,2},{3,1},{3,0},{3,-1},{3,-2},{3,-3}
						,{2,3},{2,-3}
						,{1,3},{1,-3}
						,{0,3},{0,-3}
						,{-1,3},{-1,-3}
						,{-2,3},{-2,-3}
						,{-3,3},{-3,2},{-3,1},{-3,0},{-3,-1},{-3,-2},{-3,-3}
	};
//weight array
float weights[49] = {4,
					4,4,4,4,4,4,4,4,
					4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
					4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4};

void main()
{		
	vec2 data = texture(dataTexture, inData.tc).xy;
	vec3 normal = texture(normalTexture, inData.tc).xyz;
	vec3 col = texture(rtxTexture, inData.tc).xyz * weights[0];
	float div = weights[0];
	for(int i = 1; i<min(49, pow(ubo.maxKernelSize, 2)); ++i){
		vec2 dataTMP = texture(dataTexture, inData.tc+ dirOffsets[i]/vec2(ubo.width,ubo.height)).xy;
		vec3 normTMP = texture(normalTexture, inData.tc+ dirOffsets[i]/vec2(ubo.width,ubo.height)).xyz;
		//scale down blur cosine weighted and reduce if low diffuse-> dot product * (1-diff)
		float alpha = max(0.f, dot(normal,normTMP)) * (1.f - dataTMP.y); 
		col += texture(rtxTexture, inData.tc + dirOffsets[i]/vec2(ubo.width,ubo.height)).xyz * weights[i] * alpha;
		div += weights[i]*alpha;
	}
	col = col/div;
        outColor = vec4(col, 1.f);
}
