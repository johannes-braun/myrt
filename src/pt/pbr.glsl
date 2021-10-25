#pragma once

#include "color.glsl"
#include "ggx.glsl"

#define material_info_t pbr_matinfo_t
struct material_info_t
{
  vec4 color;
  float ior;
  float roughness;
  float metallic;
  float transmission;
};

struct brdf_result_t
{
  vec3 reflectance;
  float pdf;
};

#define clear_result(result) {              \
    (result).reflectance = vec3(0);         \
    (result).continue_direction = vec3(0);  \
    (result).pdf = 0;                       \
    (result).end_path = false;              \
}

float fresnel(float cos_theta, float ior_in, float ior_out, float metallic)
{
  float f0 = abs((ior_in - ior_out) / (ior_in + ior_out));
  f0 = mix(f0 * f0, 1.0, metallic);
  return f0 + (1 - f0) * pow(1 - cos_theta, 5);
}
//
//vec3 pbr_ggx_resample(vec2 random_sample, material_info_t material, vec3 towards_view, vec3 normal, float ior_front, float ior_back)
//{
//  vec2 importance_sample = ggx_importance_sample(random_sample, material.roughness * material.roughness);
//  vec3 hemisphere_sample = sample_hemisphere(importance_sample);
//  vec3 microfacet_normal = transform_local_to_world(hemisphere_sample, normal);
//
//  vec3 ray = reflect(-towards_view, microfacet_normal);
//
//  float d = dot(ray, normal);
//  if (d < 0)
//    ray -= 2 * d * normal;
//
//  return normalize(ray);
//}

vec3 nreflect(vec3 towards_view, vec3 normal, vec3 microfacet_normal)
{
  /*float d = dot(towards_view, normal);
  float thresh = 0.25;
  if (d < thresh)
    towards_view += 2*abs(d - thresh) * normal;*/

  vec3 ray = reflect(-towards_view, microfacet_normal);

  float d = dot(ray, normal);
  if (d < 0)
    ray -= 2 * d * normal;

  return normalize(ray);
}

const int s_reflect = 0;
const int s_transmit = 1;
const int s_diffuse = 2;
int pbr_state = -1;

float GeometrySchlickGGX(float NdotV, float k)
{
  float nom = NdotV;
  float denom = NdotV * (1.0 - k) + k;
  return 1 / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float k)
{
  float NdotV = abs(dot(N, V));
  float NdotL = abs(dot(N, L));
  float ggx1 = NdotL * GeometrySchlickGGX(NdotV, k);
  float ggx2 = GeometrySchlickGGX(NdotL, k);

  return ggx1 * ggx2;
}

bool after_sample = false;

void pbr_ggx_eval(material_info_t material, vec3 towards_view, vec3 towards_light, vec3 normal, float ior_front, float ior_back, inout brdf_result_t result)
{
  float roughness = material.roughness * material.roughness;

  vec3 microfacet_normal = normalize((towards_light + towards_view));
  float D = (after_sample && roughness < 0.01) ? 1 : ggx_distribution(normal, microfacet_normal, roughness);
  float G = GeometrySmith(normal, towards_view, towards_light, roughness);// cxcook_torrance_geometry_1(towards_view, normal, microfacet_normal)* cxcook_torrance_geometry_1(towards_light, normal, microfacet_normal);

  // F is done by just the fresnel based random sampling
  float fresnel_cos_theta = dot(towards_view, microfacet_normal);
  float F = fresnel(fresnel_cos_theta, ior_front, ior_back, material.metallic);

  float den = 4 * abs(dot(towards_view, normal)) * abs(dot(towards_light, normal));

  float jacobian = 1 / (4 * abs(dot(towards_view, normal)));

  float cos_theta = (dot(microfacet_normal, normal));


  vec3 col = mix(vec3(1), material.color.rgb, sqrt(material.metallic));
  float mden = G / den;
  mden = isnan(mden) ? 1 : mden;
  result.pdf = F * D * cos_theta * mden;// D* (cos_theta)*jacobian; 
  result.reflectance = col * result.pdf;// abs(dot(towards_light, normal));// *D* G / den;
}
//
//vec3 ggx_pbr_transmit_resample(vec2 random_sample, material_info_t material, vec3 towards_view, vec3 normal, float ior_front, float ior_back)
//{
//  vec2 importance_sample = ggx_importance_sample(random_sample, material.roughness * material.roughness);
//  vec3 hemisphere_sample = sample_hemisphere(importance_sample);
//  vec3 microfacet_normal = transform_local_to_world(hemisphere_sample, normal);
//
//  return refract(-towards_view, microfacet_normal, ior_front / ior_back);
//}

