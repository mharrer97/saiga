/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 *
 * created by Mathias Harrer: mathias.mh.harrer@fau.de
 */

#version 450

layout(binding = 11) uniform sampler2D diffuseTexture;
layout(binding = 12) uniform sampler2D specularTexture;
layout(binding = 13) uniform sampler2D normalTexture;

layout(binding = 14) uniform UBO
{
    int maxKernelSize;
}
ubo;

layout(location = 0) out vec4 outColor;

layout(location = 0) in VertexData
{
    vec2 tc;
}
inData;

void main()
{
	
		outColor = vec4(1.f);
}
