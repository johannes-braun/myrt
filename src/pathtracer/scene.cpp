#include "scene.hpp"
#include <algorithm>

namespace myrt
{
    struct geometry_t
    {
        geometry_info_t info;
        int index = 0;
        aabb_t aabb;
        scene* scene = nullptr;
    };

    struct material_t
    {
        int index = 0;
    };

    namespace detail
    {
        namespace {
            template<typename T>
            [[nodiscard]] size_t vector_bytes(std::vector<T> const& vector)
            {
                return vector.size() * sizeof(T);
            }
        }
    }

    scene::~scene()
    {
        glDeleteBuffers(1, &m_gl_objects.indices_buffer);
        glDeleteBuffers(1, &m_gl_objects.vertices_buffer);
        glDeleteBuffers(1, &m_gl_objects.normals_buffer);
        glDeleteBuffers(1, &m_gl_objects.bvh_nodes_buffer);
        glDeleteBuffers(1, &m_gl_objects.bvh_indices_buffer);
        glDeleteBuffers(1, &m_gl_objects.drawable_buffer);
        glDeleteBuffers(1, &m_gl_objects.materials_buffer);
        glDeleteBuffers(1, &m_gl_objects.global_bvh_nodes_buffer);
        glDeleteBuffers(1, &m_gl_objects.global_bvh_indices_buffer);
        for (auto const& geometry : m_available_geometries)
        {
            geometry->scene = nullptr;
        }
    }

