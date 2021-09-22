#pragma once
#include "scene.hpp"

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
    void run(scene& scene, int width, int height);
    void set_view_matrix(rnu::mat4 const& matrix);
    void set_projection_matrix(rnu::mat4 const& matrix);

  private:
    void create_gen_multidraw_shader();
    void load_gen_multidraw_bindings();
    void pass_gen_multidraw(scene& scene);

    void create_render_shader(scene& scene);
    void load_render_bindings();
    void pass_render(scene& scene);

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

    std::uint32_t m_vertex_array;

    struct {
      std::int32_t geometries;
      std::int32_t materials;
      std::int32_t material_data;
      std::int32_t points_in;
      std::int32_t normal_in;
      std::int32_t uv_in;
      std::int32_t view;
      std::int32_t proj;
    } m_render_bindings;
    std::uint32_t m_render;
  };
}