#pragma once

#include "bvh.hpp"
#include <span>
#include <optional>
#include <stdexcept>
#include <vector>
#include <unordered_set>
#include <experimental/generator>
#include "sdf.hpp"
#include "../material.hpp"

namespace myrt {
struct geometry_info_t {
  using index_type = detail::default_index_type;

  index_type bvh_node_base_index;
  index_type bvh_index_base_index;
  index_type indices_base_index;
  index_type points_base_index;
  index_type index_count;
};

struct sdf_info_t {
  using id_type = detail::default_index_type;

  std::shared_ptr<sdf::instruction> root;
};

/* struct material_info_t
 {
   rnu::vec4ui8 albedo_rgba = rnu::vec4ui8(255, 255, 255, 255);
   rnu::vec4ui8 alt_color_rgba = rnu::vec4ui8(255, 255, 255, 255);
   float ior = 1.0f;
   float roughness = 1.0f;
   float metallic = 0.0f;
   float transmission = 0.0f;
   float emission = 0.0f;
   int has_albedo_texture = false;
   std::uint64_t albedo_texture = 0;
   int bbb[2];
 };*/

struct geometry_t;
using material_t = material_buffer::material_t;
struct sdf_t;
class scene;

struct geometric_object {
  std::string name;
  std::shared_ptr<geometry_t> geometry;
  std::shared_ptr<material_t> material;
  rnu::mat4 transformation;
  bool show = true;

  void enqueue() const;
  void enqueue(rnu::mat4 const& parent_transform) const;
};

struct sdf_object {
  std::string name;
  std::shared_ptr<sdf_t> sdf;
  rnu::mat4 transformation;
  bool show = true;

  template <typename T> void set(std::shared_ptr<parameter> const& parameter, T&& value);
  void enqueue() const;
  void enqueue(rnu::mat4 const& parent_transform) const;

private:
  scene* get_scene();
};

class scene {
public:
  friend struct sdf_object;
  struct scene_buffers {
    std::uint32_t indices_buffer = 0;
    std::uint32_t vertices_buffer = 0;
    std::uint32_t normals_buffer = 0;
    std::uint32_t uvs_buffer = 0;
    std::uint32_t bvh_nodes_buffer = 0;
    std::uint32_t bvh_indices_buffer = 0;
    std::uint32_t drawable_buffer = 0;
    std::uint32_t materials_buffer = 0;
    std::uint32_t materials_data_buffer = 0;
    std::uint32_t sdf_data_buffer = 0;
    std::uint32_t sdf_drawable_buffer = 0;
    std::uint32_t global_bvh_nodes_buffer = 0;
    std::uint32_t global_bvh_indices_buffer = 0;
  };
  constexpr static size_t number_of_buffers = sizeof(scene_buffers) / sizeof(std::uint32_t);

  using geometry_pointer = std::shared_ptr<geometry_t>;
  using material_pointer = std::shared_ptr<material_buffer::material_t const>;
  using sdf_pointer = std::shared_ptr<sdf_t>;

  struct prepare_result_t {
    bool materials_changed = false;
    bool materials_buffer_changed = false;
    bool geometries_changed = false;
    bool drawables_changed = false;
    bool sdfs_changed = false;
    bool sdf_buffer_changed = false;
  };

  using index_type = detail::default_index_type;
  using point_type = detail::default_point_type;
  using uv_type = rnu::vec2;

  struct drawable_geometry_t {
    rnu::mat4 transformation;
    rnu::mat4 inverse_transformation;
    geometry_info_t geometry_info;
    int material_index;
    int geometry_index;
    int pad[1];
  };
  struct drawable_sdf_t {
    rnu::mat4 transformation;
    rnu::mat4 inverse_transformation;
    int sdf_index;
    int pad[3];
  };
  struct material_reference_t {
    std::int32_t id;
    std::int32_t offset;
  };
  struct aligned_point_t {
    constexpr aligned_point_t(point_type point) : value(point) {}
    alignas(4 * sizeof(float)) point_type value;
  };

