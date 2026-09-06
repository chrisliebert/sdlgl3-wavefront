#version 450
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragViewDir;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D diffuseTex;

void main() {
    vec4 texColor = texture(diffuseTex, fragTexCoord);
    vec3 n = normalize(fragNormal);
    vec3 v = normalize(fragViewDir);
    vec3 l = normalize(vec3(10.0, 10.0, 10.0)); // simple hardcoded light

    float diff = max(dot(n, l), 0.2); // basic ambient + diffuse
    outColor = vec4(texColor.rgb * diff, texColor.a);
}
