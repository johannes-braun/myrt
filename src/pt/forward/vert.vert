#version 460 core

struct drawable_geometry_t
{
    mat4 transformation;
    mat4 inverse_transformation;
    uint bvh_node_base_index;
    uint bvh_index_base_index;
    uint indices_base_index;
    uint points_base_index;
    uint index_count;

    int material_index;
    int geometry_index;
    int pad[1];
};

layout(location = 0) in vec4 point;
#ifndef FORWARD_ONLY_POSITION
layout(location = 1) in vec4 normal;
layout(location = 2) in vec2 uv;
#endif // FORWARD_ONLY_POSITION

uniform mat4 view;
uniform mat4 proj;

layout(binding = 0, std430) restrict readonly buffer Geometries { drawable_geometry_t geometries[]; };

layout(location = 0) out vec3 out_position;
#ifndef FORWARD_ONLY_POSITION
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec2 out_uv;
layout(location = 3) flat out int out_material;
#endif // FORWARD_ONLY_POSITION

void main()
{
  drawable_geometry_t geo = geometries[gl_DrawID];

  vec4 pt = geo.transformation * vec4(point.xyz, 1);
  out_position = pt.xyz;
  gl_Position = proj * view * pt;

#ifndef FORWARD_ONLY_POSITION
  out_uv = uv;
  out_normal = (transpose(geo.inverse_transformation) * vec4(normal.xyz, 0)).xyz;
  out_material = geo.material_index;
#endif // FORWARD_ONLY_POSITION
}