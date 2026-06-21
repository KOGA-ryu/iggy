#version 450

layout(set = 1, binding = 0) uniform sampler2D baseColorTexture;

layout(location = 0) in vec2 in_uv0;
layout(location = 1) in vec3 in_color;
layout(location = 0) out vec4 out_color;

void main() {
  out_color = texture(baseColorTexture, in_uv0) * vec4(in_color, 1.0);
}
