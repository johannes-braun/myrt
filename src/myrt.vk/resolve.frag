#version 460 core

#extension GL_EXT_debug_printf : enable

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

void main()
{
mat4 vmat = mat4(1,0,0,0,0,-1,0,0,0,0,1,0,0,0,0,1) * in_constants.view_matrix;

  ivec2 pixel = ivec2(gl_FragCoord.xy);
  ivec2 gbuffersize = ivec2(textureSize(in_attachment1, 0));
  vec4 packed_position_uvx = texelFetch(in_attachment0, pixel, 0);
  vec4 packed_normal_uvy = texelFetch(in_attachment1, pixel, 0);

  bool is_bg = packed_normal_uvy.xyz == vec3(0,0,0);

  if(is_bg)
  {
    vec4 ray_direction_hom = (inverse(in_constants.projection_matrix * in_constants.view_matrix) * vec4((vec2(pixel) / vec2(gbuffersize)) * 2 - 1, 1, 1));
    vec3 ray_direction = normalize(ray_direction_hom.xyz);
    color.rgb = background(ray_direction);
    color.a = 1;
    return;
  }

  vec3 normal = normalize(packed_normal_uvy.xyz);
  vec3 position = normalize(packed_position_uvx.xyz);
  vec2 uv = vec2(packed_position_uvx.w, packed_normal_uvy.w);
  
//  random_init_nt(pixel, int(412421213 + 9897584*uint(position.x) ^uint(position.y) ^uint(position.z)));
//
//  float freeness = 0;
//  float ssao_distance = 0.1;
//
//  const float ssao_fac = 0.2;
//  int samplesxy = 1;
//  int samplesd = 8;
//
//  for(int i=0; i<samplesd; ++i)
//  {
//    for(int x = 0; x < samplesxy; ++x)
//  {
//    vec2 hs = 2*random_next_2d() - 1;
//    vec3 hemi = sample_hemisphere(hs);
//
//    vec3 world = normalize(transform_local_to_world(hemi, normal));
//
//    vec3 test_point = position + world * ssao_distance;
//
//    vec4 hom = in_constants.projection_matrix * in_constants.view_matrix * vec4(test_point, 1);
////    
////    if(pixel.x == 640 && pixel.y == 600)
////    {
////    vec4 homXX = in_constants.projection_matrix * vmat * vec4(position, 1);
////      debugPrintfEXT("hom: %v4f\n\n", homXX);
////    }
////    color.rgb = hom.xyz / hom.w;
////    color.w = 1;
////    return;
//
//    if(hom.w == 0)
//    {
//      freeness+=1;
//      continue;
//    }
//
//    vec2 px = ((hom.xy / hom.w) + 1) / 2;
//    px.y = 1-px.y;
//    px *= gbuffersize;
//    px = clamp(px, vec2(0), gbuffersize - 1);
//
//    vec4 other_norm = texelFetch(in_attachment1, ivec2(px), 0);
//
//
//    if(other_norm.xyz == vec3(0))
//    {
//      freeness+=1;
//      continue;
//    }
//        
//    vec4 other_pos = texelFetch(in_attachment0, ivec2(px), 0);
//
//
//    vec3 to_one = cam_pos - test_point;
//    vec3 to_other = cam_pos - other_pos.xyz;
//
//    float diff = length(to_other) - length(to_one);
//    float s = sign(diff);
//    diff = diff;
//
//    if(diff > 0)
//    {
//      freeness += 1;
//      continue;
//    }
//
//    freeness += float(diff < -ssao_fac);
//  }
//    ssao_distance /= 2;
//  }
//  freeness /= float(samplesxy) * samplesd;
//
//  

  color = vec4((vec3(0.7, 0.8,0.4) * background(normal) + vec3(0.7, 0.8,0.4) * max(0.0, dot(normal, normalize(vec3(1,1,1))))), 1);
  color.rgb = tonemap(color.rgb);

//  color.rgb = vec3(freeness);

  color.w = 1;
}



void texcoords(vec2 fragCoord, vec2 resolution,
			out vec2 v_rgbNW, out vec2 v_rgbNE,
			out vec2 v_rgbSW, out vec2 v_rgbSE,
			out vec2 v_rgbM) {
	vec2 inverseVP = 1.0 / resolution.xy;
	v_rgbNW = (fragCoord + vec2(-1.0, -1.0)) * inverseVP;
	v_rgbNE = (fragCoord + vec2(1.0, -1.0)) * inverseVP;
	v_rgbSW = (fragCoord + vec2(-1.0, 1.0)) * inverseVP;
	v_rgbSE = (fragCoord + vec2(1.0, 1.0)) * inverseVP;
	v_rgbM = vec2(fragCoord * inverseVP);
}

#ifndef FXAA_REDUCE_MIN
    #define FXAA_REDUCE_MIN   (1.0/ 128.0)
#endif
#ifndef FXAA_REDUCE_MUL
    #define FXAA_REDUCE_MUL   (1.0 / 8.0)
#endif
#ifndef FXAA_SPAN_MAX
    #define FXAA_SPAN_MAX     8.0
#endif

//optimized version for mobile, where dependent 
//texture reads can be a bottleneck
vec4 fxaa(sampler2D tex, vec2 fragCoord, vec2 resolution,
            vec2 v_rgbNW, vec2 v_rgbNE, 
            vec2 v_rgbSW, vec2 v_rgbSE, 
            vec2 v_rgbM) {
    vec4 color;
    vec2 inverseVP = vec2(1.0 / resolution.x, 1.0 / resolution.y);
    vec3 rgbNW = texture(tex, v_rgbNW).xyz;
    vec3 rgbNE = texture(tex, v_rgbNE).xyz;
    vec3 rgbSW = texture(tex, v_rgbSW).xyz;
    vec3 rgbSE = texture(tex, v_rgbSE).xyz;
    vec4 texColor = texture(tex, v_rgbM);
    vec3 rgbM  = texColor.xyz;
    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    
    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) *
                          (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
    
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX),
              max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
              dir * rcpDirMin)) * inverseVP;
    
    vec3 rgbA = 0.5 * (
        texture(tex, fragCoord * inverseVP + dir * (1.0 / 3.0 - 0.5)).xyz +
        texture(tex, fragCoord * inverseVP + dir * (2.0 / 3.0 - 0.5)).xyz);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(tex, fragCoord * inverseVP + dir * -0.5).xyz +
        texture(tex, fragCoord * inverseVP + dir * 0.5).xyz);

    float lumaB = dot(rgbB, luma);
    if ((lumaB < lumaMin) || (lumaB > lumaMax))
        color = vec4(rgbA, texColor.a);
    else
        color = vec4(rgbB, texColor.a);
    return color;
}

vec4 apply_fxaa(sampler2D tex, vec2 fragCoord, vec2 resolution) {
	vec2 v_rgbNW;
	vec2 v_rgbNE;
	vec2 v_rgbSW;
	vec2 v_rgbSE;
	vec2 v_rgbM;

	//compute the texture coords
	texcoords(fragCoord, resolution, v_rgbNW, v_rgbNE, v_rgbSW, v_rgbSE, v_rgbM);
	
	//compute FXAA
	return fxaa(tex, fragCoord, resolution, v_rgbNW, v_rgbNE, v_rgbSW, v_rgbSE, v_rgbM);
}