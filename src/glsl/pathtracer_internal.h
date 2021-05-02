#pragma once

#include "intersect.h"
#include "raymarch.h"

// Forward declare visitor functions.
bool visit_triangle(MYRT_INDEX_TYPE node_base_index, MYRT_INDEX_TYPE primitive_base_index, vec3 ro, vec3 rd, float maxt);
bool visit_object_aabb(MYRT_INDEX_TYPE node_base_index, MYRT_INDEX_TYPE primitive_base_index, vec3 ro, vec3 rd, float maxt);

#define MYRT_BVH_NODES_BUFFER bvh_nodes
#define MYRT_BVH_INDICES_BUFFER bvh_indices
#define MYRT_BVH_TRAVERSE bvh_traverse
#define MYRT_BVH_VISIT_PRIMITIVE visit_triangle
#include "bvh.impl.h"
#undef MYRT_BVH_VISIT_PRIMITIVE
#undef MYRT_BVH_TRAVERSE
#undef MYRT_BVH_INDICES_BUFFER
#undef MYRT_BVH_NODES_BUFFER

#define MYRT_BVH_NODES_BUFFER global_bvh_nodes
#define MYRT_BVH_INDICES_BUFFER global_bvh_indices
#define MYRT_BVH_TRAVERSE bvh_traverse_global
#define MYRT_BVH_VISIT_PRIMITIVE visit_object_aabb
#include "bvh.impl.h"
#undef MYRT_BVH_VISIT_PRIMITIVE
#undef MYRT_BVH_TRAVERSE
#undef MYRT_BVH_INDICES_BUFFER
#undef MYRT_BVH_NODES_BUFFER

struct traversal_state_t
{
    bool any_hit;
    float t;
    float global_t;
    vec2 barycentric;
    vec2 global_barycentric;
    uint hit_geometry;
    uint hit_index;
    uint global_hit_index;

    uint base_index;
    uint base_vertex;
    uint current_geometry;
} traversal;

void traversal_init() {
    const float inf = 1.0 / 0.0;
    traversal.any_hit = false;
    traversal.t = inf;
    traversal.global_t = inf;
    traversal.barycentric = vec2(0, 0);
    traversal.global_barycentric = vec2(0, 0);
    traversal.hit_geometry = 0;
    traversal.hit_index = 0;
    traversal.global_hit_index = 0;
    traversal.base_index = 0;
    traversal.base_vertex = 0;
}

bool visit_triangle(vec3 ray_origin, vec3 ray_direction, uint index, inout float max_ray_distance, out bool hits)
{
    vec3 p0 = points[traversal.base_vertex + indices[traversal.base_index + index * 3 + 0]].xyz;
    vec3 p1 = points[traversal.base_vertex + indices[traversal.base_index + index * 3 + 1]].xyz;
    vec3 p2 = points[traversal.base_vertex + indices[traversal.base_index + index * 3 + 2]].xyz;

    float t_current = 0;
    vec2 barycentric_current = vec2(0, 0);
    hits = intersect_triangle(ray_origin, ray_direction, p0, p1, p2, t_current, barycentric_current) &&
        t_current < max_ray_distance&&
        traversal.t > t_current;

    if (hits)
    {
        traversal.t = t_current;
        traversal.barycentric = barycentric_current;
        traversal.hit_index = index;
        max_ray_distance = traversal.t;
    }

    if (traversal.any_hit && hits) return true;
    return false;
}

bool visit_object_aabb(vec3 ray_origin, vec3 ray_direction, uint i, inout float max_ray_distance, out bool hits)
{
    traversal.base_index = geometries[i].indices_base_index;
    traversal.base_vertex = geometries[i].points_base_index;
    vec3 ro = (geometries[i].inverse_transformation * vec4(ray_origin, 1)).xyz;
    vec3 rd = (((geometries[i].inverse_transformation) * vec4(ray_direction, 0)).xyz);
    traversal.t = max_ray_distance;
    bool current_hits = bvh_traverse(geometries[i].bvh_node_base_index, geometries[i].bvh_index_base_index, ro, rd, max_ray_distance);
    current_hits = current_hits && traversal.global_t > traversal.t;

    if (current_hits)
    {
        traversal.global_t = traversal.t;
        traversal.global_barycentric = traversal.barycentric;
        traversal.global_hit_index = traversal.hit_index;
        traversal.hit_geometry = i;
        max_ray_distance = traversal.global_t;
    }

    hits = current_hits;

    if (traversal.any_hit && hits) return true;
    return false;
}

bool any_hit(vec3 ray_origin, vec3 ray_direction, float max_distance)
{
    traversal_init();
    traversal.any_hit = true;
    traversal.t = max_distance;
    traversal.global_t = max_distance;
    bool hits = bvh_traverse_global(0, 0, ray_origin, ray_direction, max_distance);

    if (!hits)
    {
      for (int i = 0; i < sdfs.length(); ++i)
      {
        sdf_current = i;
        if (march_sdf(ray_origin, ray_direction, max_distance, true).hits)
          return true;
      }
    }
    return hits;
}

