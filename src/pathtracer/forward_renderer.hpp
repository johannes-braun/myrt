#pragma once

#include "scene.hpp"

namespace myrt
{
  class forward_renderer 
  {
  public:
    void set_projection(rnu::mat4 matrix);
    void set_view(rnu::mat4 matrix);
    void set_cubemap(std::optional<cubemap_texture> cubemap);
    void set_sdf_marching_steps(int steps);
    void set_sdf_marching_epsilon(float eps);
    void reload_shaders(scene& scene);

  private:
    rnu::mat4 m_view_matrix;
    rnu::mat4 m_projection_matrix;
    int m_sdf_marching_steps = 400;
    float m_sdf_marching_epsilon = 0.75e-6f;
  };
}