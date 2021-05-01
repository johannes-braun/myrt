#pragma once
#include <color.h>
#include <interface.h>

#define _MT material_info_t
#define _G(I) (buf[(I)])

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


float buf[25] = float[25](0.500000, 0.500000, 1.200000, 0.300000, 0.000000, 0.800000, 1.900000, 0.000000, 1.000000, 1.000000, 1.000000, 3.000000, 1.900000, 2.100000, 0.000000, 0.800000, 1.000000, 0.000000, 0.950000, 0.000000, 1.300000, 2.000000, 0.000000, -0.600000, 0.000000);
float _rfloat(float _B0) { return _B0; }float _X0(float _D0, float _D1, _MT _mt0, _MT _mt1, inout _MT _mt, float _P0) { float d1 = _D0; float d2 = _D1; float k = _P0; float h = clamp(0.5 + 0.5 * (d2 - d1) / k, 0.0, 1.0); _mt = _mxm(_mt0, _mt1, 1.0 - h); return mix(d2, d1, h) - k * h * (1.0 - h);; }float _X5(float _D0, float _D1, _MT _mt0, _MT _mt1, inout _MT _mt) { float m = min(_D0, _D1); _mt = _mxm(_mt0, _mt1, float(m == _D1)); return m;; }int _rint(float _B0) { return int(_B0); }float _X7(vec3 _L, inout _MT _mt, float _P0, float _P1, int _P2) { float rlarge = _P0; float rsmall = _P1; _mt = _ldm(_P2); vec3 p = _L; vec2 q = vec2(length(p.xz) - rlarge, p.y); return length(q) - rsmall;; }vec3 _rvec3(float _B0, float _B1, float _B2) { return vec3(_B0, _B1, _B2); }vec3 _Xc(vec3 _L, inout float _M, vec3 _P0) { return _L - _P0;; }float _Xf(vec3 _L, inout _MT _mt, vec3 _P0, int _P1) { vec3 p = _L; _mt = _ldm(_P1); for (int n = 0; n < 4; n++) { p = abs(p); if (p.x < p.y)p.xy = p.yx; if (p.x < p.z)p.xz = p.zx; if (p.y < p.z)p.zy = p.yz; p.z -= 1. / 3.; p.z = -abs(p.z); p.z += 1. / 3.; p *= 3.; p.x -= 2.; p.y -= 2.; }vec3 d = abs(p) - _P0; float dis = min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0)); dis *= pow(3., float(-4)); return dis;; }float _X15(vec3 _L, inout _MT _mt, float _P0, int _P1) { _mt = _ldm(_P1); return length(_L) - _P0;; }float map(vec3 _L, inout _MT _mt) { float _bk0 = _G(0); float par2 = _rfloat(_bk0); float _bk1 = _G(1); float par4 = _rfloat(_bk1); float _bk2 = _G(2); float par9 = _rfloat(_bk2); float _bk3 = _G(3); float para = _rfloat(_bk3); float _bk4 = _G(4); int parb = _rint(_bk4); float jmd = 1.0; float _bk5 = _G(5); float _bk6 = _G(6); float _bk7 = _G(7); vec3 pare = _rvec3(_bk5, _bk6, _bk7); vec3 md = _Xc(_L, jmd, pare); _MT _mtp8 = _mt; float p8 = _X7(md, _mtp8, par9, para, parb); float _bk8 = _G(8); float _bk9 = _G(9); float _bk10 = _G(10); vec3 par11 = _rvec3(_bk8, _bk9, _bk10); float _bk11 = _G(11); int par12 = _rint(_bk11); float jm13 = 1.0; float _bk12 = _G(12); float _bk13 = _G(13); float _bk14 = _G(14); vec3 par14 = _rvec3(_bk12, _bk13, _bk14); vec3 m13 = _Xc(_L, jm13, par14); _MT _mtp10 = _mt; float p10 = _Xf(m13, _mtp10, par11, par12); _MT _mto6 = _mt; float o6 = _X5(p8, p10, _mtp8, _mtp10, _mto6); float _bk15 = _G(15); float par17 = _rfloat(_bk15); float _bk16 = _G(16); int par18 = _rint(_bk16); float jm19 = 1.0; float _bk17 = _G(17); float _bk18 = _G(18); float _bk19 = _G(19); vec3 par1a = _rvec3(_bk17, _bk18, _bk19); vec3 m19 = _Xc(_L, jm19, par1a); _MT _mtp16 = _mt; float p16 = _X15(m19, _mtp16, par17, par18); _MT _mto3 = _mt; float o3 = _X0(o6, p16, _mto6, _mtp16, _mto3, par4); float _bk20 = _G(20); float par1c = _rfloat(_bk20); float _bk21 = _G(21); int par1d = _rint(_bk21); float jm1e = 1.0; float _bk22 = _G(22); float _bk23 = _G(23); float _bk24 = _G(24); vec3 par1f = _rvec3(_bk22, _bk23, _bk24); vec3 m1e = _Xc(_L, jm1e, par1f); _MT _mtp1b = _mt; float p1b = _X15(m1e, _mtp1b, par1c, par1d); _MT _mto1 = _mt; float o1 = _X0(o3, p1b, _mto3, _mtp1b, _mto1, par2); _mt = _mto1; return 1.0 * jmd * jm13 * jm19 * jm1e * o1; }




//
//float buf[5] = float[5](1.000000, 1.000000, 0.000000, 0.000000, 0.000000);
//float _rfloat(float _B0)
//{
//  return _B0;
//}
//
//int _rint(float _B0)
//{
//  return int(_B0);
//}
//
//float _X0(vec3 _L, inout _MT _mt, float _P0, int _P1)
//{
//  _mt = _ldm(_P1);
//  return length(_L) - _P0;;
//}
//
//vec3 _rvec3(float _B0, float _B1, float _B2)
//{
//  return vec3(_B0, _B1, _B2);
//}
//
//vec3 _X4(vec3 _L, out float _M, vec3 _P0)
//{
//  return _L - _P0;;
//}
//
//float map(vec3 _L, inout _MT _mt)
//{
//  float _bk0 = _G(0);
//  float par2 = _rfloat(_bk0);
//  float _bk1 = _G(1);
//  int par3 = _rint(_bk1);
//  float jm5 = 1.0;
//  float _bk2 = _G(2);
//  float _bk3 = _G(3);
//  float _bk4 = _G(4);
//  vec3 par6 = _rvec3(_bk2, _bk3, _bk4);
//  vec3 m5 = _X4(_L, jm5, par6);
//  _MT _mtp1 = _mt;
//  float p1 = _X0(m5, _mtp1, par2, par3);
//  _mt = _mtp1;
//  return 1.0 * p1;
//}

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
  float res = -1;
  float t = 1e-3f;
  material_info_t material;
  for (int i = DONT_OPTIMIZE_ZERO; i < 128 && t < tmax; i++)
  {
    float h = abs(map(ro + rd * t, material, true));
    if (h < 1e-5f)
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