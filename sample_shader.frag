#version 450

layout (location = 0) in vec2 fragCoord;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    //outColor = texture(sampleTexture, fragCoord) * fragColor;
    outColor = fragColor;
}