#version 450

layout (location = 0) in vec2 fragCoord;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D sampleTexture;

void main() {
    //outColor = vec4(fragCoord,0.0,1.0);
    outColor = vec4(texture(sampleTexture, fragCoord).rgb, fragColor.a);
}