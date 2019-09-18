#version 450


layout(location=0) in vec3 inPosition;


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
	vec3 pos = inPosition.xyz * 10.f;
    outData.pos = (ubo.proj * ubo.view * vec4(pos,1));
    //outData.tc.y = 1.0 - outData.tc.y;
    gl_Position = ubo.proj * ubo.view * vec4(pos,1);
    //gl_Position.z = 0.1f;
}
