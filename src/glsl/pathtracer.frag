#version 450 core

#include "bvh.h"
bool visit_triangle(MYRT_INDEX_TYPE node_base_index, MYRT_INDEX_TYPE primitive_base_index, vec3 ro, vec3 rd, float maxt);
bool visit_object_aabb(MYRT_INDEX_TYPE node_base_index, MYRT_INDEX_TYPE primitive_base_index, vec3 ro, vec3 rd, float maxt);

struct drawable_geometry_t
{
    mat4 transformation;
    mat4 inverse_transformation;
    uint bvh_node_base_index;
    uint bvh_index_base_index;
    uint indices_base_index;
    uint points_base_index;
};

// BUFFERS
layout(binding = 0, std430) restrict readonly buffer BVH { bvh_node_t bvh_nodes[]; };
layout(binding = 1, std430) restrict readonly buffer BVHIndices { uint bvh_indices[]; };
layout(binding = 2, std430) restrict readonly buffer MeshIndices { uint indices[]; };
layout(binding = 3, std430) restrict readonly buffer MeshPoints { vec4 points[]; };
layout(binding = 4, std430) restrict readonly buffer MeshNormals { vec4 normals[]; };
layout(binding = 5, std430) restrict readonly buffer Geometries { drawable_geometry_t geometries[]; };
layout(binding = 6, std430) restrict readonly buffer BVHGlobal { bvh_node_t global_bvh_nodes[]; };
layout(binding = 7, std430) restrict readonly buffer BVHIndicesGlobal { uint global_bvh_indices[]; };

// UNIFORMS
layout(location = 2) uniform vec2 u_resolution;
layout(location = 3) uniform uint u_random_seed;
layout(location = 4) uniform int u_draw_counter;
layout(location = 5) uniform bool u_has_cubemap;

// IMAGES
layout(binding = 0) uniform samplerCube u_cubemap;
layout(binding = 1) uniform sampler1D u_random_texture;
layout(binding = 2) uniform sampler2D u_last_image;

// PASSED VARYINGS
layout(location = 0) in vec3 in_ray_origin;
layout(location = 1) in vec3 in_ray_direction;
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

float teapot_ior = 1.4;

vec3 lambert_brdf(vec3 albedo, vec3 position, vec3 incoming, vec3 normal, inout vec3 outgoing, inout float pdf)
{
    const float pi = 3.1415926535897;

    outgoing = bsdf_local_to_world(sample_cosine_hemisphere(vec2(next_random(), next_random())), normal);   
    pdf = max(0, dot(normal, outgoing)) / pi;
    float g_x = max(0, dot(normal, outgoing)) / pi;
    return albedo * g_x / pdf;
}

vec3 reflect_brdf(vec3 position, vec3 incoming, vec3 normal, inout vec3 outgoing, inout float pdf)
{
    outgoing = reflect(incoming, normal);
    pdf = 1.0;
    return vec3(1);
}

void main()
{
    vec4 last_color = texelFetch(u_last_image, ivec2(gl_FragCoord.xy), 0);
    int draw_counter = u_draw_counter;

    init_random();
    
    vec3 rc = in_ray_origin + 7 * in_ray_direction;
    vec3 ray_origin = in_ray_origin + 100 * (next_random() - 0.5) * in_pixel_right / u_resolution.x + 100 * (next_random() - 0.5) * in_pixel_up / u_resolution.y;
    vec3 ray_direction = normalize(rc - ray_origin);
    ray_origin = ray_origin + 6 * (next_random() - 0.5) * in_pixel_right / u_resolution.x + 6 * (next_random() - 0.5) * in_pixel_up / u_resolution.y;

    const int max_bounces = 10;
    int bounces = max_bounces;
    vec3 path_color = vec3(0, 0, 0);
    vec3 path_reflectance = vec3(1, 1, 1);
    float path_probability = 1;
    
    float hit_t = 1.0 / 0.0;
    vec2 hit_bary = vec2(0, 0);
    uint hit_triangle = 0;
    uint hit_geometry = 0;
    while (bounces-- > 0)
    {
        ray_direction = normalize(ray_direction);

        if (!nearest_hit(ray_origin, ray_direction, 1.0 / 0.0, hit_t, hit_bary, hit_triangle, hit_geometry))
        {
            if(u_has_cubemap)
            {
                path_color = path_reflectance * pow(texture(u_cubemap, ray_direction), vec4(1 / 1.1f)).rgb;
            }
            else
            {
                vec3 env = mix(vec3(0.7f, 0.5f, 0.3f), vec3(0.9f, 0.95f, 1.f), (ray_direction.y + 1) / 2);
                path_color = path_reflectance * env;
            }
            break;
        }

        uint base_vertex = geometries[hit_geometry].points_base_index;
        uint base_index = geometries[hit_geometry].indices_base_index;

        vec3 p0 = points[base_vertex + indices[base_index + 3 * hit_triangle + 0]].xyz;
        vec3 p1 = points[base_vertex + indices[base_index + 3 * hit_triangle + 1]].xyz;
        vec3 p2 = points[base_vertex + indices[base_index + 3 * hit_triangle + 2]].xyz;
        vec3 hit_point = (geometries[hit_geometry].transformation * vec4(hit_bary.x * p1 + hit_bary.y * p2 + (1.0 - hit_bary.x - hit_bary.y) * p0, 1)).xyz;

        vec3 n0 = normals[base_vertex + indices[base_index + 3 * hit_triangle + 0]].xyz;
        vec3 n1 = normals[base_vertex + indices[base_index + 3 * hit_triangle + 1]].xyz;
        vec3 n2 = normals[base_vertex + indices[base_index + 3 * hit_triangle + 2]].xyz;
        mat4 normal_matrix = mat4(mat3(transpose(geometries[hit_geometry].inverse_transformation)));
        vec3 normal = (normal_matrix
            * vec4(normalize(hit_bary.x * n1 + hit_bary.y * n2 + (1.0 - hit_bary.x - hit_bary.y) * n0), 0)).xyz;

        vec3 face_normal = (normal_matrix * vec4(normalize(cross(normalize(p1 - p0), normalize(p2 - p0))), 0)).xyz;
        if (sign(dot(normal, ray_direction)) != sign(dot(face_normal, ray_direction)))
            normal = face_normal;
            
        bool is_incoming = dot(normal, ray_direction) < 0;
        normal = faceforward(normal, ray_direction, normal);

        
        vec3 teapot_color = hit_triangle > 2400 ? vec3(1.f, 0.1f, 0.06f) : vec3(1);

        float probability = 1;
        float r0 = (1 - teapot_ior) / (1 + teapot_ior);
        r0 *= r0;
        float fresnel = r0 + (1 - r0) * pow(1 - dot(normal, -ray_direction), 5);
        if(fresnel > next_random())
        {
            path_reflectance *= reflect_brdf(teapot_color, ray_direction, normal, ray_direction, probability);
        }
        else
        {
            path_reflectance *= lambert_brdf(teapot_color, hit_point, ray_direction, normal, ray_direction, probability);
        }

        path_probability *= probability;
        
        vec3 off = ray_direction * 1.5e-5f;
        vec3 next_ray_origin = hit_point + off;
        ray_origin = next_ray_origin;
    }

    if(draw_counter >= 0)
        out_color = mix(last_color, vec4(path_color, 1), 1.0 / (draw_counter + 1));
    else
        out_color = vec4(path_color, 1);
}

#include "pathtracer_internal.h"