    scene::geometry_pointer scene::push_geometry(std::span<index_type const> indices, std::span<point_type const> points, std::span<point_type const> normals) {
        geometry_info_t info{
            .bvh_node_base_index = static_cast<index_type>(m_bvh_nodes.size()),
            .bvh_index_base_index = static_cast<index_type>(m_bvh_indices.size()),
            .indices_base_index = static_cast<index_type>(m_indices.size()),
            .points_base_index = static_cast<index_type>(m_vertices.size())
        };

        m_indices.insert(m_indices.end(), indices.begin(), indices.end());
        m_vertices.insert(m_vertices.end(), points.begin(), points.end());
        m_normals.insert(m_normals.end(), normals.begin(), normals.end());
        const auto aabbs = generate_triangle_bounds(indices, points);
        bvh& b = *m_object_bvhs.emplace_back(std::make_unique<bvh>(aabbs));
        m_bvh_nodes.insert(m_bvh_nodes.end(), b.nodes().begin(), b.nodes().end());
        m_bvh_indices.insert(m_bvh_indices.end(), b.reordered_indices().begin(), b.reordered_indices().end());

        m_geometries_changed = true;
        auto ptr = m_available_geometries.emplace_back(std::make_shared<geometry_t>(geometry_t{
            .info = info,
            .index = int(m_available_geometries.size()),
            .aabb = b.aabb(),
            .scene = this
            }));

        return std::shared_ptr<geometry_t>(ptr.get(), [this, ptr](geometry_t*) mutable {
            erase_geometry_indirect(ptr);
            });
    }
    material_info_t scene::info_of(material_pointer const& material) const
    {
        return m_material_infos.at(material->index);
    }
    void scene::erase_material_indirect(const material_pointer& material) {
        m_erase_on_prepare_materials.push_back(material);
    }
    void scene::erase_geometry_indirect(const geometry_pointer& geometry) {
        m_erase_on_prepare.push_back(geometry);
    }
    void scene::erase_material_direct(const material_pointer& material) {
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
    }
    void scene::erase_geometry_direct(const geometry_pointer& geometry)
    {
        m_geometries_changed = true;

        auto iter = std::find(m_available_geometries.begin(), m_available_geometries.end(), geometry);
        if (iter != m_available_geometries.end())
        {
            geometry_info_t info = geometry->info;

            m_object_bvhs.erase(m_object_bvhs.begin() + std::distance(m_available_geometries.begin(), iter));
            auto next = std::next(iter);
            const auto [bvh_index_end, bvh_node_end, indices_end, ponts_end] = [&] {
                const auto next_is_end = next == m_available_geometries.end();
                if (next_is_end)
                    return std::tuple(index_type(m_bvh_indices.size()),
                        index_type(m_bvh_nodes.size()),
                        index_type(m_indices.size()),
                        index_type(m_vertices.size()));
                return std::tuple(next->get()->info.bvh_index_base_index,
                    next->get()->info.bvh_node_base_index,
                    next->get()->info.indices_base_index,
                    next->get()->info.points_base_index);
            }();

            m_bvh_indices.erase(m_bvh_indices.begin() + info.bvh_index_base_index, m_bvh_indices.begin() + bvh_index_end);
            m_bvh_nodes.erase(m_bvh_nodes.begin() + info.bvh_node_base_index, m_bvh_nodes.begin() + bvh_node_end);
            m_indices.erase(m_indices.begin() + info.indices_base_index, m_indices.begin() + indices_end);
            m_vertices.erase(m_vertices.begin() + info.points_base_index, m_vertices.begin() + ponts_end);
            m_normals.erase(m_normals.begin() + info.points_base_index, m_normals.begin() + ponts_end);

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
    scene::material_pointer scene::push_material(material_info_t info)
    {
        m_materials_changed = true;
        auto const& ptr = m_available_materials.emplace_back(std::make_unique<material_t>());
        ptr->index = static_cast<int>(m_material_infos.size());
        m_material_infos.push_back(std::move(info));

        return std::shared_ptr<material_t>(ptr.get(), [this, ptr](material_t*) mutable {
            erase_material_indirect(ptr);
            });
    }

    scene::scene() {
        glCreateBuffers(1, &m_gl_objects.indices_buffer);
        glCreateBuffers(1, &m_gl_objects.vertices_buffer);
        glCreateBuffers(1, &m_gl_objects.normals_buffer);
        glCreateBuffers(1, &m_gl_objects.bvh_nodes_buffer);
        glCreateBuffers(1, &m_gl_objects.bvh_indices_buffer);
        glCreateBuffers(1, &m_gl_objects.drawable_buffer);
        glCreateBuffers(1, &m_gl_objects.materials_buffer);
        glCreateBuffers(1, &m_gl_objects.global_bvh_nodes_buffer);
        glCreateBuffers(1, &m_gl_objects.global_bvh_indices_buffer);

        m_default_material = push_material(material_info_t{
            .albedo_rgba = rnu::vec4ui8(255, 0, 255, 255),
            .ior = 1.0
            });
    }
    const scene::material_pointer& scene::default_material() const
    {
        return m_default_material;
    }
    void scene::enqueue(const geometry_pointer& geometry, const material_pointer& material, rnu::mat4 const& transformation)
    {
        if (geometry)
            enqueue(geometry.get(), material.get(), transformation);
    }

    [[nodiscard]] size_t hash(geometry_t const* geometry, material_t const* material, rnu::mat4 const& transformation) noexcept
    {
        size_t const geom_hash = std::hash<decltype(geometry)>()(geometry);
        size_t const mat_hash = std::hash<decltype(material)>()(material);
        size_t tf_hash = 0;
        for (size_t c = 0; c < transformation.cols; ++c)
        {
            for (size_t r = 0; r < transformation.rows; ++r) {
                tf_hash ^= std::hash<float>()(transformation.at(c, r));
            }
        }
        return geom_hash ^ mat_hash ^ tf_hash;
    }

    void scene::enqueue(geometry_t const* geometry, material_t const* material, rnu::mat4 const& transformation)
    {
        m_drawables.push_back(drawable_geometry_t{
            .transformation = transformation,
            .inverse_transformation = inverse(transformation),
            .geometry_info = geometry->info,
            .material_index = material ? material->index : 0,
            .geometry_index = geometry->index
            });

        m_current_drawable_hash ^= hash(geometry, material, transformation);

        aabb_t transformed;
        for (unsigned i = 0; i < 8; ++i)
        {
            rnu::vec3 const point(
                geometry->aabb[i & 0b001].x,
                geometry->aabb[i & 0b010].y,
                geometry->aabb[i & 0b100].z);
            rnu::vec4 const transformed_point = transformation * rnu::vec4(point.x, point.y, point.z, 1.0f);
            transformed.enclose(rnu::vec3(transformed_point));
        }
        m_drawable_aabbs.push_back(transformed);
    }

    std::optional<scene::hit> scene::pick(ray_t ray)
    {
        if (!m_global_bvh && !m_drawables.empty())
        {
            m_global_bvh = std::make_unique<bvh>(m_drawable_aabbs);
        }
        std::optional<hit> h = std::nullopt;

        if (!m_global_bvh || m_drawables.empty())
            return std::nullopt;

        for (const auto& drawable_index : m_global_bvh->traverse(ray))
        {
            ray_t transformed;
            transformed.origin = rnu::vec3(m_drawables[drawable_index].inverse_transformation * rnu::vec4(ray.origin.x, ray.origin.y, ray.origin.z, 1.0));
            transformed.direction = rnu::vec3(m_drawables[drawable_index].inverse_transformation * rnu::vec4(ray.direction.x, ray.direction.y, ray.direction.z, 0.0));
            transformed.length = ray.length;

            const auto gi = m_drawables[drawable_index].geometry_info;

            for (const auto& triangle_index : m_object_bvhs[m_drawables[drawable_index].geometry_index]->traverse(transformed))
            {
                rnu::vec2 barycentric;
                auto const& v0 = m_vertices[m_indices[gi.indices_base_index] + gi.points_base_index].value;
                auto const& v1 = m_vertices[m_indices[gi.indices_base_index + 1] + gi.points_base_index].value;
                auto const& v2 = m_vertices[m_indices[gi.indices_base_index + 2] + gi.points_base_index].value;
                const auto intersect_triangle = transformed.intersect(v0, v1, v2, barycentric);
                
                if (intersect_triangle && (!h || *intersect_triangle > h->t))
                {
                    h = hit{};
                    h->t = *intersect_triangle;
                    h->index = drawable_index;
                }
            }
        }

        return h;
    }

    bool scene::prepare() {
        bool has_changed = false;
        const auto fill_buffer = [this](auto buffer, auto const& vector) {
            glNamedBufferData(buffer, detail::vector_bytes(vector), vector.data(), GL_DYNAMIC_DRAW);
        };
        if (!m_drawables.empty() && (m_current_drawable_hash != m_last_drawable_hash))
        {
            m_last_drawable_hash = m_current_drawable_hash;
            m_global_bvh = std::make_unique<bvh>(m_drawable_aabbs);
            fill_buffer(m_gl_objects.global_bvh_nodes_buffer, m_global_bvh->nodes());
            fill_buffer(m_gl_objects.global_bvh_indices_buffer, m_global_bvh->reordered_indices());
            glNamedBufferData(m_gl_objects.drawable_buffer, detail::vector_bytes(m_drawables), m_drawables.data(), GL_DYNAMIC_DRAW);
            has_changed = true;
        }
        m_drawables.clear();
        m_drawable_aabbs.clear();
        m_current_drawable_hash = 0;
        if (m_materials_changed)
        {
            m_materials_changed = false;
            fill_buffer(m_gl_objects.materials_buffer, m_material_infos);
            has_changed = true;
        }
        if (m_geometries_changed)
        {
            m_geometries_changed = false;
            fill_buffer(m_gl_objects.indices_buffer, m_indices);
            fill_buffer(m_gl_objects.vertices_buffer, m_vertices);
            fill_buffer(m_gl_objects.normals_buffer, m_normals);
            fill_buffer(m_gl_objects.bvh_nodes_buffer, m_bvh_nodes);
            fill_buffer(m_gl_objects.bvh_indices_buffer, m_bvh_indices);
            has_changed = true;
        }
        if (!m_erase_on_prepare.empty() || !m_erase_on_prepare_materials.empty())
            printf("Erasing %d geometries and %d materials...\n", int(m_erase_on_prepare.size()), int(m_erase_on_prepare_materials.size()));
        if (!m_erase_on_prepare.empty()) {
            for (auto& to_erase : m_erase_on_prepare)
            {
                erase_geometry_direct(to_erase);
                has_changed = true;
            }
            m_erase_on_prepare.clear();
            m_last_drawable_hash = 0;
        }
        if (!m_erase_on_prepare_materials.empty()) {
            for (auto& to_erase : m_erase_on_prepare_materials)
            {
                erase_material_direct(to_erase);
                has_changed = true;
            }
            m_erase_on_prepare_materials.clear();
            m_last_drawable_hash = 0;
        }
        return has_changed;
    }

    bool scene::prepare_and_bind() {
        const bool has_changed = prepare();
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buffer_binding_bvh_nodes, m_gl_objects.bvh_nodes_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buffer_binding_bvh_indices, m_gl_objects.bvh_indices_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buffer_binding_indices, m_gl_objects.indices_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buffer_binding_vertices, m_gl_objects.vertices_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buffer_binding_normals, m_gl_objects.normals_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buffer_binding_drawables, m_gl_objects.drawable_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buffer_binding_global_bvh_nodes, m_gl_objects.global_bvh_nodes_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buffer_binding_global_bvh_indices, m_gl_objects.global_bvh_indices_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buffer_binding_materials, m_gl_objects.materials_buffer);
        return has_changed;
    }

    void geometric_object::enqueue() const
    {
        if (geometry && geometry->scene)
        {
            geometry->scene->enqueue(geometry, material, transformation);
        }
    }
}