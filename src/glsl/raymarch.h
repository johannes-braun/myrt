#pragma once
#include <color.h>
#include <interface.h>

#define _MT material_info_t
float _G(int I) { return sdf_buffer[(I)]; }

bool g_do_load_materials = false;
vec3 g_point_normal = vec3(0, 1, 0);
vec3 g_point = vec3(0, 1, 0);

// "p" point being textured
// "n" surface normal at "p"
// "k" controls the sharpness of the blending in the transitions areas
// "s" texture sampler
vec4 boxmap(in sampler2D s, in vec3 p, in vec3 n, in float k)
{
  // project+fetch
  vec4 x = textureLod(s, p.yz, 0);
  vec4 y = textureLod(s, p.zx, 0);
  vec4 z = textureLod(s, p.xy, 0);

  // blend factors
  vec3 w = pow(abs(n), vec3(k));
  // blend and return
  return (x * w.x + y * w.y + z * w.z) / (w.x + w.y + w.z);
}

_MT _ldm(int index) {
  material_info_t mat;
  
  if (g_do_load_materials) {
    mat = materials[index];
    if(mat.has_albedo_texture != 0)
      mat.albedo_rgba_unorm = color_make(color_get(mat.albedo_rgba_unorm) * pow(boxmap(mat.albedo_texture, g_point, g_point_normal, 2.f), vec4(2.2)));
  }

  return mat;
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

// "p" point being textured
// "n" surface normal at "p"
// "k" controls the sharpness of the blending in the transitions areas
// "s" texture sampler
vec4 sample_texture(in sampler2D s, in vec3 p, in vec3 n, in float k)
{
  // project+fetch
  vec4 x = texture(s, p.yz);
  vec4 y = texture(s, p.zx);
  vec4 z = texture(s, p.xy);

  // blend factors
  vec3 w = pow(abs(n), vec3(k));
  // blend and return
  return (x * w.x + y * w.y + z * w.z) / (w.x + w.y + w.z);
}

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

vec3 nor(vec3 p, inout material_info_t mat)
{
  material_info_t dum;
  vec3 n = vec3(0.0);
  for (int i = DONT_OPTIMIZE_ZERO; i < 4; i++)
  {
    vec3 e = 0.5773 * (2.0 * vec3((((i + 3) >> 1) & 1), ((i >> 1) & 1), (i & 1)) - 1.0);
    n += e * map(p + 0.0005 * e, dum, true).x;
  }
  g_point = p;
  g_point_normal = normalize(n);
  g_do_load_materials = true;
  map(p, mat, true);
  return g_point_normal;
}

hit_t march_sdf(vec3 ro, vec3 rd, float tmax, bool hits_only)
{
  g_do_load_materials = false;
  drawable_sdf_t sdf = sdfs[sdf_current];
  sdf_index_current = sdf.sdf_index;
  ro = (sdf.inverse_transformation * vec4(ro, 1)).xyz;
  rd = (((sdf.inverse_transformation) * vec4(rd, 0)).xyz);

  float res = -1;
  float t = 10 * u_sdf_marching_epsilon;
  material_info_t material;
  for (int i = DONT_OPTIMIZE_ZERO; i < u_sdf_marching_steps && t < tmax; i++)
  {
    float h = abs(map(ro + rd * t, material, true));
    if (h < u_sdf_marching_epsilon)
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
  hit.normal = nor(hit.position, hit.material);
  return hit;
}