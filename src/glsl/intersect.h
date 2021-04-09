#pragma once

bool intersect_triangle(const vec3 origin, const vec3 direction, const vec3 v1, const vec3 v2, const vec3 v3, inout float t, inout vec2 barycentric)
{
    float float_epsilon = 1e-23f;
    float border_epsilon = 1e-6f;
    vec3 e1 = v2 - v1;
    vec3 e2 = v3 - v1;
    vec3 P = cross(vec3(direction), e2);
    float det = dot(e1, P);
    if (det > -float_epsilon && det < float_epsilon)
        return false;
    float inv_det = 1.0 / det;
    vec3 T = origin.xyz - v1;
    barycentric.x = dot(T, P) * inv_det;
    if (barycentric.x < -border_epsilon || barycentric.x > 1.0 + border_epsilon)
        return false;
    vec3 Q = cross(T, e1);
    barycentric.y = dot(vec3(direction), Q) * inv_det;
    if (barycentric.y < -border_epsilon || barycentric.x + barycentric.y > 1.0 + border_epsilon)
        return false;
    float tt = dot(e2, Q) * inv_det;
    if (tt > float_epsilon) {
        t = tt;
        return true;
    }
    return false;
}

float raySphereIntersect(vec3 r0, vec3 rd, vec3 s0, float sr) {
  float a = dot(rd, rd);
  vec3 s0_r0 = r0 - s0;
  float b = 2.0 * dot(rd, s0_r0);
  float c = dot(s0_r0, s0_r0) - (sr * sr);
  if (b * b - 4.0 * a * c < 0.0) {
    return -1.0;
  }
  return (-b - sqrt((b * b) - 4.0 * a * c)) / (2.0 * a);
}
