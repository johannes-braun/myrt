#pragma once
const float pi = 3.1415926535897;

float ggx_chi(float v)
{
    return float(v > 0);
}

float ggx_distribution(vec3 n, vec3 h, float alpha)
{
  float ndotm = dot(n, h);
  float a2 = alpha * alpha;
  float den = ndotm * ndotm * (a2 - 1) + 1;
  return ggx_chi(ndotm) * a2 / (pi * den * den);
}

float cook_torrance_geometry_1(vec3 v, vec3 n, vec3 h)
{
  return  2 * abs(dot(n, h)) * abs(dot(n, v) / dot(v, h));
}

float ggx_geometry(const in vec3 view, const in vec3 outgoing, const in vec3 normal, const in vec3 facet_normal, float roughness)
{
 /* return cook_torrance_geometry_1(-view, normal, facet_normal) *
    cook_torrance_geometry_1(outgoing, normal, facet_normal);*/

  return min(1, min(
    cook_torrance_geometry_1(-view, normal, facet_normal), 
    cook_torrance_geometry_1(outgoing, normal, facet_normal)));
}

vec2 ggx_importance_sample(const in vec2 random_sample, const in float roughness)
{
  float phi = random_sample.y * 2.f * pi;
  float rough2 = roughness*roughness;
  float theta = atan(sqrt(rough2 * random_sample.x / (1 - random_sample.x)));
  return vec2(phi, theta);
}

vec3 sample_hemisphere(const in vec2 importance_sample)
{
  float phi = importance_sample.x;
  float cosTheta = cos(importance_sample.y);
  float sinTheta = sin(importance_sample.y);
  return vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}