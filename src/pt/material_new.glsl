
#include "pbr.glsl"

struct material_state_t {
  pbr_matinfo_t mat;
  uvec2 albedo_texture;
  bool is_incoming;
  float ior1;
  float ior2;
};

material_state_t material_state;

vec3 material_normal(vec3 n) {
  return material_state.is_incoming ? n : -n;
}

void open()
{
  material_state.albedo_texture = this.albedo_texture;
  material_state.mat.color = this.albedo.value;
  material_state.mat.ior = this.ior;
  material_state.mat.roughness = this.roughness;
  material_state.mat.metallic = this.metallic;
  material_state.mat.transmission = this.transmission;
}

void evaluate(vec3 point, vec2 uv, vec3 normal, vec3 towards_light, vec3 towards_viewer, out vec3 reflectance, out float pdf) {
  sampler2D albedo_texture = sampler2D(material_state.albedo_texture);
  if (material_state.albedo_texture != uvec2(0))
    material_state.mat.color = pow(texture(albedo_texture, vec2(uv.x, 1 - uv.y)), vec4(2.2));
  material_state.is_incoming = dot(normal, -towards_viewer) < 0;
  material_state.ior1 = material_state.is_incoming ? 1.0 : material_state.mat.ior;
  material_state.ior2 = material_state.is_incoming ? material_state.mat.ior : 1.0;

  brdf_result_t ev;
  pbr_eval(material_state.mat, towards_viewer, towards_light, material_normal(normal), material_state.ior1,
      material_state.ior2, ev);
  reflectance = ev.reflectance;
  pdf = ev.pdf;
}

vec3 bounce(vec2 random, vec3 towards_viewer, vec3 normal) {
  material_state.is_incoming = dot(normal, -towards_viewer) < 0;
  material_state.ior1 = material_state.is_incoming ? 1.0 : material_state.mat.ior;
  material_state.ior2 = material_state.is_incoming ? material_state.mat.ior : 1.0;
  return pbr_resample(
      random, material_state.mat, towards_viewer, material_normal(normal), material_state.ior1, material_state.ior2);
}