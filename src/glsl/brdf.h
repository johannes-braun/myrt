#pragma once
#include "interface.h"
#include "ggx.h"
struct brdf_result_t
{
    vec3 reflectance;
    vec3 continue_direction;
    float pdf;
    bool end_path;
};

#define clear_result(result) {\
    result.reflectance = vec3(0);\
    result.continue_direction = vec3(0);\
    result.pdf = 0;\
    result.end_path = false;\
}

// https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf


void lambert_resample(material_info_t material, vec4 randoms, vec3 position, vec3 incoming, vec3 normal, float ior_front, float ior_back, inout brdf_result_t result)
{
    result.continue_direction = bsdf_local_to_world(sample_cosine_hemisphere(randoms.xy), normal);
}

void lambert_eval(material_info_t material, vec4 randoms, vec3 position, vec3 incoming, vec3 normal, float ior_front, float ior_back, inout brdf_result_t result)
{
    result.pdf = max(0, dot(normal, result.continue_direction)) / pi;
    float g_x = max(0, dot(normal, result.continue_direction)) / pi;
    result.reflectance = albedo(material).rgb * g_x / dot(result.continue_direction, normal);
}
void lambert_brdf(bool do_resample, vec4 randoms, material_info_t material, vec3 position, vec3 incoming, vec3 normal, float ior_front, float ior_back, inout brdf_result_t result) {
    if(do_resample) lambert_resample(material, randoms, position, incoming, normal, ior_front, ior_back, result);
    lambert_eval(material, randoms, position, incoming, normal, ior_front, ior_back, result);
}

float fresnel(float cos_theta, float ior_in, float ior_out, float metallic)
{
    float f0 = abs((ior_in - ior_out) / (ior_in + ior_out));
    f0 = mix(f0*f0, 1.0, metallic);
    return f0 + (1-f0) * pow(1 - cos_theta, 5);
}

void ggx_pbr_resample(bool do_resample, vec4 randoms, material_info_t material, vec3 position, vec3 incoming, vec3 normal, float ior_front, float ior_back, inout brdf_result_t result)
{
    vec2 importance_sample = ggx_importance_sample(randoms.xy, material.roughness * material.roughness);
    vec3 hemisphere_sample = sample_hemisphere(importance_sample);
    vec3 microfacet_normal = bsdf_local_to_world(hemisphere_sample, normal);

    result.continue_direction = reflect(incoming, microfacet_normal);
}

void ggx_pbr_eval(bool do_resample, vec4 randoms, material_info_t material, vec3 position, vec3 incoming, vec3 normal, float ior_front, float ior_back, inout brdf_result_t result)
{
    float roughness = material.roughness * material.roughness;
    vec3 microfacet_normal = normalize(sign(dot(-incoming, normal)) * (-incoming + result.continue_direction));
    float D = ggx_distribution(normal, microfacet_normal, roughness);
    float G = ggx_geometry(incoming, result.continue_direction, normal, microfacet_normal, roughness);
    // F is done by just the fresnel based random sampling

    float den = 4*abs(dot(-incoming, normal))* abs(dot(result.continue_direction, normal));

    float jacobian = 1 / (4 * abs(dot(result.continue_direction, microfacet_normal)));

    float cos_theta = (dot(microfacet_normal, normal));
    result.pdf = D * (cos_theta) * jacobian;
    result.reflectance = mix(vec3(1), albedo(material).rgb, material.metallic) * D * G / den;
}
void ggx_pbr_brdf(bool do_resample, vec4 randoms, material_info_t material, vec3 position, vec3 incoming, vec3 normal, float ior_front, float ior_back, inout brdf_result_t result)
{
    if (do_resample)
        ggx_pbr_resample(do_resample, randoms, material, position, incoming, normal, ior_front, ior_back, result);

    ggx_pbr_eval(do_resample, randoms, material, position, incoming, normal, ior_front, ior_back, result);
}

