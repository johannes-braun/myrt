#version 460 core

#extension GL_EXT_debug_printf : enable
#extension GL_EXT_shader_realtime_clock : enable

#include "../pt/random.glsl"
#include "../pt/ggx.glsl"
#include "../pt/transform.glsl"

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

const float noising = 0.12;

void main()
{
mat4 vmat = mat4(1,0,0,0,0,-1,0,0,0,0,1,0,0,0,0,1) * in_constants.view_matrix;

  ivec2 pixel = ivec2(gl_FragCoord.xy);
  ivec2 gbuffersize = ivec2(textureSize(in_attachment1, 0));
  vec4 packed_position_uvx = texelFetch(in_attachment0, pixel, 0);
  vec4 packed_normal_uvy = texelFetch(in_attachment1, pixel, 0);
  vec4 attachment2 = texelFetch(in_attachment2, pixel, 0);

  bool is_bg = packed_normal_uvy.xyz == vec3(0,0,0);
  random_init_nt(pixel, int(411113 * clockRealtime2x32EXT().x));

  mat4 mat_mul = in_constants.projection_matrix * in_constants.view_matrix;
  if(is_bg)
  {
    vec4 ray_direction_hom = (inverse(mat_mul) * vec4((vec2(pixel) / vec2(gbuffersize)) * 2 - 1, 1, 1));
    vec3 ray_direction = normalize(ray_direction_hom.xyz);
    color.rgb = background(ray_direction);
    color.rgb += noising * vec3(random_next(),random_next(),random_next());
    color.a = 1;
    return;
  }

  vec3 normal = normalize(packed_normal_uvy.xyz);
  vec3 position = packed_position_uvx.xyz;
  vec2 uv = vec2(packed_position_uvx.w, packed_normal_uvy.w);
  

  float freeness = 0;
  float ssao_distance = 0.1;
  int hemisphere_samples = 2;
  int distance_samples = 4;

  for(int i=0; i<distance_samples; ++i)
  {
    for(int y = 0; y < hemisphere_samples; ++y)
    {
      for(int x = 0; x < hemisphere_samples; ++x)
      {
        vec2 hs = vec2(x/float(hemisphere_samples-1), y/float(hemisphere_samples-1)) * 2 - 1;
        vec3 hemi = sample_hemisphere(hs);
        vec3 world = normalize(transform_local_to_world(hemi, normal));
        vec3 test_point = position + world * ssao_distance;
        vec4 hom = mat_mul * vec4(test_point, 1);

        if(hom.w == 0)
        {
          freeness+=1;
          continue;
        }

        vec2 px = ((hom.xy / hom.w) + 1) / 2;

        float dd = hom.z / hom.w;

        freeness += texture(in_depth_attachment, vec3(px, dd));
      }
    }
    ssao_distance /= 2;
  }
  freeness /= float(hemisphere_samples * hemisphere_samples) * distance_samples;
  freeness = 1 - (1-freeness) * random_next();
  
  vec3 diff = vec3(0.7, 0.8,0.4);

  uint id = uint(attachment2.x) | (uint(attachment2.y)<<12);

  if(id % 2 == 0)
  {
    diff = vec3(0.67, 0, 0);
  }

  color = vec4(freeness * (diff * background(normal) + diff * max(0.0, dot(normal, normalize(vec3(1,1,1))))), 1);
  color.rgb =  tonemap(color.rgb);

  color.rgb += noising * vec3(random_next(),random_next(),random_next());

  color.w = 1;
}