hit_t nearest_hit(vec3 ray_origin, vec3 ray_direction, float max_distance)
{
    hit_t hit;

    traversal_init();
    traversal.any_hit = false;
    traversal.global_t = max_distance;
    hit.hits = bvh_traverse_global(0, 0, ray_origin, ray_direction, max_distance);

    hit.t = traversal.global_t;

    bool hit_marched = false;
    for (int i = 0; i < sdfs.length(); ++i)
    {
      sdf_current = i;
      hit_t h = march_sdf(ray_origin, ray_direction, hit.hits ? hit.t : max_distance, false);
      if (h.hits && h.t < hit.t) {
        hit = h;
        hit_marched = true;
      }
    }
    if (hit_marched)
      return hit;

    vec2 hit_bary = traversal.global_barycentric;
    uint hit_triangle = traversal.global_hit_index;
    uint hit_geometry = traversal.hit_geometry;

    uint base_vertex = geometries[hit_geometry].points_base_index;
    uint base_index = geometries[hit_geometry].indices_base_index;

    vec3 p0 = points[base_vertex + indices[base_index + 3 * hit_triangle + 0]].xyz;
    vec3 p1 = points[base_vertex + indices[base_index + 3 * hit_triangle + 1]].xyz;
    vec3 p2 = points[base_vertex + indices[base_index + 3 * hit_triangle + 2]].xyz;
    hit.position = (geometries[hit_geometry].transformation * vec4(hit_bary.x * p1 + hit_bary.y * p2 + (1.0 - hit_bary.x - hit_bary.y) * p0, 1)).xyz;
    
    vec3 n0 = normals[base_vertex + indices[base_index + 3 * hit_triangle + 0]].xyz;
    vec3 n1 = normals[base_vertex + indices[base_index + 3 * hit_triangle + 1]].xyz;
    vec3 n2 = normals[base_vertex + indices[base_index + 3 * hit_triangle + 2]].xyz;
    mat4 normal_matrix = mat4(mat3(transpose(geometries[hit_geometry].inverse_transformation)));
    vec3 normal = normalize((normal_matrix
        * vec4((hit_bary.x * n1 + hit_bary.y * n2 + (1.0 - hit_bary.x - hit_bary.y) * n0), 0)).xyz);

    vec3 face_normal = (normal_matrix * vec4(normalize(cross(normalize(p1 - p0), normalize(p2 - p0))), 0)).xyz;
    if (sign(dot(normal, ray_direction)) != sign(dot(face_normal, ray_direction)))
    {
        normal = face_normal;
    }
    hit.normal = normal;

    hit.material = materials[geometries[hit_geometry].material_index];

    return hit;
}

const float inverse_max_uint = 1.f / float(0xFFFFFFFFu);
const float random_primes[6] = { 69019.f, 96013.f, 32159.f, 22783.f, 87011.f, 45263.f };
uint random_wang_hash(uint a)
{
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    return a;
}
uint rng_state = 0;
int  current_prime = 0;
uint random_xorshift()
{
    // Xorshift algorithm from George Marsaglia's paper
    rng_state ^= (rng_state << 13);
    rng_state ^= (rng_state >> 17);
    rng_state ^= (rng_state << 5);
    return rng_state;
}

int random_count;
float random_value(int seed)
{
    rng_state += random_wang_hash(uint(seed + 0xaaffb6 * texelFetch(u_random_texture, seed ^ 0x377fba, 0)));
    return clamp(float(random_xorshift()) * inverse_max_uint, 0.f, 1.f);
}
void init_random()
{
    random_count = int(textureSize(u_random_texture, 0));
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    int seed_base = int(random_value(pixel.x ^ 0xba77fa) * random_primes[0] + random_value(pixel.y ^ 0xcca6df) * random_primes[1] + random_value(int(u_random_seed))
        + random_value(int(u_draw_counter)));
    random_value(seed_base + int(0xffaf86 * texelFetch(u_random_texture, int(seed_base % random_count), 0).x));
}
float next_random()
{
    return random_value(current_prime = (current_prime + 1) % 6);
}

vec3 sample_cosine_hemisphere(vec2 uv)
{
    // (Uniformly) sample a point on the unit disk
    float r = sqrt(uv.x);
    float theta = 2 * 3.14159265359f * uv.y;
    float x = r * cos(theta);
    float y = r * sin(theta);

    // Project point up to the unit sphere
    float z = float(sqrt(max(0.f, 1 - x * x - y * y)));
    return vec3(x, y, z);
}
vec3 bsdf_local_to_world(const in vec3 vector, const in vec3 normal)
{
    // Find an axis that is not parallel to normal
    vec3 perp = vec3(1, 0, 0);
    vec3 u = normalize(cross(normal, perp));
    for (int i = 1; any(isnan(u)) && i < 3; ++i)
    {
        perp[i - 1] = 0;
        perp[i] = 1;
        u = normalize(cross(normal, perp));
    }
    return normalize(mat3(u, cross(normal, u), normal) * vector);
}