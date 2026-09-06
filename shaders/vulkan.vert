#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragViewDir;

layout(push_constant) uniform PushConstants {
    mat4 modelViewMatrix;
    mat4 projectionMatrix;
} pcs;

void main() {
    gl_Position = pcs.projectionMatrix * pcs.modelViewMatrix * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
    fragNormal = mat3(pcs.modelViewMatrix) * inNormal;
    fragViewDir = -vec3(pcs.modelViewMatrix * vec4(inPosition, 1.0));
}
