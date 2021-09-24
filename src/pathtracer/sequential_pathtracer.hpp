#pragma once
#include <rnu/math/math.hpp>
#include <random>
#include "texture_provider.hpp"
#include "scene.hpp"

namespace myrt {
  enum class shader_flags : std::uint8_t
  {
    none = 0,
    generate = 1,
    trace = 2,
    color = 4,
    filter = 8,
    all = 0xff
  };

  constexpr shader_flags operator|(shader_flags lhs, shader_flags rhs) { return shader_flags(std::uint8_t(lhs) | std::uint8_t(rhs)); }
  constexpr shader_flags operator&(shader_flags lhs, shader_flags rhs) { return shader_flags(std::uint8_t(lhs) & std::uint8_t(rhs)); }
  constexpr shader_flags operator^(shader_flags lhs, shader_flags rhs) { return shader_flags(std::uint8_t(lhs) ^ std::uint8_t(rhs)); }

  class sequential_pathtracer {
  public:
    static constexpr std::uint32_t linear_group_size = 64;

    std::shared_ptr<texture_t> run(texture_provider_t& provider, scene& scene, int width, int height);

    void set_view_matrix(rnu::mat4 const& matrix);
    void set_projection_matrix(rnu::mat4 const& matrix);
    void set_dof_enabled(bool enable);
    void set_lens_size(rnu::vec2 lens_size);
    void set_focus(float focus);
    void set_bokeh_mask(std::uint32_t bokeh_texture);
    void set_cubemap(std::uint32_t map, std::uint32_t sampler);
    void set_num_bounces(int num_bounces);
    void set_sdf_marching_steps(float steps);
    void set_sdf_marching_epsilon(float eps);

    rnu::mat4 const& get_view_matrix();
    rnu::mat4 const& get_projection_matrix();
    bool get_dof_enabled();
    rnu::vec2 get_lens_size();
    float get_focus();
    std::uint32_t get_bokeh_mask();
    std::pair<std::uint32_t, std::uint32_t> get_cubemap();
    int get_num_bounces();
    float get_sdf_marching_steps();
    float get_sdf_marching_epsilon();

    std::uint32_t sample_count() const;

    void invalidate_shaders();
    void invalidate_shader(shader_flags which);
    void invalidate_texture();
    void invalidate_counter();

    std::uint32_t color_texture_id() const;
    std::uint32_t debug_texture_id() const;

  private:
    struct generate_output_t {
      rnu::vec3 origin;
      float random_value;
      rnu::vec3 direction;
      float pixel_x;
      rnu::vec3 reflectance;
      float pixel_y;
      rnu::vec4  color;
      float pdf;
      float pad[3];
    };

    struct trace_output_t {
      rnu::vec3 point;
      std::int32_t intersects;
      rnu::vec3 normal;
      std::uint32_t generated_ray;

      rnu::vec2 uv;
      std::int32_t material_index;

      rnu::vec4ui8 light_color;
      rnu::vec3 light_direction;
      float light_strength;
    };

    struct filter_access_t {
      std::uint32_t num_elements;
      std::uint32_t num_groups_x;
      std::uint32_t num_groups_y;
      std::uint32_t num_groups_z;
    };

    void create_generate_shader();
    void load_generate_bindings();
    void pass_generate();

    void pass_trace(scene& scene);
    void create_trace_shader(scene& scene);
    void load_trace_bindings();

    void pass_filter();
    void create_filter_shader();
    void load_filter_bindings();

    void pass_color(bool force_write, int bounce);
    void create_color_shader();
    void load_color_bindings();

    std::uint32_t m_generate_shader = 0;
    std::uint32_t m_trace_shader = 0;
    std::uint32_t m_resample_shader = 0;
    std::uint32_t m_ray_filter_shader = 0;
    std::uint32_t m_color_shader = 0;
    std::uint32_t m_generate_buffer = 0;
    std::uint32_t m_trace_buffer = 0;
    std::uint32_t m_filter_buffer = 0;
    std::uint32_t m_filter_control_buffer = 0;
    std::uint32_t m_filter_control_buffer_target = 0;

    texture_provider_t* m_texture_provider;

    rnu::mat4 m_camera_view;
    rnu::mat4 m_camera_projection;
    rnu::mat4 m_inverse_camera_view;
    rnu::mat4 m_inverse_camera_projection;
    rnu::vec2ui m_image_size;
    std::vector<float> m_random_texture_data;
    std::shared_ptr<texture_t> m_random_texture;

    bool m_dof_active = false;
    rnu::vec2 m_lens_size = { 10, 10 };
    float m_focus = 10.0f;
    int m_num_bounces = 10;
    float m_sdf_marching_steps = 400;
    float m_sdf_marching_epsilon = 1e-5;


    std::uint32_t m_bokeh_texture = 0;
    std::uint32_t m_cubemap = 0;
    std::uint32_t m_cubemap_sampler = 0;

    std::uint32_t m_sample_counter = 0;

    rnu::vec2i m_generate_group_sizes;

    std::mt19937 m_rng;

    std::shared_ptr<texture_t> m_color_texture;
    std::shared_ptr<texture_t> m_last_color_texture;

    std::shared_ptr<texture_t> m_debug_texture;

    scene* m_last_scene;

    struct {
      std::int32_t debug_texture;

      std::int32_t inverse_camera_view;
      std::int32_t inverse_camera_projection;
      std::int32_t dof_active;
      std::int32_t image_size;
      std::int32_t lens_size;
      std::int32_t focus;
      std::int32_t random_seed;

      std::int32_t random_texture;
      std::int32_t bokeh_shape;

      std::int32_t output_buffer;
    } m_generate_bindings;

    struct {
      std::int32_t access_control;
      std::int32_t input_buffer;
      std::int32_t output_buffer;

      std::int32_t bvh;
      std::int32_t bvh_indices;
      std::int32_t mesh_indices;
      std::int32_t mesh_uvs;
      std::int32_t mesh_points;
      std::int32_t mesh_normals;
      std::int32_t mesh_geometries;
      std::int32_t global_bvh;
      std::int32_t global_bvh_indices;
      std::int32_t sdf_data;
      std::int32_t sdf_drawables;
      std::int32_t sdf_marching_steps;
      std::int32_t sdf_marching_epsilon;

      std::int32_t random_sample;
      std::int32_t random_texture;
    } m_trace_bindings;

    struct {
      std::int32_t access_control;
      std::int32_t access_control_target;

      std::int32_t generate_buffer;
      std::int32_t trace_buffer;
      std::int32_t filter_buffer;
    } m_ray_filter_bindings;

    struct {
      std::int32_t materials;
      std::int32_t material_data;
      std::int32_t access_control;
      std::int32_t generate_output;
      std::int32_t trace_output;
      std::int32_t debug_texture;
      std::int32_t output_image;
      std::int32_t sample_index;
      std::int32_t force_write_color;
      std::int32_t random_texture;
      std::int32_t random_sample;
      std::int32_t cubemap;
      std::int32_t bounce_index;
    } m_color_bindings;
  };
}