#pragma once

struct drawable_geometry_t
{
    mat4 transformation;
    mat4 inverse_transformation;
    uint bvh_node_base_index;
    uint bvh_index_base_index;
    uint indices_base_index;
    uint points_base_index;

    int material_index;
    int geometry_index;
    int pad[2];
};

struct material_info_t
{
    uint albedo_rgba_unorm;
    float ior;
};
vec4 albedo(material_info_t info)
{
    return unpackUnorm4x8(info.albedo_rgba_unorm);
}

// BUFFERS
layout(binding = 0, std430) restrict readonly buffer BVH { bvh_node_t bvh_nodes[]; };
layout(binding = 1, std430) restrict readonly buffer BVHIndices { uint bvh_indices[]; };
layout(binding = 2, std430) restrict readonly buffer MeshIndices { uint indices[]; };
layout(binding = 3, std430) restrict readonly buffer MeshPoints { vec4 points[]; };
layout(binding = 4, std430) restrict readonly buffer MeshNormals { vec4 normals[]; };
layout(binding = 5, std430) restrict readonly buffer Geometries { drawable_geometry_t geometries[]; };
layout(binding = 6, std430) restrict readonly buffer BVHGlobal { bvh_node_t global_bvh_nodes[]; };
layout(binding = 7, std430) restrict readonly buffer BVHIndicesGlobal { uint global_bvh_indices[]; };
layout(binding = 8, std430) restrict readonly buffer Materials { material_info_t materials[]; };

// UNIFORMS
layout(location = 0) uniform mat4 u_inv_proj;
layout(location = 1) uniform mat4 u_inv_view;
layout(location = 2) uniform vec2 u_resolution;
layout(location = 3) uniform uint u_random_seed;
layout(location = 4) uniform int u_draw_counter;
layout(location = 5) uniform bool u_has_cubemap;
layout(location = 6) uniform float u_lens_radius;
layout(location = 7) uniform bool u_has_bokeh;
layout(location = 8) uniform float u_focus;

// IMAGES
layout(binding = 0) uniform samplerCube u_cubemap;
layout(binding = 1) uniform sampler1D u_random_texture;
layout(binding = 2) uniform sampler2D u_last_image;
layout(binding = 3) uniform sampler2D u_bokeh;

// PASSED VARYINGS
layout(location = 0) in vec3 in_ray_origin;
layout(location = 2) in vec3 in_pixel_right;
layout(location = 3) in vec3 in_pixel_up;

// OUTPUTS
layout(location = 0) out vec4 out_color;

bool any_hit(vec3 ray_origin, vec3 ray_direction, float max_distance);
bool nearest_hit(vec3 ray_origin, vec3 ray_direction, float max_distance, out float t, out vec2 barycentric, out uint triangle, out uint geometry);

void init_random();
float next_random();

vec3 sample_cosine_hemisphere(vec2 uv);
vec3 bsdf_local_to_world(const in vec3 vector, const in vec3 normal);