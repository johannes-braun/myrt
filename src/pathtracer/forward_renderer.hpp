#pragma once
#include "scene.hpp"
#include "texture_provider.hpp"
#include <random>

namespace myrt
{
  class forward_renderer 
  {
    struct multi_draw_indirect_t
    {
      using uint = std::uint32_t;
      uint count;
      uint instances;
      uint first_index;
      uint first_vertex;
      uint first_instance;

      uint pad[3];
    };
  public:
    std::shared_ptr<texture_t> run(texture_provider_t& provider, scene& scene, int width, int height);
    void set_view_matrix(rnu::mat4 const& matrix);
    void set_projection_matrix(rnu::mat4 const& matrix);

    void reload_resolve_shader();
    void reload_render_shader();

  private:
    void create_gen_multidraw_shader();
    void load_gen_multidraw_bindings();
    void pass_gen_multidraw(scene& scene);

    void create_render_shader();
    void load_render_bindings();
    void pass_render(scene& scene);

    void create_resolve_shader(scene& scene);
    void load_resolve_bindings();
    void pass_resolve(scene& scene);

    struct {
      std::int32_t geometries;
      std::int32_t multidraws;
    } m_gen_multidraw_bindings;

    std::uint32_t m_gen_multidraw_group_size = 1;
    std::uint32_t m_gen_multidraw = 0;

    size_t m_num_multidraws = 0;

    std::uint32_t m_multidraw_buffer = 0;

    rnu::mat4 m_view;
    rnu::mat4 m_projection;

    std::uint32_t m_framebuffer;
    std::uint32_t m_vertex_array;
    std::uint32_t m_resolve_vertex_array;

    std::shared_ptr<texture_t> m_gbuffer_texture;
    std::shared_ptr<texture_t> m_result_texture;
    std::shared_ptr<texture_t> m_depth_texture;
    rnu::vec2ui m_image_size;

    std::uniform_int_distribution<int> m_dist = std::uniform_int_distribution<int>(0, std::numeric_limits<int>::max());
    std::mt19937 m_rng;
    
    std::uint32_t m_resolve_framebuffer;
    struct {
      std::int32_t gbuffer;
      std::int32_t view;
      std::int32_t proj;
      std::int32_t materials;
      std::int32_t material_data;
      std::int32_t random_seed;
    } m_resolve_bindings;
    std::uint32_t m_resolve_program;

    struct {
      std::int32_t geometries;
      std::int32_t points_in;
      std::int32_t normal_in;
      std::int32_t uv_in;
      std::int32_t view;
      std::int32_t proj;
    } m_render_bindings;

    std::uint32_t m_render;
  };
}