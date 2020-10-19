#pragma once

#include "bvh.hpp"
#include <span>
#include <optional>
#include <stdexcept>
#include <vector>
#include <unordered_set>
#include <glad/glad.h>
#include <experimental/generator>

namespace myrt
{
    struct geometry_info_t
    {
        using index_type = detail::default_index_type;

        index_type bvh_node_base_index;
        index_type bvh_index_base_index;
        index_type indices_base_index;
        index_type points_base_index;
    };

    struct material_info_t
    {
        rnu::vec4ui8 albedo_rgba = rnu::vec4ui8(255, 255, 255, 255);
        rnu::vec4ui8 alt_color_rgba = rnu::vec4ui8(255, 255, 255, 255);
        float ior = 1.0f;
        float brightness = 1.0f;
    };

    struct geometry_t;
    struct material_t;

    class scene
    {
    public:
        using geometry_pointer = std::shared_ptr<geometry_t>;
        using material_pointer = std::shared_ptr<material_t>;

        constexpr static unsigned buffer_binding_bvh_nodes = 0;
        constexpr static unsigned buffer_binding_bvh_indices = 1;
        constexpr static unsigned buffer_binding_indices = 2;
        constexpr static unsigned buffer_binding_vertices = 3;
        constexpr static unsigned buffer_binding_normals = 4;
        constexpr static unsigned buffer_binding_drawables = 5;

        constexpr static unsigned buffer_binding_global_bvh_nodes = 6;
        constexpr static unsigned buffer_binding_global_bvh_indices = 7;
        constexpr static unsigned buffer_binding_materials = 8;

        using index_type = detail::default_index_type;
        using point_type = detail::default_point_type;

        scene();
        ~scene();
        [[nodiscard]] const geometry_pointer& push_geometry(
            std::span<index_type const> indices,
            std::span<point_type const> points,
            std::span<point_type const> normals
        );
        [[nodiscard]] const material_pointer& push_material(material_info_t info);

        void erase_geometry_direct(const geometry_pointer& geometry);
        void erase_geometry_indirect(const geometry_pointer& geometry);
        void enqueue(geometry_t const* geometry, material_t const* material, rnu::mat4 const& transformation);
        void enqueue(const geometry_pointer& geometry, const material_pointer& material, rnu::mat4 const& transformation);
        bool prepare_and_bind();

        struct hit {
            size_t index;
            float t;
        };
        std::optional<hit> pick(ray_t ray);

        const material_pointer& default_material() const;

    private:
        bool prepare();
        struct drawable_geometry_t
        {
            rnu::mat4 transformation;
            rnu::mat4 inverse_transformation;
            geometry_info_t geometry_info;
            int material_index;
            int geometry_index;
            int pad[2];
        };
        struct aligned_point_t
        {
            constexpr aligned_point_t(point_type point) : value(point) {}
            alignas(4 * sizeof(float)) point_type value;
        };

        struct
        {
            GLuint indices_buffer = 0;
            GLuint vertices_buffer = 0;
            GLuint normals_buffer = 0;
            GLuint bvh_nodes_buffer = 0;
            GLuint bvh_indices_buffer = 0;
            GLuint drawable_buffer = 0;
            GLuint materials_buffer = 0;

            GLuint global_bvh_nodes_buffer = 0;
            GLuint global_bvh_indices_buffer = 0;
        } m_gl_objects;

        size_t m_last_drawable_hash = 0;
        size_t m_current_drawable_hash = ~0;

        std::vector<index_type> m_indices;
        std::vector<aligned_point_t> m_vertices;
        std::vector<aligned_point_t> m_normals;
        std::vector<aligned_node_t> m_bvh_nodes;
        std::vector<index_type> m_bvh_indices;
        std::vector<drawable_geometry_t> m_drawables;
        std::vector<aabb_t> m_drawable_aabbs;
        std::vector<geometry_pointer> m_available_geometries;
        std::vector<geometry_pointer> m_erase_on_prepare;
        std::vector<material_info_t> m_material_infos;
        std::vector<material_pointer> m_available_materials;
        std::vector<std::unique_ptr<bvh>> m_object_bvhs;
        std::unique_ptr<bvh> m_global_bvh;
        material_pointer m_default_material;
        bool m_materials_changed = false;
        bool m_geometries_changed = false;
    };

    struct geometric_object
    {
        scene::geometry_pointer geometry;
        scene::material_pointer material;
        rnu::mat4 transformation;

        void enqueue() const;
    };
}