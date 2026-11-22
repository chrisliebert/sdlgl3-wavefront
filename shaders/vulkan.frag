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

const vec2 poissonDisk[16] = vec2[](
    vec2(-0.94201624, -0.39906216),
    vec2( 0.94558609, -0.76890725),
    vec2(-0.09418410, -0.92938870),
    vec2( 0.34495938,  0.29387760),
    vec2(-0.91588581,  0.45771432),
    vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543,  0.27676845),
    vec2( 0.97484398,  0.75648379),
    vec2( 0.44323325, -0.97511554),
    vec2( 0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023),
    vec2( 0.79197514,  0.19090188),
    vec2(-0.24188840,  0.99706507),
    vec2(-0.81409955,  0.91437590),
    vec2( 0.19984126,  0.78641367),
    vec2( 0.14383161, -0.14100790)
);

float shadowCalculation(vec4 fragLightSpacePos, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragLightSpacePos.xyz / max(fragLightSpacePos.w, 0.0001);
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0 || projCoords.z < 0.0 || projCoords.z > 1.0)
    {
        return 0.0;
    }

    float currentDepth = projCoords.z;
    float bias = max(0.0015 * (1.0 - dot(normal, lightDir)), 0.00035);

    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    float shadow = 0.0;
    float filterRadius = 2.0;
    for (int i = 0; i < 16; ++i)
    {
        float pcfDepth = texture(shadowMap, projCoords.xy + poissonDisk[i] * texelSize * filterRadius).r;
        shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
    }
    return shadow / 16.0;
}

void main() {
    vec4 texColor = texture(diffuseTex, fragTexCoord);
    vec3 n = normalize(fragNormal);
    vec3 l = normalize(ubo.lightPos.xyz);

    float ambient = 0.2;
    float diffuse = max(dot(n, l), 0.0);
    float shadow = shadowCalculation(fragPosLightSpace, n, l);
    float lighting = ambient + (1.0 - shadow) * diffuse;

    vec3 baseColor = (dot(texColor.rgb, texColor.rgb) < 1e-6) ? vec3(0.85, 0.80, 0.65) : texColor.rgb;
    float alpha = (texColor.a < 0.01) ? 1.0 : texColor.a;
    outColor = vec4(baseColor * lighting, alpha);
}
