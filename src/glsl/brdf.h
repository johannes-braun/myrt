#pragma once
#include "interface.h"

const float pi = 3.1415926535897;

struct brdf_result_t
{
    vec3 reflectance;
    vec3 continue_direction;
    float pdf;
    bool end_path;
};

void clear_result(inout brdf_result_t result)
{
    result.reflectance = vec3(0);
    result.continue_direction = vec3(0);
    result.pdf = 0;
    result.end_path = false;
}

void lambert_brdf(vec3 albedo, vec3 position, vec3 incoming, vec3 normal, float ior_front, float ior_back, inout brdf_result_t result)
{
    result.continue_direction = bsdf_local_to_world(sample_cosine_hemisphere(vec2(next_random(), next_random())), normal);
    result.pdf = max(0, dot(normal, result.continue_direction)) / pi;
    float g_x = max(0, dot(normal, result.continue_direction)) / pi;
    result.reflectance = albedo * g_x / result.pdf;
}

void reflect_brdf(vec3 albedo, vec3 position, vec3 incoming, vec3 normal, float ior_front, float ior_back, inout brdf_result_t result)
{
    result.continue_direction = reflect(incoming, normal);
    result.pdf = length(albedo);
    result.reflectance = albedo;
}

void refract_brdf(vec3 albedo, vec3 position, vec3 incoming, vec3 normal, float ior_front, float ior_back, inout brdf_result_t result)
{
    result.continue_direction = normalize(refract(incoming, normal, ior_front/ior_back));
    if (any(isnan(result.continue_direction)))
    {
        reflect_brdf(albedo, position, incoming, normal, ior_front, ior_back, result);
        return;
    }

    result.pdf = length(albedo);
    result.reflectance = albedo;
}

void fresnel_simple(vec3 albedo, vec3 position, vec3 incoming, vec3 normal, float ior_front, float ior_back, inout brdf_result_t result)
{
    float r0 = (ior_front - ior_back) / (ior_front + ior_back);
    r0 *= r0;
    float fresnel = r0 + (1 - r0) * pow(1 - dot(normal, -incoming), 5);
    if (fresnel > next_random())
    {
        reflect_brdf(vec3(1), position, incoming, normal, ior_front, ior_back, result);
    }
    else
    {
        lambert_brdf(albedo, position, incoming, normal, ior_front, ior_back, result);
    }
}

void sample_brdf(vec3 albedo, vec3 position, vec3 incoming, vec3 normal, float ior_front, float ior_back, inout brdf_result_t result)
{
    if (all(greaterThanEqual(albedo, vec3(1))))
    {
        result.reflectance = albedo;
        result.end_path = true;
        return;
    }

    float rough = 0.3;
    float a = rough * rough;
    vec3 mfn = bsdf_local_to_world(normalize(vec3(a, a, 1) * sample_cosine_hemisphere(vec2(next_random(), next_random()))), normal);
    if (dot(mfn, incoming) >= 0)
        mfn = -mfn + 2 * normal;
    
    fresnel_simple(albedo, position, incoming, mfn, ior_front, ior_back, result);

    float mpdf = max(0, dot(normal, mfn)) / pi / a;
    float g_x = max(0, dot(normal, mfn)) / pi / a;
    result.reflectance *= g_x / mpdf;
    result.pdf *= mpdf;
}