  scene();
  ~scene();

  template<typename T>
  decltype(auto) register_type() {
    return m_type_registry.create<T>();
  }


  [[nodiscard]] geometry_pointer push_geometry(std::span<index_type const> indices, std::span<point_type const> points,
      std::span<point_type const> normals, std::span<uv_type const> uvs);
  
  [[nodiscard]] decltype(auto) push_material_type(std::shared_ptr<object_type const> type, std::string glsl) {
    return m_material_types.push_type(std::move(type), std::move(glsl));
  }
  [[nodiscard]] decltype(auto) push_material(material_buffer::material_type_t type) {
    auto const& m = m_material_buffer.push(std::move(type));
    m_material_references.push_back(
        material_reference_t{.id = int(m->type_info->id), .offset = int(m->offset)});
    m_materials_changed = true;
    return m;
  }

  [[nodiscard]] sdf_pointer push_sdf(sdf_info_t info);

  void enqueue(geometry_t const* geometry, material_t const* material, rnu::mat4 const& transformation);
  void enqueue(const geometry_pointer& geometry, const material_pointer& material, rnu::mat4 const& transformation);

  void enqueue(sdf_t const* geometry, rnu::mat4 const& transformation);
  void enqueue(const sdf_pointer& geometry, rnu::mat4 const& transformation);

  //[[nodiscard]] std::shared_ptr<material_type> type_of(material_pointer const& material) const;

  // void set_material_parameter(const sdf_pointer& sdf, std::shared_ptr<int_param> const& parameter, material_pointer
  // material);

  template <typename T>
  void set_parameter(const sdf_pointer& sdf, std::shared_ptr<parameter> const& parameter, T&& value);
  template <typename T> void set_parameter(const sdf_t* sdf, std::shared_ptr<parameter> const& parameter, T&& value);

  void set_parameter(const sdf_pointer& sdf, std::shared_ptr<parameter> const& parameter, float* value_ptr);

  //template <typename T>
  //void set_parameter(const material_pointer& material, std::shared_ptr<parameter> const& parameter_name, T&& value);

  template <typename T> void set_parameter(const material_pointer& material, std::string const& name, T&& value);

 /* template <typename T>
  T get_parameter_value(const material_pointer& material, std::shared_ptr<parameter> const& param);
  template <typename T> T get_parameter_value(const material_pointer& material, std::string const& name);*/
  std::shared_ptr<parameter> get_parameter(const material_pointer& material, std::string const& name) const;

  struct hit {
    drawable_geometry_t* drawable;
    size_t index;
    float t;
  };
  std::optional<hit> pick(ray_t ray);

  std::experimental::generator<size_t> get_overlapping(myrt::aabb_t const& aabb);

  const material_pointer& default_material() const;

  /*[[nodiscard]] std::vector<material_pointer> const& materials() const noexcept {
    return m_available_materials;
  }*/
  [[nodiscard]] std::vector<sdf_pointer> const& sdfs() const noexcept {
    return m_available_sdfs;
  }

  sdf::glsl_assembler const& get_sdf_assembler() {
    return m_sdf_assembler;
  }
  //material_glsl_assembler const& get_material_assembler() {
  //  return m_material_assembler;
  //}
  material_buffer const& get_material_buffer() {
    return m_material_buffer;
  }

  prepare_result_t prepare();

  scene_buffers const& get_scene_buffers() const {
    return m_scene_buffers;
  }

  std::string generate_glsl() const {
    return generate_object_loaders(m_type_registry) + m_material_types.generate_code();
  }

private:
  sdf::glsl_assembly& get_sdf_assembly(sdf_t* sdf);
  sdf::glsl_assembly const& get_sdf_assembly(const sdf_t* sdf);
  //parameter_buffer_description& get_material_assembly(material_pointer const& mat);
  //parameter_buffer_description const& get_material_assembly(material_pointer const& mat) const;

