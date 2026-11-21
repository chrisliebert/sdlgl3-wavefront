#version 450
layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = 2) uniform FrameUniforms {
    mat4 projection;
    mat4 view;
    mat4 lightSpaceMatrix;
    vec4 lightPos;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} pcs;

void main()
{
    gl_Position = ubo.lightSpaceMatrix * pcs.model * vec4(inPosition, 1.0);
}
