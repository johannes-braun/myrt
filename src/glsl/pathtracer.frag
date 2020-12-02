#version 450 core

#include "bvh.h"
#include "interface.h"
#include "brdf.h"

vec3 rand_offset3d(float maximum, vec3 right, vec3 up)
{
    vec2 rands = vec2(
        next_random() * 2 * 3.1415926535897f,
        next_random()
    );

    return sqrt(rands.y) * maximum * (sin(rands.x) * right + cos(rands.x) * up);
}
vec2 rand_offset2d(float maximum, vec2 right, vec2 up)
{
    vec2 rands = vec2(
        next_random() * 2 * 3.1415926535897f,
        next_random()
    );

    return sqrt(rands.y) * maximum * (sin(rands.x) * right + cos(rands.x) * up);
}

void main()
{
    vec4 last_color = texelFetch(u_last_image, ivec2(gl_FragCoord.xy), 0);
    int draw_counter = u_draw_counter;

    init_random();
    
    vec2 off = rand_offset2d(1.0f, vec2(0, 1), vec2(1, 0));

    vec2 tsize = vec2(textureSize(u_last_image, 0));
    vec4 ird = (u_inv_view * u_inv_proj*vec4(((off + gl_FragCoord.xy) / tsize)*2-1, 1, 1));
    vec3 in_ray_direction = normalize(ird.xyz);

    vec3 rc = in_ray_origin + u_focus * in_ray_direction;

    vec2 bokeh_offset = rand_offset2d(0.49f, vec2(0, 1), vec2(1, 0));
    
    const int max_bounces = 16;
    int bounces = max_bounces;
    vec3 path_color = vec3(0, 0, 0);
    vec3 path_reflectance = vec3(1, 1, 1);
    float path_probability = 1;

    if(u_has_bokeh)
    {
        path_reflectance = textureLod(u_bokeh, vec2(0.5) + bokeh_offset, 0).rgb;
    }

    vec2 scaled_bokeh = bokeh_offset * 2 * u_lens_radius * vec2(tsize.x / tsize.y, 1);
    vec3 ray_origin = in_ray_origin + scaled_bokeh.x * in_pixel_right / u_resolution.x + scaled_bokeh.y * in_pixel_up / u_resolution.y;
    vec3 ray_direction = normalize(rc - ray_origin);
    
    float hit_t = 1.0 / 0.0;
    vec2 hit_bary = vec2(0, 0);
    uint hit_triangle = 0;
    uint hit_geometry = 0;

    brdf_result_t brdf;
    while (bounces-- > 0)
    {
        if(dot(path_reflectance, path_reflectance) < 0.01 * 0.01){
            break;
        }

        ray_direction = normalize(ray_direction);

        if (!nearest_hit(ray_origin, ray_direction, 1.0 / 0.0, hit_t, hit_bary, hit_triangle, hit_geometry))
        {
            if(u_has_cubemap)
            {
                path_color = path_reflectance * pow(textureLod(u_cubemap, ray_direction, 0), vec4(1 / 2.2f)).rgb;
            }
            else
            {
                vec3 env = mix(vec3(0.3f, 0.4f, 0.5f), vec3(1.0f, 0.98f, 0.96f), (ray_direction.y + 1) / 2);
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
        
        material_info_t material = materials[geometries[hit_geometry].material_index];

        float ior_front = is_incoming ? 1.0 : material.ior;
        float ior_back = is_incoming ? material.ior : 1.0;

        clear_result(brdf);
        sample_brdf(albedo(material).rgb, ray_origin, ray_direction, normal, ior_front, ior_back, brdf);
        
        if(brdf.end_path)
        {
            path_color = brdf.reflectance * path_reflectance;
            break;
        }        

        ray_direction = brdf.continue_direction;
        path_probability *= brdf.pdf;
        path_reflectance *= brdf.reflectance;

        vec3 off = ray_direction * 1.5e-5f;
        vec3 next_ray_origin = hit_point + off;
        ray_origin = next_ray_origin;
    }

    path_color = clamp(path_color, 0.0f, 8.0f);
    if(draw_counter > 0)
    {
        out_color = mix(last_color, vec4(path_color, 1), 1.0f / (draw_counter + 1));
    }
    else
        out_color = vec4(path_color, 1);
}

#include "pathtracer_internal.h"
