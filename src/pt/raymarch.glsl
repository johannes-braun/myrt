#pragma once

#define DONT_OPTIMIZE_ZERO min(sdf_marching_steps, 0)
struct mt_indices_t
{
  ivec4 indices;
  vec4 weights;
};

#define _MT mt_indices_t
float _G(int I) { return sdf_buffer[(I)]; }

bool g_do_load_materials = false;
vec3 g_point_normal = vec3(0, 1, 0);
vec3 g_point = vec3(0, 1, 0);

// "p" point being textured
// "n" surface normal at "p"
// "k" controls the sharpness of the blending in the transitions areas
// "s" texture sampler
//vec4 boxmap(in sampler2D s, in vec3 p, in vec3 n, in float k)
//{
//  // project+fetch
//  vec4 x = textureLod(s, p.yz, 0);
//  vec4 y = textureLod(s, p.zx, 0);
//  vec4 z = textureLod(s, p.xy, 0);
//
//  // blend factors
//  vec3 w = pow(abs(n), vec3(k));
//  // blend and return
//  return (x * w.x + y * w.y + z * w.z) / (w.x + w.y + w.z);
//}

_MT _ldm(int index) {
  _MT mat;
  
  if (g_do_load_materials) {
    mat.indices[0] = index;
    mat.weights[0] = 1;
  }

  return mat;
}
_MT _mxm(_MT a, _MT b, float t)
{
  _MT o;
  if (g_do_load_materials)
  {
    // TODO change...
    o = a;
  }
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

#ifdef MYRT_INJECT_SDF_MAP_CODE_HERE
MYRT_INJECT_SDF_MAP_CODE_HERE
#else
float map(vec3 p, _MT _mt) { return 1.0 / 0.0; }
#endif

float sdSphere(vec3 p, float s)
{
  return length(p) - s;
}

//
float opSmoothUnion(float d1, float d2, float k) {
  float h = clamp(0.5 + 0.5 * (d2 - d1) / k, 0.0, 1.0);
  return mix(d2, d1, h) - k * h * (1.0 - h);
}

float map(vec3 p, inout _MT material, bool writeMaterial)
{
   return map(p, material);
}

vec3 nor(vec3 p, inout _MT mat)
{
  _MT dum;
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

hit_t march_sdf(vec3 ro, vec3 rd, float tmax, bool hits_only, int steps, float eps)
{
  g_do_load_materials = false;
  drawable_sdf_t sdf = sdf_drawables[sdf_current];
  sdf_index_current = sdf.sdf_index;
  ro = (sdf.inverse_transformation * vec4(ro, 1)).xyz;
  rd = (((sdf.inverse_transformation) * vec4(rd, 0)).xyz);

  float res = -1;
  float t = 10 * eps;
  _MT material;
  for (int i = DONT_OPTIMIZE_ZERO; i < steps && t < tmax; i++)
  {
    float h = abs(map(ro + rd * t, material, true));
    if (h < eps)
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
  _MT current_mat;
  hit.normal = nor(hit.position, current_mat);
  hit.material_indices = current_mat.indices;
  hit.material_weights = current_mat.weights;
  return hit;
}