#version 460 core

#extension GL_KHR_vulkan_glsl : enable

#include "test_include.glsl"

layout(local_size_x = 8, local_size_y = 8) in;
layout(set = 0, binding = 0, rgba16f) uniform image2D out_image;

void main() {
  ivec2 gid = ivec2(gl_GlobalInvocationID.xy);
  ivec2 size = ivec2(imageSize(out_image));
  if(any(greaterThanEqual(gid, size)))
    return;

  imageStore(out_image, gid, vec4(vec2(gid) / vec2(size), 0.f, 1.f));
}