  void erase_geometry_direct(const geometry_pointer& geometry);
  void erase_geometry_indirect(const geometry_pointer& geometry);

  scene_buffers m_scene_buffers;

  size_t m_last_drawable_hash = 0;
  size_t m_current_drawable_hash = ~0;
  size_t m_last_sdf_drawable_hash = ~0;
  size_t m_current_sdf_drawable_hash = ~0;

  std::vector<drawable_geometry_t> m_drawables;
  std::vector<drawable_geometry_t> m_last_drawables;
  std::vector<aabb_t> m_drawable_aabbs;
  std::vector<aabb_t> m_last_drawable_aabbs;

  std::vector<geometry_pointer> m_available_geometries;
  //std::vector<material_pointer> m_available_materials;
  std::vector<sdf_pointer> m_available_sdfs;
  std::vector<std::unique_ptr<bvh>> m_object_bvhs;
  std::vector<geometry_pointer> m_erase_on_prepare;
  // std::vector<material_pointer> m_erase_on_prepare_materials;

  std::vector<index_type> m_indices;
  std::vector<aligned_point_t> m_vertices;
  std::vector<aligned_point_t> m_normals;
  std::vector<uv_type> m_uvs;
  std::vector<aligned_node_t> m_bvh_nodes;
  std::vector<index_type> m_bvh_indices;
  std::vector<float> m_sdf_parameter_buffer;
  //std::vector<float> m_material_parameter_buffer;
  std::vector<material_reference_t> m_material_references;
  std::vector<drawable_sdf_t> m_sdf_drawables;

  //material_glsl_assembler m_material_assembler;
  myrt::types_registry m_type_registry;
  material_registry m_material_types;
  material_buffer m_material_buffer;
  // material_registry m_material_registry;
  sdf::glsl_assembler m_sdf_assembler;
  std::unique_ptr<bvh> m_global_bvh;
  material_pointer m_default_material;
  bool m_materials_changed = false;
  bool m_materials_buffer_changed = false;
  bool m_geometries_changed = false;
  bool m_sdfs_changed = false;
  bool m_sdf_buffer_changed = false;
};

template <typename T>
inline void scene::set_parameter(const sdf_pointer& sdf, std::shared_ptr<parameter> const& parameter, T&& value) {
  set_parameter(sdf.get(), parameter, std::forward<T>(value));
}
template <typename T>
inline void scene::set_parameter(const sdf_t* sdf, std::shared_ptr<parameter> const& parameter, T&& value) {
  m_sdf_buffer_changed = true;
  get_sdf_assembly(sdf).buffer_description.set_value(m_sdf_parameter_buffer.data(), parameter, std::forward<T>(value));
}

//template <typename T>
//void scene::set_parameter(const material_pointer& material, std::shared_ptr<parameter> const& parameter, T&& value) {
//  get_material_assembly(material).set_value(m_material_parameter_buffer.data(), parameter, std::forward<T>(value));
//}

template <typename T> void scene::set_parameter(const material_pointer& material, std::string const& name, T&& value) {
  //set_parameter(material, get_parameter(material, name), std::forward<T>(value));
  m_material_buffer.set(*material, name, std::forward<T>(value));
  m_materials_buffer_changed = true;
}
//
//template <typename T>
//inline T scene::get_parameter_value(const material_pointer& material, std::shared_ptr<parameter> const& param) {
//  return get_material_assembly(material).get_value<T>(m_material_parameter_buffer.data(), param);
//}

//template <typename T> inline T scene::get_parameter_value(const material_pointer& material, std::string const& name) {
//  return get_parameter_value<T>(material, get_parameter(material, name));
//}

template <typename T> void sdf_object::set(std::shared_ptr<parameter> const& parameter, T&& value) {
  get_scene()->set_parameter(sdf, parameter, std::forward<T>(value));
}
} // namespace myrt