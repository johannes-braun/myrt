#version 460 core

#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec4 out_attachment0;
layout(location = 1) out vec4 out_attachment1;

void write_position(vec3 position)
{
  out_attachment0.rgb = position;
}

void write_normal(vec3 normal)
{
  out_attachment1.rgb = normal;
}

void write_uv(vec2 uv)
{
  out_attachment0.w = uv.x;
  out_attachment1.w = uv.y;
}

void main()
{
  write_position(in_normal);
  write_normal(in_normal);
  write_uv(vec2(1, 1));
}