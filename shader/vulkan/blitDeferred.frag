#version 450

#extension GL_GOOGLE_include_directive : require



layout(binding = 11) uniform sampler2D colorTexture;

layout (location = 0) out vec4 outDiffuse;
layout (location = 1) out vec4 outSpecular;
layout (location = 2) out vec4 outNormal;
layout (location = 3) out vec4 outAdditional;

layout(location=0) in VertexData
{
    vec2 tc;
} inData;


void main() 
{
    outDiffuse = texture(colorTexture,inData.tc);
    outSpecular = vec4(0.f);
    outNormal = vec4(0.f);
    outAdditional = vec4(0.f);
}


