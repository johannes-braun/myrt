#version 460 core

#extension GL_EXT_shader_realtime_clock : enable

#include "../pt/random.glsl"
#include "../pt/ggx.glsl"
#include "../pt/transform.glsl"
#include "../pt/sky.glsl"

layout(location = 0) flat in vec3 cam_pos;

layout(push_constant) uniform constants {
  mat4 view_matrix;
  mat4 projection_matrix;
} in_constants;

layout(set = 0, binding = 0) uniform sampler2D in_attachment0;
layout(set = 0, binding = 1) uniform sampler2D in_attachment1;
layout(set = 0, binding = 2) uniform sampler2D in_attachment2;
layout(set = 0, binding = 3) uniform sampler2DShadow in_depth_attachment;

layout(location = 0) out vec4 color;

vec3 tonemapFilmic(vec3 x) {
  vec3 X = max(vec3(0.0), x - 0.004);
  vec3 result = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
  return pow(result, vec3(1.3));
}

vec3 tonemap(vec3 col)
{
  return tonemapFilmic(col);
}
vec4 apply_fxaa(sampler2D tex, vec2 fragCoord, vec2 resolution); 

const float noising = 0.01;
const float fog_min = 70;
const float fog_max = 100;

struct ssao_settings_t{
  float max_distance;
  float hemisphere_samples;
  float distance_samples;
} ;
const ssao_settings_t ssao_settings = {
  0.1,
  2,
  3
};

float ssao(sampler2DShadow depth_map, vec3 world_position, vec3 world_normal, mat4 view_projection)
{
  float freeness = 0;
  float ssao_distance = ssao_settings.max_distance;

  for(int i=0; i<ssao_settings.distance_samples; ++i)
  {
    for(int y = 0; y < ssao_settings.hemisphere_samples; ++y)
    {
      for(int x = 0; x < ssao_settings.hemisphere_samples; ++x)
      {
        vec2 hs = vec2(x/float(ssao_settings.hemisphere_samples-1), y/float(ssao_settings.hemisphere_samples-1));
        vec3 hemi = sample_hemisphere(hs);
        vec3 world = normalize(transform_local_to_world(hemi, world_normal));
        vec3 test_point = world_position + world * ssao_distance;
        vec4 hom = view_projection * vec4(test_point, 1);

        if(hom.w == 0)
        {
          freeness+=1;
          continue;
        }

        vec2 px = ((hom.xy / hom.w) + 1) / 2;

        float dd = hom.z / hom.w;

        freeness += texture(depth_map, vec3(px, dd));
      }
    }
    ssao_distance = max(0.01, ssao_distance/2);
  }
  freeness /= float(ssao_settings.hemisphere_samples * ssao_settings.hemisphere_samples) * ssao_settings.distance_samples;
  return freeness;
}

void main()
{
  mat4 vmat = mat4(1,0,0,0,0,-1,0,0,0,0,1,0,0,0,0,1) * in_constants.view_matrix;

  ivec2 pixel = ivec2(gl_FragCoord.xy);
  ivec2 gbuffersize = ivec2(textureSize(in_attachment1, 0));
  vec4 packed_position_uv = texelFetch(in_attachment0, pixel, 0);
  vec4 packed_normal_id = texelFetch(in_attachment1, pixel, 0);
  vec4 attachment2 = texelFetch(in_attachment2, pixel, 0);

  bool is_bg = packed_normal_id.xyz == vec3(0,0,0);
  random_init_nt(pixel, int(411113 * clockRealtime2x32EXT().x));

  mat4 mat_mul = in_constants.projection_matrix * in_constants.view_matrix;
  mat4 mat_mul2 = in_constants.projection_matrix * mat4(mat3(in_constants.view_matrix));

  float fac = 0.3;
  float po = (clockRealtime2x32EXT().x / float(~0u));
  float pe = mod((clockRealtime2x32EXT().y % 1000 + po) * fac, 1.0);

  time = 1;//pe * 2 * 3.1415926535;

  vec4 ray_direction_hom = (inverse(mat_mul2) * vec4((vec2(pixel) / vec2(gbuffersize)) * 2 - 1, 1, 1));
  vec3 ray_direction = normalize(ray_direction_hom.xyz);
  vec3 sundir;
  color.rgb = sky_noclouds(ray_direction, cam_pos, sundir).xyz;
  color.rgb += noising * vec3(random_next(),random_next(),random_next());
  
  if(is_bg)
  {
    color.a = 1;
    return;
  }

  vec3 normal = normalize(packed_normal_id.xyz);
  normal = faceforward(normal, ray_direction, normal);

  vec3 position = packed_position_uv.xyz;
  vec2 uv = unpackUnorm2x16(floatBitsToUint(packed_position_uv.w));

  float freeness = ssao(in_depth_attachment, position, normal, mat_mul);
  freeness = 1 - (1-freeness) * random_next();
  
  vec3 diff = vec3(0.7, 0.8,0.4);

  uint id = floatBitsToUint(packed_normal_id.w);

  if(id % 2 == 0)
    diff = vec3(0.15, 0, 0);
  if(id % 5 == 0)
    diff = vec3(0, 0.3, 0);

  vec3 m = normalize(sundir + -ray_direction);

  vec3 avg = sky_noclouds(reflect(ray_direction, normal), cam_pos, sundir);
  vec4 objects = vec4(freeness * (diff * avg + diff * max(0.0, dot(normal, sundir)) + pow(max(0.0, dot(normal, m)), 200)), 1);
  objects.rgb = tonemap(objects.rgb);

  objects.rgb += noising * vec3(random_next(),random_next(),random_next());
  objects.w = 1;
  
  float dist = distance(position, cam_pos);
  color = mix(objects, color, smoothstep(fog_min, fog_max, dist));
}
