#pragma once

vec3 transform_local_to_world(vec3 vector, vec3 normal)
{
  // Find an axis that is not parallel to normal
  vec3 perp = vec3(1, 0, 0);
  vec3 u = normalize(cross(normal, perp));
  for (int i = 1; any(isnan(u)) && i < 3; ++i)
  {
    perp[i - 1] = 0;
    perp[i] = 1;
    u = normalize(cross(normal, perp));
  }
  return normalize(mat3(u, cross(normal, u), normal) * vector);
}