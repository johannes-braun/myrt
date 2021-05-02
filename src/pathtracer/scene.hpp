#pragma once

#include "bvh.hpp"
#include <span>
#include <optional>
#include <stdexcept>
#include <vector>
#include <unordered_set>
#include <glad/glad.h>
#include <experimental/generator>
#include "sdf.hpp"

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

    struct sdf_info_t {
      using id_type = detail::default_index_type;

      std::shared_ptr<sdf::instruction> root;
    };

    struct material_info_t
    {
        rnu::vec4ui8 albedo_rgba = rnu::vec4ui8(255, 255, 255, 255);
        rnu::vec4ui8 alt_color_rgba = rnu::vec4ui8(255, 255, 255, 255);
        float ior = 1.0f;
        float roughness = 1.0f;
        float metallic = 0.0f;
        float transmission = 0.0f;
        float emission = 0.0f;

        int aaa[1];
    };

    struct geometry_t;
    struct material_t;
    struct sdf_t;
    class scene;

    struct geometric_object
    {
      std::string name;
      std::shared_ptr<geometry_t> geometry;
      std::shared_ptr<material_t> material;
      rnu::mat4 transformation;
      bool show = true;

      void enqueue() const;
    };

    struct sdf_object 
    {
      std::string name;
      std::shared_ptr<sdf_t> sdf;
      rnu::mat4 transformation;
      bool show = true;

      template<typename T>
      void set(std::shared_ptr<sdf::parameter> const& parameter, T&& value);
      void enqueue() const;

    private:
      scene* get_scene();
    };

    class scene
    {
    public:
      friend struct sdf_object;

        using geometry_pointer = std::shared_ptr<geometry_t>;
        using material_pointer = std::shared_ptr<material_t>;
        using sdf_pointer = std::shared_ptr<sdf_t>;

        struct prepare_result_t
        {
          bool materials_changed = false;
          bool geometries_changed = false;
          bool drawables_changed = false;
          bool sdfs_changed = false;
          bool sdf_buffer_changed = false;
        };

        constexpr static unsigned buffer_binding_bvh_nodes = 0;
        constexpr static unsigned buffer_binding_bvh_indices = 1;
        constexpr static unsigned buffer_binding_indices = 2;
        constexpr static unsigned buffer_binding_vertices = 3;
        constexpr static unsigned buffer_binding_normals = 4;
        constexpr static unsigned buffer_binding_drawables = 5;
        constexpr static unsigned buffer_binding_sdf_data = 9;
        constexpr static unsigned buffer_binding_sdf_drawable = 10;

        constexpr static unsigned buffer_binding_global_bvh_nodes = 6;
        constexpr static unsigned buffer_binding_global_bvh_indices = 7;
        constexpr static unsigned buffer_binding_materials = 8;

        using index_type = detail::default_index_type;
        using point_type = detail::default_point_type;

        scene();
        ~scene();
        [[nodiscard]] geometry_pointer push_geometry(
            std::span<index_type const> indices,
            std::span<point_type const> points,
            std::span<point_type const> normals
        );
        [[nodiscard]] material_pointer push_material(material_info_t info);
        [[nodiscard]] sdf_pointer push_sdf(sdf_info_t info);

        void enqueue(geometry_t const* geometry, material_t const* material, rnu::mat4 const& transformation);
        void enqueue(const geometry_pointer& geometry, const material_pointer& material, rnu::mat4 const& transformation);

        void enqueue(sdf_t const* geometry, rnu::mat4 const& transformation);
        void enqueue(const sdf_pointer& geometry, rnu::mat4 const& transformation);

        prepare_result_t prepare_and_bind();

        [[nodiscard]] material_info_t info_of(material_pointer const& material) const;
        void update_material(material_pointer const& material, const material_info_t& info);

        void set_material_parameter(const sdf_pointer& sdf, std::shared_ptr<sdf::int_param> const& parameter, material_pointer material);

        template<typename T>
        void set_parameter(const sdf_pointer& sdf, std::shared_ptr<sdf::parameter> const& parameter, T&& value);
        template<typename T>
        void set_parameter(const sdf_t* sdf, std::shared_ptr<sdf::parameter> const& parameter, T&& value);

        void set_parameter(const sdf_pointer& sdf, std::shared_ptr<sdf::parameter> const& parameter, float* value_ptr);

        struct hit {
            size_t index;
            float t;
        };
        std::optional<hit> pick(ray_t ray);

        const material_pointer& default_material() const;

        [[nodiscard]] decltype(auto) materials() const noexcept { return m_available_materials; }
        [[nodiscard]] decltype(auto) sdfs() const noexcept { return m_available_sdfs; }

        sdf::glsl_assembler const& get_sdf_assembler() {
          return m_sdf_assembler;
        }

    private:
      sdf::glsl_assembly& get_sdf_assembly(sdf_t* sdf);
      sdf::glsl_assembly const& get_sdf_assembly(const sdf_t* sdf);

        void erase_geometry_direct(const geometry_pointer& geometry);
        void erase_geometry_indirect(const geometry_pointer& geometry);
        void erase_material_direct(const material_pointer& material);
        void erase_material_indirect(const material_pointer& material);

        prepare_result_t prepare();
        struct drawable_geometry_t
        {
            rnu::mat4 transformation;
            rnu::mat4 inverse_transformation;
            geometry_info_t geometry_info;
            int material_index;
            int geometry_index;
            int pad[2];
        };
        struct drawable_sdf_t
        {
          rnu::mat4 transformation;
          rnu::mat4 inverse_transformation;
          int sdf_index;
          int pad[3];
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
            GLuint sdf_data_buffer = 0;
            GLuint sdf_drawable_buffer = 0;

            GLuint global_bvh_nodes_buffer = 0;
            GLuint global_bvh_indices_buffer = 0;
        } m_gl_objects;

        size_t m_last_drawable_hash = 0;
        size_t m_current_drawable_hash = ~0;
        size_t m_last_sdf_drawable_hash = ~0;
        size_t m_current_sdf_drawable_hash = ~0;

        std::vector<drawable_geometry_t> m_drawables;
        std::vector<aabb_t> m_drawable_aabbs;
         
        std::vector<geometry_pointer> m_available_geometries;
        std::vector<material_pointer> m_available_materials;
        std::vector<sdf_pointer> m_available_sdfs;
        std::vector<std::unique_ptr<bvh>> m_object_bvhs;
        std::vector<geometry_pointer> m_erase_on_prepare;
        std::vector<material_pointer> m_erase_on_prepare_materials;

        std::vector<index_type> m_indices;
        std::vector<aligned_point_t> m_vertices;
        std::vector<aligned_point_t> m_normals;
        std::vector<aligned_node_t> m_bvh_nodes;
        std::vector<index_type> m_bvh_indices;
        std::vector<material_info_t> m_material_infos;
        std::vector<float> m_sdf_parameter_buffer;
        std::vector<drawable_sdf_t> m_sdf_drawables;

        sdf::glsl_assembler m_sdf_assembler;
        std::unique_ptr<bvh> m_global_bvh;
        material_pointer m_default_material;
        bool m_materials_changed = false;
        bool m_geometries_changed = false;
        bool m_sdfs_changed = false;
        bool m_sdf_buffer_changed = false;
    };

    template<typename T>
    inline void scene::set_parameter(const sdf_pointer& sdf, std::shared_ptr<sdf::parameter> const& parameter, T&& value)
    {
      set_parameter(sdf.get(), parameter, std::forward<T>(value));
    }
    template<typename T>
    inline void scene::set_parameter(const sdf_t* sdf, std::shared_ptr<sdf::parameter> const& parameter, T&& value)
    {
      m_sdf_buffer_changed = true;
      get_sdf_assembly(sdf).set_value(m_sdf_parameter_buffer.data(), parameter, std::forward<T>(value));
    }

    template<typename T>
    void sdf_object::set(std::shared_ptr<detail::sdf_parameter> const& parameter, T&& value)
    {
      get_scene()->set_parameter(sdf, parameter, std::forward<T>(value));
    }
}