#version 450

layout(location = 0) in vec3 in_position_asset;
layout(location = 1) in vec2 in_uv0;
layout(location = 2) in vec3 in_base_color;
layout(location = 3) in vec3 in_normal_asset;
layout(location = 4) in vec4 in_model_column0;
layout(location = 5) in vec4 in_model_column1;
layout(location = 6) in vec4 in_model_column2;
layout(location = 7) in vec4 in_model_column3;
layout(location = 8) in vec4 in_normal_column0;
layout(location = 9) in vec4 in_normal_column1;
layout(location = 10) in vec4 in_normal_column2;

layout(push_constant) uniform DrawPushConstants {
  mat4 clipFromWorld;
} draw;

layout(location = 0) out vec3 out_color;

void main() {
  mat4 modelFromAsset = mat4(in_model_column0, in_model_column1,
                             in_model_column2, in_model_column3);
  mat3 normalFromAsset = mat3(in_normal_column0.xyz, in_normal_column1.xyz,
                              in_normal_column2.xyz);
  vec3 worldNormal = normalFromAsset * in_normal_asset;
  float normalLength = length(worldNormal);
  vec3 unitNormal = normalLength > 0.000001
                        ? worldNormal / normalLength
                        : vec3(0.0);
  vec3 light = normalize(vec3(-0.35, 0.85, 0.40));
  float brightness = 0.52 + 0.48 * max(0.0, dot(unitNormal, light));

  gl_Position = draw.clipFromWorld *
                modelFromAsset * vec4(in_position_asset, 1.0);
  out_color = in_base_color * brightness;
}
