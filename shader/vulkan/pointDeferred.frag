#version 450

//layout (location = 1) in vec3 inColor;

layout(location=0) in VertexData
{
  vec3 color;
} inData;


layout (location = 0) out vec4 outDiffuse;
layout (location = 1) out vec4 outSpecular;
layout (location = 2) out vec4 outNormal;
layout (location = 3) out vec4 outAdditional;

void main() 
{   
	outDiffuse = vec4(inData.color, 1.0);
	outSpecular = vec4(1.f);
	outNormal = vec4(0.f);
	outAdditional = vec4(0.f, 0.f, 0.f, 1.f);	
}
