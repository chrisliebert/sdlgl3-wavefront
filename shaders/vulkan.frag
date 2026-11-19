#version 450
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) in vec4 fragPosLightSpace;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D diffuseTex;
layout(set = 0, binding = 1) uniform sampler2D shadowMap;

layout(set = 0, binding = 2) uniform FrameUniforms {
    mat4 projection;
    mat4 view;
    mat4 lightSpaceMatrix;
    vec4 lightPos;
} ubo;

float shadowCalculation(vec4 fragLightSpacePos, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragLightSpacePos.xyz / max(fragLightSpacePos.w, 0.0001);
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0 || projCoords.z > 1.0)
    {
        return 0.0;
    }

    float currentDepth = projCoords.z;
    float bias = max(0.0025 * (1.0 - dot(normal, lightDir)), 0.00075);

    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    float shadow = 0.0;
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

void main() {
    vec4 texColor = texture(diffuseTex, fragTexCoord);
    vec3 n = normalize(fragNormal);
    vec3 l = normalize(ubo.lightPos.xyz - fragWorldPos);

    float ambient = 0.2;
    float diffuse = max(dot(n, l), 0.0);
    float shadow = shadowCalculation(fragPosLightSpace, n, l);
    float lighting = ambient + (1.0 - shadow) * diffuse;

    outColor = vec4(texColor.rgb * lighting, texColor.a);
}
