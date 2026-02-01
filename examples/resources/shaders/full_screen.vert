#version 450

layout(location = 0) out vec2 uv_out;

void main() {
    uv_out = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(uv_out * 2.0 - 1.0, 0.0, 1.0);
}