#version 450

layout (location = 0) in vec2 position;
layout (location = 1) in vec4 color;
layout (location = 0) out vec4 fragColor;

layout( push_constant ) uniform constants {
  layout(offset = 0) mat4 transform;
} world;

void main() {
    gl_Position = world.transform * vec4(position, 0.0, 1.0);
    fragColor = color;
}