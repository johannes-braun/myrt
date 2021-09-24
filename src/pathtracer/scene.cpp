#include "scene.hpp"
#include <algorithm>
#include "material.hpp"
#include <mygl/mygl.hpp>

namespace myrt {
struct geometry_t {
  geometry_info_t info;
  int index = 0;
  aabb_t aabb;
  scene* scene = nullptr;
};

struct material_t {
  int index;
  parameter_buffer_description buffer_description;
  std::shared_ptr<material> material;
};

struct sdf_t {
  sdf_info_t info;
  sdf::glsl_assembly assembly;
  scene* scene = nullptr;
  int index = 0;
  size_t buffer_offset = 0;
};

namespace detail {
  namespace {
    template <typename T> [[nodiscard]] size_t vector_bytes(std::vector<T> const& vector) {
      return vector.size() * sizeof(T);
    }
  } // namespace
} // namespace detail

scene::~scene() {
  glDeleteBuffers(number_of_buffers, reinterpret_cast<std::uint32_t*>(&m_scene_buffers));
  for (auto const& geometry : m_available_geometries) {
    geometry->scene = nullptr;
  }
}

scene::geometry_pointer scene::push_geometry(std::span<index_type const> indices, std::span<point_type const> points,
    std::span<point_type const> normals, std::span<uv_type const> uvs) {
  geometry_info_t info{.bvh_node_base_index = static_cast<index_type>(m_bvh_nodes.size()),
      .bvh_index_base_index = static_cast<index_type>(m_bvh_indices.size()),
      .indices_base_index = static_cast<index_type>(m_indices.size()),
      .points_base_index = static_cast<index_type>(m_vertices.size()),
      .index_count = static_cast<index_type>(indices.size())};

  m_indices.insert(m_indices.end(), indices.begin(), indices.end());
  m_vertices.insert(m_vertices.end(), points.begin(), points.end());
  m_normals.insert(m_normals.end(), normals.begin(), normals.end());

  if (uvs.size() == points.size())
    m_uvs.insert(m_uvs.end(), uvs.begin(), uvs.end());
  else
    m_uvs.resize(m_uvs.size() + points.size());

  const auto aabbs = generate_triangle_bounds(indices, points);

  bvh& b = *m_object_bvhs.emplace_back(std::make_unique<bvh>(aabbs, split_aabbs));
  m_bvh_nodes.insert(m_bvh_nodes.end(), b.nodes().begin(), b.nodes().end());
  m_bvh_indices.insert(m_bvh_indices.end(), b.reordered_indices().begin(), b.reordered_indices().end());

  m_geometries_changed = true;
  auto ptr = m_available_geometries.emplace_back(std::make_shared<geometry_t>(
      geometry_t{.info = info, .index = int(m_available_geometries.size()), .aabb = b.aabb(), .scene = this}));

  return std::shared_ptr<geometry_t>(ptr.get(), [this, ptr](geometry_t*) mutable { erase_geometry_direct(ptr); });
}
std::shared_ptr<material_type> scene::type_of(material_pointer const& material) const {
  return material->material->type();
}
/*void scene::update_material(material_pointer const& material, const material_info_t& info)
{
  m_material_infos.at(material->index) = info;
  m_materials_changed = true;
}*/
/*void scene::set_material_parameter(const sdf_pointer& sdf, std::shared_ptr<int_param> const& parameter,
material_pointer material)
{
  m_sdf_buffer_changed = true;
  sdf->assembly.buffer_description.set_value(m_sdf_parameter_buffer.data(), parameter, material->index);
}
void scene::erase_material_indirect(const material_pointer& material) {
  m_erase_on_prepare_materials.push_back(material);
}*/
void scene::erase_geometry_indirect(const geometry_pointer& geometry) {
  m_erase_on_prepare.push_back(geometry);
}
/* void scene::erase_material_direct(const material_pointer& material) {
   m_materials_changed = true;

   auto iter = std::find(m_available_materials.begin(), m_available_materials.end(), material);
   if (iter != m_available_materials.begin() && iter != m_available_materials.end())
   {
     int const index = iter->get()->index;
     std::for_each(m_available_materials.begin(), m_available_materials.end(), [&](const material_pointer& ptr) {
       if (ptr->index > index)
         ptr->index = std::max(0, ptr->index - 1);
       else if (ptr->index == index)
         ptr->index = 0;
       });
     m_material_infos.erase(m_material_infos.begin() + index);
     std::iter_swap(iter, std::prev(m_available_materials.end()));
     m_available_materials.pop_back();
   }
 }*/

void scene::set_parameter(const sdf_pointer& sdf, std::shared_ptr<parameter> const& parameter, float* value_ptr) {
  m_sdf_buffer_changed = true;
  sdf->assembly.buffer_description.set_value(m_sdf_parameter_buffer.data(), parameter, value_ptr);
}
void scene::erase_geometry_direct(const geometry_pointer& geometry) {
  m_geometries_changed = true;

  auto iter = std::find(m_available_geometries.begin(), m_available_geometries.end(), geometry);
  if (iter != m_available_geometries.end()) {
    geometry_info_t info = geometry->info;

    m_object_bvhs.erase(m_object_bvhs.begin() + std::distance(m_available_geometries.begin(), iter));
    auto next = std::next(iter);
    const auto [bvh_index_end, bvh_node_end, indices_end, ponts_end] = [&] {
      const auto next_is_end = next == m_available_geometries.end();
      if (next_is_end)
        return std::tuple(index_type(m_bvh_indices.size()), index_type(m_bvh_nodes.size()),
            index_type(m_indices.size()), index_type(m_vertices.size()));
      return std::tuple(next->get()->info.bvh_index_base_index, next->get()->info.bvh_node_base_index,
          next->get()->info.indices_base_index, next->get()->info.points_base_index);
    }();

    if (!m_bvh_indices.empty())
      m_bvh_indices.erase(m_bvh_indices.begin() + info.bvh_index_base_index, m_bvh_indices.begin() + bvh_index_end);
    if (!m_bvh_nodes.empty())
      m_bvh_nodes.erase(m_bvh_nodes.begin() + info.bvh_node_base_index, m_bvh_nodes.begin() + bvh_node_end);
    if (!m_indices.empty())
      m_indices.erase(m_indices.begin() + info.indices_base_index, m_indices.begin() + indices_end);
    if (!m_vertices.empty())
      m_vertices.erase(m_vertices.begin() + info.points_base_index, m_vertices.begin() + ponts_end);
    if (!m_normals.empty())
      m_normals.erase(m_normals.begin() + info.points_base_index, m_normals.begin() + ponts_end);
    if (!m_uvs.empty())
      m_uvs.erase(m_uvs.begin() + info.points_base_index, m_uvs.begin() + ponts_end);

    std::for_each(iter, m_available_geometries.end(), [&](const geometry_pointer& ptr) {
      ptr->info.bvh_index_base_index -= bvh_index_end - info.bvh_index_base_index;
      ptr->info.bvh_node_base_index -= bvh_node_end - info.bvh_node_base_index;
      ptr->info.indices_base_index -= indices_end - info.indices_base_index;
      ptr->info.points_base_index -= ponts_end - info.points_base_index;
    });
    std::iter_swap(iter, std::prev(m_available_geometries.end()));
    m_available_geometries.pop_back();
    m_drawables.clear();
  }
}
scene::material_pointer scene::push_material(std::shared_ptr<material> type) {
  m_materials_changed = true;
  m_materials_buffer_changed = true;

  auto const& ptr = m_available_materials.emplace_back(std::make_unique<material_t>());
  ptr->index = static_cast<int>(m_material_references.size());
  ptr->buffer_description = m_material_assembler.append(type);
  ptr->buffer_description.base_offset = 0;
  // ptr->buffer_description.base_offset += static_cast<int>(m_material_parameter_buffer.size());
  ptr->material = type;
  m_material_references.push_back(
      material_reference_t{.id = ptr->buffer_description.id, .offset = ptr->buffer_description.base_offset});
  m_material_parameter_buffer.resize(m_material_parameter_buffer.size() + ptr->buffer_description.blocks_required);
  ptr->buffer_description.apply_defaults(m_material_parameter_buffer);
  // m_material_infos.push_back(std::move(info));

  return std::shared_ptr<material_t>(ptr.get(), [this, ptr](material_t*) mutable {
    // todo...
  });
}
std::shared_ptr<parameter> scene::get_parameter(const material_pointer& material, std::string const& name) const {
  if (auto const it = material->material->parameters().find(name); it != material->material->parameters().end())
    return it->second;
  return nullptr;
}
parameter_buffer_description& scene::get_material_assembly(material_pointer const& mat) {
  return mat->buffer_description;
}
parameter_buffer_description const& scene::get_material_assembly(material_pointer const& mat) const {
  return mat->buffer_description;
}

scene::sdf_pointer scene::push_sdf(sdf_info_t info) {
  auto const assembly = m_sdf_assembler.append(info.root);

  // m_sdf_buffer_changed = true;
  m_sdfs_changed = true;
  auto const& ptr = m_available_sdfs.emplace_back(std::make_unique<sdf_t>());
  ptr->index = static_cast<int>(m_available_sdfs.size()) - 1;
  ptr->info = std::move(info);
  ptr->scene = this;
  ptr->assembly = std::move(assembly);
  ptr->buffer_offset = m_sdf_parameter_buffer.size();
  m_sdf_parameter_buffer.resize(m_sdf_parameter_buffer.size() + ptr->assembly.buffer_description.blocks_required);
  ptr->assembly.initialize_from_default(m_sdf_parameter_buffer.data(), ptr->info.root);

  return ptr;
}

sdf::glsl_assembly const& scene::get_sdf_assembly(const sdf_t* sdf) {
  return sdf->assembly;
}

sdf::glsl_assembly& scene::get_sdf_assembly(sdf_t* sdf) {
  return sdf->assembly;
}

scene::scene() {
  glCreateBuffers(number_of_buffers, reinterpret_cast<std::uint32_t*>(&m_scene_buffers));

  /* m_default_material = push_material(material_info_t{
       .albedo_rgba = rnu::vec4ui8(255, 0, 255, 255),
       .ior = 1.0
     });*/
}
const scene::material_pointer& scene::default_material() const {
  return m_default_material;
}
void scene::enqueue(
    const geometry_pointer& geometry, const material_pointer& material, rnu::mat4 const& transformation) {
  if (geometry)
    enqueue(geometry.get(), material.get(), transformation);
}

[[nodiscard]] size_t hash(
    geometry_t const* geometry, material_t const* material, rnu::mat4 const& transformation) noexcept {
  size_t const geom_hash = std::hash<decltype(geometry)>()(geometry);
  size_t const mat_hash = std::hash<decltype(material)>()(material);
  size_t tf_hash = 0;
  for (size_t c = 0; c < transformation.columns; ++c) {
    for (size_t r = 0; r < transformation.rows; ++r) {
      tf_hash ^= std::hash<float>()(transformation.at(c, r));
    }
  }
  return 3145551 * geom_hash + 16136514 * mat_hash + 2138867 * tf_hash;
}

[[nodiscard]] size_t hash(sdf_t const* sdf, rnu::mat4 const& transformation) noexcept {
  size_t const sdf_hash = std::hash<decltype(sdf)>()(sdf);
  size_t tf_hash = 0;
  for (size_t c = 0; c < transformation.columns; ++c) {
    for (size_t r = 0; r < transformation.rows; ++r) {
      tf_hash ^= std::hash<float>()(transformation.at(c, r));
    }
  }
  return sdf_hash ^ tf_hash;
}

void scene::enqueue(geometry_t const* geometry, material_t const* material, rnu::mat4 const& transformation) {
  m_drawables.push_back(drawable_geometry_t{.transformation = transformation,
      .inverse_transformation = inverse(transformation),
      .geometry_info = geometry->info,
      .material_index = material ? material->index : 0,
      .geometry_index = geometry->index});

  m_current_drawable_hash ^= hash(geometry, material, transformation);

  aabb_t transformed;
  for (unsigned i = 0; i < 8; ++i) {
    rnu::vec3 const point(geometry->aabb[i & 0b001].x, geometry->aabb[i & 0b010].y, geometry->aabb[i & 0b100].z);
    rnu::vec4 const transformed_point = transformation * rnu::vec4(point.x, point.y, point.z, 1.0f);
    transformed.enclose(rnu::vec3(transformed_point));
  }
  m_drawable_aabbs.push_back(transformed);
}

std::optional<scene::hit> scene::pick(ray_t ray) {
  if (!m_global_bvh && !m_drawables.empty()) {
    m_global_bvh = std::make_unique<bvh>(m_drawable_aabbs, split_aabbs);
  }
  std::optional<hit> h = std::nullopt;

  if (!m_global_bvh)
    return std::nullopt;

  if (m_drawables.empty() && m_last_drawables.empty())
    return std::nullopt;

  auto& drawables = m_drawables.empty() ? m_last_drawables : m_drawables;

  for (const auto& drawable_index : m_global_bvh->traverse(ray)) {
    ray_t transformed;
    transformed.origin = rnu::vec3(drawables[drawable_index].inverse_transformation * rnu::vec4(ray.origin, 1.0));
    transformed.direction = rnu::vec3(drawables[drawable_index].inverse_transformation * rnu::vec4(ray.direction, 0.0));
    transformed.length = static_cast<float>(ray.length * norm(transformed.direction));

    const auto gi = drawables[drawable_index].geometry_info;

    for (const auto& triangle_index : m_object_bvhs[drawables[drawable_index].geometry_index]->traverse(transformed)) {
      rnu::vec2 barycentric;
      auto const& v0 = m_vertices[m_indices[gi.indices_base_index + 3 * triangle_index] + gi.points_base_index].value;
      auto const& v1 =
          m_vertices[m_indices[gi.indices_base_index + 1 + 3 * triangle_index] + gi.points_base_index].value;
      auto const& v2 =
          m_vertices[m_indices[gi.indices_base_index + 2 + 3 * triangle_index] + gi.points_base_index].value;
      const auto intersect_triangle = transformed.intersect(v0, v1, v2, barycentric);

      if (intersect_triangle && (!h || *intersect_triangle < h->t)) {
        h = hit{};
        h->drawable = &drawables[drawable_index];
        h->t = *intersect_triangle;
        h->index = drawable_index;
      }
    }
  }

  return h;
}

std::experimental::generator<size_t> scene::get_overlapping(myrt::aabb_t const& aabb) {
  co_return;

  for (auto const node : m_global_bvh->traverse([&](aabb_t const& test) -> std::optional<float> {
         auto const intersect = test.intersect(aabb);
         if (!intersect.empty()) {
           return 1.f / intersect.surface_area();
         }
         return std::nullopt;
       })) {
    co_yield node;
  }
}

scene::prepare_result_t scene::prepare() {
  prepare_result_t result;
  const auto fill_buffer = [this](auto buffer, auto const& vector) {
    glNamedBufferData(buffer, detail::vector_bytes(vector), vector.data(), GL_DYNAMIC_DRAW);
  };
  if (!m_drawables.empty() && (m_current_drawable_hash != m_last_drawable_hash)) {
    m_last_drawable_hash = m_current_drawable_hash;
    m_global_bvh = std::make_unique<bvh>(m_drawable_aabbs, dont_split);
    fill_buffer(m_scene_buffers.global_bvh_nodes_buffer, m_global_bvh->nodes());
    fill_buffer(m_scene_buffers.global_bvh_indices_buffer, m_global_bvh->reordered_indices());
    glNamedBufferData(
        m_scene_buffers.drawable_buffer, detail::vector_bytes(m_drawables), m_drawables.data(), GL_DYNAMIC_DRAW);
    result.drawables_changed = true;
  }
  if (!m_sdf_drawables.empty() && (m_current_sdf_drawable_hash != m_last_sdf_drawable_hash)) {
    m_last_sdf_drawable_hash = m_current_sdf_drawable_hash;
    glNamedBufferData(m_scene_buffers.sdf_drawable_buffer, detail::vector_bytes(m_sdf_drawables),
        m_sdf_drawables.data(), GL_DYNAMIC_DRAW);
    result.drawables_changed = true;
    m_sdf_buffer_changed = true;
  }
  m_last_drawables = m_drawables;
  m_last_drawable_aabbs = m_drawable_aabbs;
  m_drawables.clear();
  m_drawable_aabbs.clear();

  m_sdf_drawables.clear();
  m_current_drawable_hash = 0;
  m_current_sdf_drawable_hash = 0;
  if (m_materials_changed) {
    m_materials_changed = false;
    fill_buffer(m_scene_buffers.materials_buffer, m_material_references);
    result.materials_changed = true;
  }
  if (m_materials_buffer_changed) {
    m_materials_buffer_changed = false;
    fill_buffer(m_scene_buffers.materials_data_buffer, m_material_parameter_buffer);
    result.materials_buffer_changed = true;
  }
  if (m_geometries_changed) {
    m_geometries_changed = false;
    fill_buffer(m_scene_buffers.indices_buffer, m_indices);
    fill_buffer(m_scene_buffers.vertices_buffer, m_vertices);
    fill_buffer(m_scene_buffers.normals_buffer, m_normals);
    fill_buffer(m_scene_buffers.uvs_buffer, m_uvs);
    fill_buffer(m_scene_buffers.bvh_nodes_buffer, m_bvh_nodes);
    fill_buffer(m_scene_buffers.bvh_indices_buffer, m_bvh_indices);
    result.geometries_changed = true;
  }
  if (!m_erase_on_prepare.empty() /* || !m_erase_on_prepare_materials.empty()*/)
    printf("Erasing %d geometries...\n", int(m_erase_on_prepare.size()) /*, int(m_erase_on_prepare_materials.size())*/);
  if (!m_erase_on_prepare.empty()) {
    for (auto& to_erase : m_erase_on_prepare) {
      erase_geometry_direct(to_erase);
      result.geometries_changed = true;
    }
    m_erase_on_prepare.clear();
    m_last_drawable_hash = 0;
  }
  /* if (!m_erase_on_prepare_materials.empty()) {
     for (auto& to_erase : m_erase_on_prepare_materials)
     {
       erase_material_direct(to_erase);
       result.materials_changed = true;
     }
     m_erase_on_prepare_materials.clear();
     m_last_drawable_hash = 0;
   }*/
  if (m_sdfs_changed || m_sdf_buffer_changed) {
    fill_buffer(m_scene_buffers.sdf_data_buffer, m_sdf_parameter_buffer);
  }
  result.sdfs_changed = m_sdfs_changed;
  result.sdf_buffer_changed = m_sdf_buffer_changed;

  m_sdfs_changed = false;
  m_sdf_buffer_changed = false;

  return result;
}

void scene::enqueue(sdf_t const* sdf, rnu::mat4 const& transformation) {
  auto& info = m_sdf_drawables.emplace_back();
  info.transformation = transformation;
  info.inverse_transformation = inverse(transformation);
  info.sdf_index = sdf->index;
  m_current_sdf_drawable_hash ^= hash(sdf, transformation);
}

void scene::enqueue(const sdf_pointer& sdf, rnu::mat4 const& transformation) {
  enqueue(sdf.get(), transformation);
}

void geometric_object::enqueue() const {
  if (show && geometry && geometry->scene) {
    geometry->scene->enqueue(geometry, material, transformation);
  }
}
void sdf_object::enqueue() const {
  if (show && sdf && sdf->scene) {
    sdf->scene->enqueue(sdf, transformation);
  }
}

void geometric_object::enqueue(rnu::mat4 const& parent_transform) const {
  if (show && geometry && geometry->scene) {
    geometry->scene->enqueue(geometry, material, parent_transform * transformation);
  }
}
void sdf_object::enqueue(rnu::mat4 const& parent_transform) const {
  if (show && sdf && sdf->scene) {
    sdf->scene->enqueue(sdf, parent_transform * transformation);
  }
}
scene* sdf_object::get_scene() {
  return sdf->scene;
}
} // namespace myrt