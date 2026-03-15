#version 450

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 _;
layout(location = 2) in vec3 in_color;

layout(location = 3) in vec2 in_offset;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(in_position + in_offset, 0.0, 1.0);
    fragColor = in_color;
}