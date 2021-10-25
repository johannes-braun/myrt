#version 460 core

#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_debug_printf : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in mat4 in_transform;

layout(push_constant) uniform constants {
  mat4 view_matrix;
  mat4 projection_matrix;
} in_constants;

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec2 out_uv;

void main()
{
  out_position = (in_transform * vec4(in_position, 1)).xyz;
  out_normal = (transpose(inverse(in_transform)) * vec4(in_position, 1)).xyz;
  out_uv = in_uv;

  gl_Position = in_constants.projection_matrix * in_constants.view_matrix * vec4(out_position, 1);
}