void ggx_pbr_transmit_eval(material_info_t material, vec3 towards_view, vec3 towards_light, vec3 normal, float ior_front, float ior_back, inout brdf_result_t result)
{
  float roughness = material.roughness * material.roughness;
  vec3 microfacet_normal = normalize(-(ior_front * towards_view + ior_back * towards_light));
  //// The computation above yields the "outside" normal when going from inside to outside 
  //// but here we always need the one pointing towards the viewer.
  microfacet_normal *= sign(dot(microfacet_normal, normal));

  float D = (after_sample && roughness < 0.01) ? 1 : ggx_distribution(normal, microfacet_normal, roughness);
  float G = GeometrySmith(normal, towards_view, towards_light, roughness);// ggx_geometry(-towards_view, towards_light, normal, microfacet_normal, roughness);
  //// F is done by just the fresnel based random sampling

  float idoth = dot(towards_view, microfacet_normal);
  float odoth = dot(towards_light, microfacet_normal);
  float abs_idotn = abs(dot(towards_view, normal));
  float abs_odotn = abs(dot(towards_light, normal));

  float prefac = ((abs(idoth) * abs(odoth)) / (abs_idotn * abs_odotn));
  float den = (ior_front * idoth + ior_back * odoth);

  float fresnel_cos_theta = dot(towards_view, microfacet_normal);
  float F = fresnel(fresnel_cos_theta, ior_front, ior_back, material.metallic);

  float ior2_by_den2 = ior_back * ior_back / (den * den);
  float ft = prefac * (1 - F) * G * D * ior2_by_den2;
  float jacobian = ior2_by_den2 * abs(odoth);

  float cos_theta = (dot(microfacet_normal, normal));
  result.pdf = cos_theta * ft;
  result.reflectance = material.color.rgb * result.pdf;
}

float resample_factor = 1.0;

float ref = 1.0;
float trm = 1.0;
float dif = 1.0;

vec3 pbr_resample(vec2 random_sample, material_info_t material, vec3 towards_view, vec3 normal, float ior_front, float ior_back)
{
  float frand = random_value(int(19022 * random_sample.x) * 2909293);
  float trand = random_value(int(23452 * random_sample.y) * 8994355);

  vec2 importance_sample = ggx_importance_sample(random_sample, material.roughness * material.roughness);
  vec3 hemisphere_sample = sample_hemisphere(importance_sample);
  vec3 microfacet_normal = transform_local_to_world(hemisphere_sample, normal);

  float fresnel_cos_theta = dot(towards_view, microfacet_normal);
  float F = fresnel(fresnel_cos_theta, ior_front, ior_back, material.metallic);

  after_sample = true;
  vec3 refracted = refract(-towards_view, microfacet_normal, ior_front / ior_back);// ggx_pbr_transmit_resample(random_sample, material, towards_view, normal, ior_front, ior_back);
  if ((material.transmission > 0 && refracted == vec3(0)) || F >= frand)
  {
    pbr_state = s_reflect;
    resample_factor = F;
    return nreflect(towards_view, normal, microfacet_normal);//pbr_ggx_resample(random_sample, material, towards_view, normal, ior_front, ior_back);
  }
  else if(material.transmission >= trand)
  {
    resample_factor = (1-F) * material.transmission;
    pbr_state = s_transmit;
    return refracted;
  }
  else
  {
    resample_factor = (1 - F) * (1 - material.transmission);
    pbr_state = s_diffuse;
    material.roughness = 1.0;
    material.metallic = 1.0;
    importance_sample = ggx_importance_sample(random_sample, 1.0);
    hemisphere_sample = sample_hemisphere(importance_sample);
    microfacet_normal = transform_local_to_world(hemisphere_sample, normal);
    return nreflect(towards_view, normal, microfacet_normal);
  }
}

void pbr_eval(material_info_t material, vec3 towards_view, vec3 towards_light, vec3 normal, float ior_front, float ior_back, inout brdf_result_t result)
{
  brdf_result_t transmit;
  ggx_pbr_transmit_eval(material, towards_view, towards_light, normal, ior_front, ior_back, transmit);

  brdf_result_t gloss;
  pbr_ggx_eval(material, towards_view, towards_light, normal, ior_front, ior_back, gloss);

  brdf_result_t diffuse;
  material.roughness = 1.0;
  material.metallic = 1.0;
  pbr_ggx_eval(material, towards_view, towards_light, normal, ior_front, ior_back, diffuse);

  switch (pbr_state)
  {
  case s_transmit:
    result = transmit;
    break;
  case s_diffuse:
    result = diffuse;
    break;
  case s_reflect:
    result = gloss;
    break;
  }
  after_sample = false;

  result.pdf *= resample_factor;
  result.reflectance *= resample_factor;
}
#undef material_info_t