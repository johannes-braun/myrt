#pragma once

struct bokeh_t
{
  vec3 value;
  float amount;
  vec2 offset;
};

float bokeh_amount(vec3 bokeh_value)
{
  return max(max(bokeh_value.x, bokeh_value.y), bokeh_value.z);
}

bokeh_t bokeh_get(sampler2D mask)
{
  int bokeh_tries = 8;
  bokeh_t bokeh;
  bokeh.offset = random_uniform_circle(0.49f);
  bokeh.value = vec3(1, 1, 1);
  bokeh.amount = 1;

  if (textureSize(mask, 0).x < 10)
    return bokeh;

  while ((bokeh_tries == 8 || bokeh.amount < 1e-5) && bokeh_tries-- > 0)
  {
    bokeh.value = textureLod(mask, vec2(0.5) + bokeh.offset, 0).rgb;
    bokeh.offset = random_uniform_circle(0.49f);
    bokeh.amount = bokeh_amount(bokeh.value);
  }

  return bokeh;
}