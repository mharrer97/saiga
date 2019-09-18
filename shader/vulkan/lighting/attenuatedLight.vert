#version 450


layout(location=0) in vec4 inPosition;

layout (binding = 7) uniform UBO2 
{
	mat4 proj;
	mat4 view;
} ubo;

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(location=0) out VertexData
{
    vec4 pos;
} outData;

void main() 
{
    outData.pos = ubo.proj * ubo.view * vec4(inPosition.xyz,1);
    //outData.tc.y = 1.0 - outData.tc.y;
    gl_Position = ubo.proj * ubo.view * vec4(inPosition.xyz,1);
}