void ggx_pbr_transmit_resample(bool do_resample, vec4 randoms, material_info_t material, vec3 position, vec3 incoming, vec3 normal, float ior_front, float ior_back, inout brdf_result_t result)
{
    vec2 importance_sample = ggx_importance_sample(randoms.xy, material.roughness * material.roughness);
    vec3 hemisphere_sample = sample_hemisphere(importance_sample);
    vec3 microfacet_normal = bsdf_local_to_world(hemisphere_sample, normal);

    result.continue_direction = refract(incoming, microfacet_normal, ior_front / ior_back);
}

void ggx_pbr_transmit_eval(bool do_resample, vec4 randoms, material_info_t material, vec3 position, vec3 incoming, vec3 normal, float ior_front, float ior_back, inout brdf_result_t result)
{
    float roughness = material.roughness * material.roughness;
    vec3 microfacet_normal = normalize(-(ior_front * -incoming + ior_back * result.continue_direction));
    //// The computation above yields the "outside" normal when going from inside to outside 
    //// but here we always need the one pointing towards the viewer.
    microfacet_normal *= sign(dot(microfacet_normal, normal));

    float D = ggx_distribution(normal, microfacet_normal, roughness);
    float G = ggx_geometry(incoming, result.continue_direction, normal, microfacet_normal, roughness);
    //// F is done by just the fresnel based random sampling

    float idoth = dot(-incoming, microfacet_normal);
    float odoth = dot(result.continue_direction, microfacet_normal);
    float abs_idotn = abs(dot(-incoming, normal));
    float abs_odotn = abs(dot(result.continue_direction, normal));

    float prefac = ((abs(idoth) * abs(odoth)) / (abs_idotn * abs_odotn));
    float den = (ior_front * idoth + ior_back * odoth);

    float ior2_by_den2 = ior_back * ior_back / (den * den);
    float ft = prefac * G * D * ior2_by_den2;
    float jacobian = ior2_by_den2 * abs(odoth);

    float cos_theta = (dot(microfacet_normal, normal));
    result.pdf = cos_theta * D * jacobian;
    result.reflectance = albedo(material).rgb*ft;
}

void ggx_pbr_btdf(bool do_resample, vec4 randoms, material_info_t material, vec3 position, vec3 incoming, vec3 normal, float ior_front, float ior_back, inout brdf_result_t result)
{
    if(do_resample)
        ggx_pbr_transmit_resample(do_resample, randoms, material, position, incoming, normal, ior_front, ior_back, result);

    ggx_pbr_transmit_eval(do_resample, randoms, material, position, incoming, normal, ior_front, ior_back, result);
}

void sample_brdf(bool do_resample, vec4 randoms, material_info_t material, vec3 position, vec3 incoming, vec3 normal, float ior_front, float ior_back, inout brdf_result_t result)
{
    float fresnel_cos_theta = dot(-incoming, normal);
    float F = fresnel(fresnel_cos_theta, ior_front, ior_back, material.metallic);
    float random = randoms.z;
    float transmission_rnd = randoms.w;
    bool transmits = transmission_rnd <= material.transmission;

    // Widen distribution such that there are no infinite distribution values for low roughnesses.
    float clamping = 0.97;
    material.roughness = clamp(material.roughness * clamping + (1 - clamping), (1 - clamping), 1.0);

    bool should_reflect = false;
    if (transmits)
    {
        vec2 importance_sample = ggx_importance_sample(randoms.xy, material.roughness * material.roughness);
        vec3 hemisphere_sample = sample_hemisphere(importance_sample);
        vec3 microfacet_normal = bsdf_local_to_world(hemisphere_sample, normal);
        vec3 ref = refract(incoming, microfacet_normal, ior_front / ior_back);
        should_reflect = ref == vec3(0);
    }
    if (F > random || should_reflect)
    {
        ggx_pbr_brdf(do_resample, randoms, material, position, incoming, normal, ior_front, ior_back, result);
    }
    else
    {
        if(transmits)
            ggx_pbr_btdf(do_resample, randoms, material, position, incoming, normal, ior_front, ior_back, result);
        else
        {
            lambert_brdf(do_resample, randoms, material, position, incoming, normal, ior_front, ior_back, result);
        }
    }
}