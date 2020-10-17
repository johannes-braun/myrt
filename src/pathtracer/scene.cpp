#include "scene.hpp"
#include <algorithm>

namespace myrt
{
    struct geometry_t
    {
        geometry_info_t info;
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

    const scene::geometry_pointer& scene::push_geometry(std::span<index_type const> indices, std::span<point_type const> points, std::span<point_type const> normals) {
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
        bvh bvh(aabbs);
        m_bvh_nodes.insert(m_bvh_nodes.end(), bvh.nodes().begin(), bvh.nodes().end());
        m_bvh_indices.insert(m_bvh_indices.end(), bvh.reordered_indices().begin(), bvh.reordered_indices().end());

        m_geometries_changed = true;
        return m_available_geometries.emplace_back(std::make_shared<geometry_t>(geometry_t{
            .info = info,
            .aabb = bvh.aabb(),
            .scene = this
            }));
    }

    void scene::erase_geometry_indirect(const geometry_pointer& geometry) {
        m_erase_on_prepare.push_back(geometry);
    }
    void scene::erase_geometry_direct(const geometry_pointer& geometry)
    {
        m_geometries_changed = true;

        auto iter = std::find(m_available_geometries.begin(), m_available_geometries.end(), geometry);
        if (iter != m_available_geometries.end())
        {
            geometry_info_t info = geometry->info;

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

            iter = m_available_geometries.erase(iter);
            std::for_each(iter, m_available_geometries.end(), [&](const geometry_pointer& ptr) {
                ptr->info.bvh_index_base_index -= bvh_index_end - info.bvh_index_base_index;
                ptr->info.bvh_node_base_index -= bvh_node_end - info.bvh_node_base_index;
                ptr->info.indices_base_index -= indices_end - info.indices_base_index;
                ptr->info.points_base_index -= ponts_end - info.points_base_index;
                });
            m_drawables.clear();
        }
    }
    const scene::material_pointer& scene::push_material(material_info_t info)
    {
        m_materials_changed = true;
        auto const& ptr = m_available_materials.emplace_back(std::make_unique<material_t>());
        ptr->index = static_cast<int>(m_material_infos.size());
        m_material_infos.push_back(std::move(info));
        return ptr;
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
            .albedo_rgba = glm::u8vec4(255, 0, 255, 255),
            .ior = 1.0
            });
    }
    const scene::material_pointer& scene::default_material() const
    {
        return m_default_material;
    }
    void scene::enqueue(const geometry_pointer& geometry, const material_pointer& material, glm::mat4 const& transformation)
    {
        if (geometry)
            enqueue(geometry.get(), material.get(), transformation);
    }
    void scene::enqueue(geometry_t const* geometry, material_t const* material, glm::mat4 const& transformation)
    {
        m_drawables.push_back(drawable_geometry_t{
            .transformation = transformation,
            .inverse_transformation = inverse(transformation),
            .geometry_info = geometry->info,
            .material_index = material ? material->index : 0
            });

        aabb_t transformed;
        for (unsigned i = 0; i < 8; ++i)
        {
            glm::vec3 const point(
                geometry->aabb[i & 0b001].x,
                geometry->aabb[i & 0b010].y,
                geometry->aabb[i & 0b100].z);
            glm::vec4 const transformed_point = transformation * glm::vec4(point, 1.0f);
            transformed.enclose(glm::vec3(transformed_point));
        }
        m_drawable_aabbs.push_back(transformed);
    }

    void scene::prepare() {
        const auto fill_buffer = [this](auto buffer, auto const& vector) {
            glNamedBufferData(buffer, detail::vector_bytes(vector), vector.data(), GL_DYNAMIC_DRAW);
        };
        if (!m_drawables.empty())
        {
            bvh object_bvh(m_drawable_aabbs);
            fill_buffer(m_gl_objects.global_bvh_nodes_buffer, object_bvh.nodes());
            fill_buffer(m_gl_objects.global_bvh_indices_buffer, object_bvh.reordered_indices());
            glNamedBufferData(m_gl_objects.drawable_buffer, detail::vector_bytes(m_drawables), m_drawables.data(), GL_DYNAMIC_DRAW);
            m_drawables.clear();
            m_drawable_aabbs.clear();
        }
        if (m_materials_changed)
        {
            m_materials_changed = false;
            fill_buffer(m_gl_objects.materials_buffer, m_material_infos);
        }
        if (m_geometries_changed)
        {
            m_geometries_changed = false;
            fill_buffer(m_gl_objects.indices_buffer, m_indices);
            fill_buffer(m_gl_objects.vertices_buffer, m_vertices);
            fill_buffer(m_gl_objects.normals_buffer, m_normals);
            fill_buffer(m_gl_objects.bvh_nodes_buffer, m_bvh_nodes);
            fill_buffer(m_gl_objects.bvh_indices_buffer, m_bvh_indices);
        }
        for (auto& to_erase : m_erase_on_prepare)
            erase_geometry_direct(to_erase);
    }

    void scene::bind_buffers() {
        prepare();
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buffer_binding_bvh_nodes, m_gl_objects.bvh_nodes_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buffer_binding_bvh_indices, m_gl_objects.bvh_indices_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buffer_binding_indices, m_gl_objects.indices_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buffer_binding_vertices, m_gl_objects.vertices_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buffer_binding_normals, m_gl_objects.normals_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buffer_binding_drawables, m_gl_objects.drawable_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buffer_binding_global_bvh_nodes, m_gl_objects.global_bvh_nodes_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buffer_binding_global_bvh_indices, m_gl_objects.global_bvh_indices_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, buffer_binding_materials, m_gl_objects.materials_buffer);
    }

    void geometric_object::enqueue() const
    {
        if (geometry && geometry->scene)
        {
            geometry->scene->enqueue(geometry, material, transformation);
        }
    }
}