#version 450 core

#include "bvh.h"
#include "interface.h"
#include "tonemapping.h"
#include "brdf.h"
#include "intersect.h"

vec2 rand_offset2d(float maximum, vec2 right, vec2 up)
{
    vec2 rands = vec2(
        next_random() * 2 * 3.1415926535897f,
        next_random()
    );

    return sqrt(rands.y) * maximum * (sin(rands.x) * right + cos(rands.x) * up);
}

struct light_t
{
    vec3 position;
    float radius;
    vec3 color;
    vec2 attenuation;
} test_lights[2];

vec3 tonemap(vec3 col);

void main()
{
    test_lights[0].position = vec3(4, 5, 4);
    test_lights[0].color = 25*vec3(1, 1.8, 3);
    test_lights[0].radius = 0.2f;
    test_lights[1].position = vec3(-4, 3, 5);
    test_lights[1].color = 12*vec3(3, 1.8, 1);
    test_lights[1].radius = 0.1f;

    vec4 last_color = texelFetch(u_last_image, ivec2(gl_FragCoord.xy), 0);
    int draw_counter = u_draw_counter;

    init_random();
    
    vec2 off = rand_offset2d(1.0f, vec2(0, 1), vec2(1, 0));

    vec2 tsize = vec2(textureSize(u_last_image, 0));
    vec4 ird = (u_inv_view * u_inv_proj * vec4(((off + gl_FragCoord.xy) / tsize) * 2 - 1, 1, 1));
    vec3 in_ray_direction = normalize(ird.xyz);

    vec3 rc = in_ray_origin + u_focus * in_ray_direction;

    
    int bounces = u_max_bounces;
    vec3 path_color = vec3(0, 0, 0);
    vec3 path_reflectance = vec3(1, 1, 1);
    float path_probability = 1;

    int bokeh_tries = 8;
    vec2 bokeh_offset = rand_offset2d(0.49f, vec2(0, 1), vec2(1, 0));
    while(u_has_bokeh && (bokeh_tries == 8 || max(max(path_reflectance.x, path_reflectance.y), path_reflectance.z) < 1e-5) && bokeh_tries-- > 0)
    {
        path_reflectance = textureLod(u_bokeh, vec2(0.5) + bokeh_offset, 0).rgb;
        bokeh_offset = rand_offset2d(0.49f, vec2(0, 1), vec2(1, 0));
    }
    
    if(max(max(path_reflectance.x, path_reflectance.y), path_reflectance.z) < 1e-5)
    {
        path_color = vec3(0, 0, 0);
        out_color = last_color;
        return;
    }
    else
    {
        vec2 scaled_bokeh = bokeh_offset * 2 * u_lens_radius * vec2(tsize.x / tsize.y, 1);
        vec3 ray_origin = in_ray_origin + scaled_bokeh.x * in_pixel_right / u_resolution.x + scaled_bokeh.y * in_pixel_up / u_resolution.y;
        vec3 ray_direction = normalize(rc - ray_origin);
    
        brdf_result_t brdf;
        while (bounces-- > 0)
        {
            ray_direction = normalize(ray_direction);

            hit_t hit = nearest_hit(ray_origin, ray_direction, 1.0 / 0.0);
            for(int l = DONT_OPTIMIZE_ZERO; l < test_lights.length(); ++l)
            {
                float t_light = raySphereIntersect(ray_origin, ray_direction, test_lights[l].position, test_lights[l].radius);
                if(t_light > 0 && (!hit.hits||t_light < hit.t))
                {
                
                    path_color += path_reflectance * test_lights[l].color;
                    break;
                }
            }

            if (!hit.hits)
            {
                if(u_has_cubemap)
                {
                    path_color += pow(path_reflectance * textureLod(u_cubemap, ray_direction, 0).rgb, vec3(1/1.0)) + vec3(0.0);
                }
                else
                {
                    vec3 env = mix(vec3(0.3f, 0.4f, 0.5f), vec3(1.0f, 0.98f, 0.96f), (ray_direction.y + 1) / 2);
                    path_color += path_reflectance * env;
                }
                break;
            }


            bool is_incoming = dot(hit.normal, ray_direction) < 0;
            vec3 normal = is_incoming ? hit.normal : -hit.normal;
            float ior_front = is_incoming ? 1.0 : hit.material.ior;
            float ior_back = is_incoming ? hit.material.ior : 1.0;

            vec4 brdf_randoms = vec4(next_random(), next_random(), next_random(), next_random());

            clear_result(brdf);
            sample_brdf(true, brdf_randoms, hit.material, hit.position, ray_direction, normal, ior_front, ior_back, brdf);

            if(brdf.end_path || brdf.pdf < 1e-3 || isnan(brdf.pdf) || isinf(brdf.pdf) || any(isnan(brdf.reflectance)) || any(isinf(brdf.reflectance)))
            {
                path_color += brdf.reflectance * path_reflectance;
                break;
            }

            float light_random = next_random();

            path_probability *= brdf.pdf;
            path_reflectance *= brdf.reflectance * abs(dot(brdf.continue_direction, normal)) / brdf.pdf;

            if(u_enable_russian_roulette && bounces < u_max_bounces - 3)
            {
                float p = max(max(path_reflectance.x, path_reflectance.y), path_reflectance.z);
                if(next_random() > p)
                {
                    path_color = vec3(0);
                    break;
                }
                path_reflectance *= 1/p;
            }

            // Make light test
            int light_index = clamp(int(test_lights.length() * next_random()), 0, test_lights.length()-1);
            {
                vec3 point_on_light = test_lights[light_index].position + normalize(vec3(next_random(), next_random(), next_random())) * test_lights[light_index].radius * sqrt(next_random());

                vec3 path_to_light = point_on_light - hit.position;
                vec3 direction_to_light = normalize(path_to_light);
                float distance_to_light = length(path_to_light);
    
                brdf_result_t light_test;
                clear_result(light_test);
                light_test.continue_direction = direction_to_light;
                sample_brdf(false, brdf_randoms, hit.material, hit.position, ray_direction, normal, ior_front, ior_back, light_test);

                if(!any_hit(hit.position + light_test.continue_direction * 1.5e-5f, direction_to_light, length(path_to_light)))
                {
                    vec3 ref = light_test.reflectance * test_lights[light_index].color * abs(dot(light_test.continue_direction, normal));
                    path_color += path_reflectance * ref * test_lights.length() / (distance_to_light);
                }
            }   

            ray_direction = brdf.continue_direction;

            vec3 off = ray_direction * 1e-4f;
            vec3 next_ray_origin = hit.position + off;
            ray_origin = next_ray_origin;
        }
    }
    path_color = tonemap(path_color);

    if(any(isnan(path_color)) || any(isinf(path_color)))
        path_color = last_color.rgb;

    if(draw_counter > 0)
    {
        out_color = mix(last_color, vec4(path_color, 1), 1.0f / (draw_counter + 1));
    }
    else
        out_color = vec4(path_color, 1);
}

#include "pathtracer_internal.h"
  
vec3 unreal(vec3 x) {
  return x / (x + 0.155) * 1.019;
}

float unreal(float x) {
  return x / (x + 0.155) * 1.019;
}
vec3 tonemap(vec3 col)
{
  col = clamp(col, 0, 10);
  return unreal(col);
}
