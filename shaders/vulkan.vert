#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) out vec4 fragPosLightSpace;

layout(set = 0, binding = 2) uniform FrameUniforms {
    mat4 projection;
    mat4 view;
    mat4 lightSpaceMatrix;
    vec4 lightPos;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} pcs;

void main() {
    vec4 worldPos = pcs.model * vec4(inPosition, 1.0);
    vec4 viewPos = ubo.view * worldPos;

    mat4 clip = mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0,-1.0, 0.0, 0.0,
        0.0, 0.0, 0.5, 0.0,
        0.0, 0.0, 0.5, 1.0
    );

    gl_Position = clip * ubo.projection * viewPos;
    fragTexCoord = inTexCoord;
    fragNormal = mat3(transpose(inverse(pcs.model))) * inNormal;
    fragWorldPos = worldPos.xyz;
    fragPosLightSpace = ubo.lightSpaceMatrix * worldPos;
}
