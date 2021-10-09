#version 460 core
#extension GL_ARB_bindless_texture : require

layout(location = 0) in vec3 position;
#ifndef FORWARD_ONLY_POSITION
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) flat in int material;
#endif // FORWARD_ONLY_POSITION

layout(location = 0) out vec4 out_position;
#ifndef FORWARD_ONLY_POSITION
layout(location = 1) out vec4 out_uv;
layout(location = 2) out vec4 out_normal_mat;
#endif // FORWARD_ONLY_POSITION

void main(){
  out_position.xyz = position;
  out_position.w = 1;

#ifndef FORWARD_ONLY_POSITION
  out_uv.xy = uv;
  out_normal_mat.xyz = normalize(normal);
  out_normal_mat.w = float(material);
#endif // FORWARD_ONLY_POSITION
}