#version 450

layout(location = 0) in vec3 in_position_model;
layout(location = 1) in vec2 in_uv0;
layout(location = 2) in vec3 in_color;

layout(push_constant) uniform DrawPushConstants {
  mat4 clipFromModel;
} draw;

layout(location = 0) out vec2 out_uv0;
layout(location = 1) out vec3 out_color;

void main() {
  gl_Position = draw.clipFromModel * vec4(in_position_model, 1.0);
  out_uv0 = in_uv0;
  out_color = in_color;
}
