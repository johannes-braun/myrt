#pragma once

struct color_rgba_t
{
  uint value;
};

vec4 color_get(color_rgba_t color)
{
  return unpackUnorm4x8(color.value);
}

color_rgba_t color_make(vec4 color)
{
  color_rgba_t result;
  result.value = packUnorm4x8(color);
  return result;
}

color_rgba_t color_mix(color_rgba_t first, color_rgba_t second, float t)
{
  return color_make(mix(color_get(first), color_get(second), t));
}