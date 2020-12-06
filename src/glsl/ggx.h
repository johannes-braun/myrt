#pragma once
const float pi = 3.1415926535897;

float ggx_chi(float v)
{
    return float(v > 0);
}

float ggx_distribution(vec3 n, vec3 h, float alpha)
{
    float NoH = dot(n,h);
    float alpha2 = alpha * alpha;
    float NoH2 = NoH * NoH;
    float den = NoH2 * alpha2 + (1 - NoH2);
    return (ggx_chi(NoH) * alpha2) / ( pi * den * den );
}

float ggx_partial_geometry(vec3 v, vec3 n, vec3 h, float alpha)
{
    float VoH2 = dot(v,h);
    float chi = ggx_chi( VoH2 / dot(v,n) );
    VoH2 = VoH2 * VoH2;
    float tan2 = ( 1 - VoH2 ) / VoH2;
    return (chi * 2) / ( 1 + sqrt( 1 + alpha * alpha * tan2 ) );
}

float ggx_geometry(const in vec3 view, const in vec3 outgoing, const in vec3 normal, const in vec3 facet_normal, float roughness)
{
  float geom_in = ggx_partial_geometry(-view, normal, facet_normal, roughness);
  float geom_out = ggx_partial_geometry(outgoing, normal, facet_normal, roughness);
  return geom_in * geom_out;
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