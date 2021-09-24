#version 460 core
#extension GL_ARB_bindless_texture : require

layout(location = 0) in vec3 normal;
layout(location = 1) in vec3 position;
layout(location = 2) in vec2 uv;
layout(location = 3) flat in int material;
layout(location = 4) flat in vec3 cam_pos;

layout(location = 0) out vec4 color;

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

struct light_t
{
  vec3 position;
  float radius;
  vec3 color;
  vec2 attenuation;
} test_lights[1];

vec3 tonemapFilmic(vec3 x) {
  vec3 X = max(vec3(0.0), x - 0.004);
  vec3 result = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
  return pow(result, vec3(1.3));
}

vec3 tonemap(vec3 col)
{
  return tonemapFilmic(col);
}

void main(){
  vec3 ref = vec3(0);
  float pdf = 1;
  
  test_lights[0].position = 2*vec3(4, 5, 4);
  test_lights[0].color = 6*vec3(1, 1.8, 3);
  test_lights[0].radius = 1.5f;
  
  vec3 to_cam = normalize(cam_pos - position);
  vec3 nrom = normalize(normal);
  vec3 path_to_light = test_lights[0].position - position;
  vec3 direction_to_light = normalize(path_to_light);
  float distance_to_light = length(path_to_light);
  vec3 lnorm = nrom;
    material_load(material);
    
  vec3 refgb = vec3(0);
  vec3 bg = mix(vec3(0.3, 0.4, 0.5), vec3(1.0, 0.98, 0.96), (nrom.y + 1) / 2);
  pbr_state = s_diffuse;
  material_sample(position, uv, nrom, nrom, to_cam, refgb, pdf);
  bg *= refgb / pdf;
  
 pbr_state = s_diffuse;
 material_sample(position, uv, lnorm, direction_to_light, to_cam, ref, pdf);
  ref = ref / pdf * max(0, dot(lnorm, direction_to_light));
  ref *= test_lights[0].color / (1+distance_to_light*distance_to_light);
   
 if(dot((direction_to_light + to_cam) / 2, lnorm) > 0)
 {
   vec3 ref2 = vec3(0);
   pbr_state = s_reflect;
   material_sample(position, uv, lnorm, direction_to_light, to_cam, ref2, pdf);
 
   ref = mix(ref, ref2 * test_lights[0].color, 0.5);
 }

  color = vec4(tonemap(ref + bg), 1);
}