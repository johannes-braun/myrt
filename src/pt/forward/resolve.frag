#version 430 core
#extension GL_ARB_bindless_texture : enable

layout(location = 0) flat in vec3 cam_pos;
layout(binding = 0) uniform sampler2DArray gbuffer;
layout(location = 0) out vec4 color;

uniform mat4 view;
uniform mat4 proj;
uniform uint random_seed;

struct material_reference_t
{
  int id;
  int offset;
};
layout(binding = 1, std430) restrict readonly buffer MaterialsBuffer { material_reference_t materials[]; };
layout(binding = 2, std430) restrict readonly buffer MaterialsDataBuffer { float materials_data[]; };

#include "../transform.glsl"
#include "../random.glsl"

#define _G(P) materials_data[P]
#define _MTY(I) materials[I].id
#define _MBO(I) materials[I].offset
#include "../material.glsl"
#undef _G
#undef _MTY
#undef _MBO

#include "../ggx.glsl"

vec3 tonemapFilmic(vec3 x) {
  vec3 X = max(vec3(0.0), x - 0.004);
  vec3 result = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
  return pow(result, vec3(1.3));
}

vec3 tonemap(vec3 col)
{
  return tonemapFilmic(col);
}

vec3 background(vec3 dir)
{
  return mix(vec3(0.3, 0.4, 0.5), vec3(1.0, 0.98, 0.96), (dir.y + 1) / 2);
}

float rescale(float val, float smin, float smax, float dmin, float dmax)
{
  float new_scale = dmax - dmin;
  float old_scale = smax - smin;

  val -= smin;
  val /= old_scale;
  val *= new_scale;
  val += dmin;
  return val;
}

void main()
{
  ivec2 pixel = ivec2(gl_FragCoord.xy);

  vec4 gbuf_position = texelFetch(gbuffer, ivec3(pixel, 0), 0);
  vec4 gbuf_uv = texelFetch(gbuffer, ivec3(pixel, 1), 0);
  vec4 gbuf_norm_mat = texelFetch(gbuffer, ivec3(pixel, 2), 0);
  vec2 gbuffersize = textureSize(gbuffer, 0).xy;

  bool is_bg = gbuf_norm_mat == vec4(0,0,0,0);

  if(is_bg)
  {
    vec4 ray_direction_hom = (inverse(view) * inverse(proj) * vec4((vec2(pixel) / vec2(gbuffersize)) * 2 - 1, 1, 1));
    vec3 ray_direction = normalize(ray_direction_hom.xyz);
    color.rgb = background(ray_direction);
    color.a = 1;
    return;
  }
  vec3 to_cam = normalize(cam_pos - gbuf_position.xyz);
  vec3 ld = normalize(vec3(10, 10, 20));

  vec3 normal = normalize(gbuf_norm_mat.xyz);
  normal = faceforward(normal, -to_cam, normal);
  vec3 position = gbuf_position.xyz;
  vec2 uv = gbuf_uv.xy;
  int material = int(gbuf_norm_mat.w);

  material_load(material);
  pbr_state = s_diffuse;
  vec3 ref = vec3(0);
  float pdf = 1;
  material_sample(position, uv, normal, ld, to_cam, ref, pdf);


  vec3 refgb = vec3(0);
  pbr_state = s_diffuse;
  material_sample(position, uv, normal, normal, to_cam, refgb, pdf);

  float freeness = 0;
  const float ssao_distance = 0.42;
  const float ssao_fac = 0.1;
  int samplesxy = 8;

  random_init_nt(pixel, int(random_seed + 9897584*uint(position.x) ^uint(position.y) ^uint(position.z)));

  for(int x = 0; x < samplesxy; ++x)
  {
    vec2 hs = 2*random_next_2d() - 1;
    vec3 hemi = sample_hemisphere(hs);
    vec3 world = normalize(transform_local_to_world(hemi, normal));

    vec3 test_point = position + world * ssao_distance;

    vec4 hom = proj * view * vec4(test_point, 1);
    vec2 px = ((hom.xy / hom.z) + 1) / 2;
    px *= gbuffersize;
    px = clamp(px, vec2(0), gbuffersize - 1);

    vec4 other_norm = texelFetch(gbuffer, ivec3(px, 2), 0);

    if(other_norm == vec4(0))
    {
      freeness+=1;
      continue;
    }
        
    vec4 other_pos = texelFetch(gbuffer, ivec3(px, 0), 0);
    vec3 to_one = cam_pos - test_point;
    vec3 to_other = cam_pos - other_pos.xyz;

    float diff = length(to_other) - length(to_one);
    float s = sign(diff);
    diff = diff;

    if(diff > 0)
    {
      freeness += 1;
      continue;
    }

    freeness += pow(max(smoothstep(diff, ssao_fac, -ssao_fac), smoothstep(diff, ssao_fac, -ssao_fac)), 15);
  }
  freeness /= float(samplesxy);

  freeness = pow(freeness, 1);

  vec4 color0 = vec4((refgb / pdf + ref / pdf * max(0, dot(normal, ld))), 1);
  vec4 color1 = freeness * color0;

  color = color1;
}