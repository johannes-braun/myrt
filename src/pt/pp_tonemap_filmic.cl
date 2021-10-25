float3 tonemap_filmic_impl(float3 x) {
  float3 X = max((float3)(0.0f), x - 0.004f);
  float3 result = (X * (6.2f * X + 0.5f)) / (X * (6.2f * X + 1.7f) + 0.06f);
  return pow(result, (float3)(1.3f));
}

kernel void tonemap_filmic(__read_only image2d_t input, __write_only image2d_t output, int width, int height) {
  const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
  int2 coord = (int2)(get_global_id(0), get_global_id(1));

  if (coord.x >= width || coord.y >= height)
    return;

  float4 in_color = read_imagef(input, sampler, coord);
  write_imagef(output, coord, (float4)(tonemap_filmic_impl(in_color.xyz), 1.f));
}