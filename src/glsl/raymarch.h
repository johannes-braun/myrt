#pragma once
#include <color.h>
#include <interface.h>

#define _MT material_info_t
float _G(int I) { return sdf_buffer[(I)]; }

_MT _ldm(int index) {
  return materials[index];
}
_MT _mxm(_MT a, _MT b, float t)
{
  _MT o;
#define cppar(par) o.par = mix(a.par, b.par, t)

  cppar(roughness);
  o.albedo_rgba_unorm = color_make(mix(color_get(a.albedo_rgba_unorm), color_get(b.albedo_rgba_unorm), t));
  cppar(ior);
  cppar(metallic);
  cppar(transmission);

#undef cppar
  return o;
}

int sdf_current = 1;
int sdf_index_current = 0;


MYRT_INJECT_SDF_MAP_CODE_HERE

float sdSphere(vec3 p, float s)
{
  return length(p) - s;
}

//
float opSmoothUnion(float d1, float d2, float k) {
  float h = clamp(0.5 + 0.5 * (d2 - d1) / k, 0.0, 1.0);
  return mix(d2, d1, h) - k * h * (1.0 - h);
}

float map(vec3 p, inout material_info_t material, bool writeMaterial)
{
   return map(p, material);
  float sphere0 = sdSphere(p - vec3(0, 0.7, 0), 1.5);
  if (writeMaterial)
  {
    material.roughness = step(sin(p.y * 5), 0.0001) * 0.3 + 0.2;
    material.albedo_rgba_unorm = color_make(mix(vec4(1, 1, 0, 1), vec4(0, 0, 1, 1), material.roughness));
    material.ior = 1.5;
    material.metallic = step(-sphere0, 0.0001);
    material.transmission = 0;
  }
  return opSmoothUnion(sphere0, sdSphere(p - vec3(0, 3, 0), 0.5), 1.0);
}

vec3 nor(vec3 p)
{
  material_info_t dummy_material;
  vec3 n = vec3(0.0);
  for (int i = DONT_OPTIMIZE_ZERO; i < 4; i++)
  {
    vec3 e = 0.5773 * (2.0 * vec3((((i + 3) >> 1) & 1), ((i >> 1) & 1), (i & 1)) - 1.0);
    n += e * map(p + 0.0005 * e, dummy_material, true).x;
  }
  return normalize(n);
}

hit_t march_sdf(vec3 ro, vec3 rd, float tmax, bool hits_only)
{
  drawable_sdf_t sdf = sdfs[sdf_current];
  sdf_index_current = sdf.sdf_index;
  ro = (sdf.inverse_transformation * vec4(ro, 1)).xyz;
  rd = (((sdf.inverse_transformation) * vec4(rd, 0)).xyz);

  float res = -1;
  float t = 1e-3f;
  material_info_t material;
  for (int i = DONT_OPTIMIZE_ZERO; i < 128 && t < tmax; i++)
  {
    float h = abs(map(ro + rd * t, material, true));
    if (h < 1e-6f)
    {
      res = t;
      break;
    }
    t += h;
  }

  if (res < 0)
  {
    hit_t h;
    h.hits = false;
    return h;
  }

  hit_t hit;
  hit.hits = true;

  if (hits_only)
    return hit;

  hit.t = res;
  hit.position = ro + t * rd;
  hit.normal = nor(hit.position);
  hit.material = material;
  return hit;
}