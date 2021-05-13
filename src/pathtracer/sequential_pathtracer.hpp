#pragma once
#include <rnu/math/math.hpp>
#include "../glad/glad.h"
#include <random>
#include "texture_provider.hpp"
#include "scene.hpp"

namespace myrt {
  class sequential_pathtracer {
  public:
    static constexpr std::uint32_t linear_group_size = 64;

    void run(scene& scene, int width, int height);

    void set_view_matrix(rnu::mat4 const& matrix);
    void set_projection_matrix(rnu::mat4 const& matrix);
    void set_dof_enabled(bool enable);
    void set_lens_size(rnu::vec2 lens_size);
    void set_focus(float focus);
    void set_bokeh_mask(GLuint bokeh_texture);
    void set_cubemap(GLuint map, GLuint sampler);

    std::uint32_t sample_count() const;

    void invalidate_texture();
    void invalidate_counter();

    GLuint color_texture_id() const;
    GLuint debug_texture_id() const;

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

    void pass_color(bool force_write);
    void create_color_shader();
    void load_color_bindings();

    GLuint m_generate_shader = 0;
    GLuint m_trace_shader = 0;
    GLuint m_resample_shader = 0;
    GLuint m_ray_filter_shader = 0;
    GLuint m_color_shader = 0;
    GLuint m_generate_buffer = 0;
    GLuint m_trace_buffer = 0;
    GLuint m_filter_buffer = 0;
    GLuint m_filter_control_buffer = 0;
    GLuint m_filter_control_buffer_target = 0;

    texture_provider_t m_texture_provider;

    rnu::mat4 m_camera_view;
    rnu::mat4 m_camera_projection;
    rnu::mat4 m_inverse_camera_view;
    rnu::mat4 m_inverse_camera_projection;
    bool m_dof_active = false;
    rnu::vec2ui m_image_size;
    std::vector<float> m_random_texture_data;
    std::shared_ptr<texture_t> m_random_texture;
    rnu::vec2 m_lens_size = { 10, 10 };
    float m_focus = 10.0f;

    GLuint m_bokeh_texture = 0;
    GLuint m_cubemap = 0;
    GLuint m_cubemap_sampler = 0;

    std::uint32_t m_sample_counter = 0;

    rnu::vec2i m_generate_group_sizes;

    std::mt19937 m_rng;

    std::shared_ptr<texture_t> m_color_texture;
    std::shared_ptr<texture_t> m_last_color_texture;

    std::shared_ptr<texture_t> m_debug_texture;

    scene* m_last_scene;

    struct {
      GLint debug_texture;

      GLint inverse_camera_view;
      GLint inverse_camera_projection;
      GLint dof_active;
      GLint image_size;
      GLint lens_size;
      GLint focus;
      GLint random_seed;

      GLint random_texture;
      GLint bokeh_shape;

      GLint output_buffer;
    } m_generate_bindings;

    struct {
      GLint access_control;
      GLint input_buffer;
      GLint output_buffer;

      GLint bvh;
      GLint bvh_indices;
      GLint mesh_indices;
      GLint mesh_points;
      GLint mesh_normals;
      GLint mesh_geometries;
      GLint global_bvh;
      GLint global_bvh_indices;
      GLint sdf_data;
      GLint sdf_drawables;
      GLint sdf_marching_steps;
      GLint sdf_marching_epsilon;

      GLint random_sample;
      GLint random_texture;
    } m_trace_bindings;

    struct {
      GLint access_control;
      GLint access_control_target;

      GLint generate_buffer;
      GLint trace_buffer;
      GLint filter_buffer;
    } m_ray_filter_bindings;

    struct {
      GLint access_control;
      GLint generate_output;
      GLint trace_output;
      GLint debug_texture;
      GLint output_image;
      GLint sample_index;
      GLint force_write_color;
      GLint random_texture;
      GLint random_sample;
      GLint cubemap;
    } m_color_bindings;
  };
}