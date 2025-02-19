#version 450

layout(binding = 1) uniform sampler2D textureSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTextureCoords;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor * texture(textureSampler, fragTextureCoords).rgb, 1.0);
}
