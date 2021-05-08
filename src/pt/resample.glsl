#pragma once

vec3 resample_cosine_hemisphere(vec2 uv)
{
  // (Uniformly) sample a point on the unit disk
  float r = sqrt(uv.x);
  float theta = 2 * 3.14159265359f * uv.y;
  float x = r * cos(theta);
  float y = r * sin(theta);

  // Project point up to the unit sphere
  float z = float(sqrt(max(0.f, 1 - x * x - y * y)));
  return vec3(x, y, z);
}