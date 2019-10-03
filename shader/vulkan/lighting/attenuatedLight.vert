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

layout (push_constant) uniform PushConstants {
	mat4 model;

} pushConstants;

layout(location=0) out VertexData
{
    vec3 pos;
} outData;

void main() 
{
	vec3 pos = inPosition.xyz ;
    outData.pos = ((ubo.proj * ubo.view * pushConstants.model * vec4(pos,1))).xyw;
    //outData.tc.y = 1.0 - outData.tc.y;
    gl_Position = ubo.proj * ubo.view * pushConstants.model * vec4(pos,1);
    //gl_Position.z = 0.1f;
}
