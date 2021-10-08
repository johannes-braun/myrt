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
layout(location = 1) in vec4 normal;
layout(location = 2) in vec2 uv;

uniform mat4 view;
uniform mat4 proj;

layout(binding = 0, std430) restrict readonly buffer Geometries { drawable_geometry_t geometries[]; };

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec3 out_position;
layout(location = 2) out vec2 out_uv;
layout(location = 3) flat out int out_material;
//layout(location = 4) flat out vec3 cam_pos;

void main()
{
  drawable_geometry_t geo = geometries[gl_DrawID];

  //cam_pos = inverse(view)[3].xyz;

  out_uv = uv;
  vec4 pt = geo.transformation * vec4(point.xyz, 1);
  out_position = pt.xyz;
  gl_Position = proj * view * pt;
  out_normal = (transpose(geo.inverse_transformation) * vec4(normal.xyz, 0)).xyz;
  out_material = geo.material_index;
}