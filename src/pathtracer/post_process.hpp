#pragma once
#include "texture_provider.hpp"
#include <rnu/math/math.hpp>

namespace myrt {
class denoise {
public:
  float exponent = 10;
  float strength = 0.05f;

  std::shared_ptr<texture_t> process(texture_provider_t& provider, std::uint32_t texture);

private:
  void create();
  void load_bindings();

  struct {
    std::int32_t input_image = 0;
    std::int32_t output_image = 0;
    std::int32_t strength = 0;
    std::int32_t exponent = 0;
  } m_denoiser_bindings;

  std::uint32_t m_input_sampler;
  rnu::vec2i m_denoiser_group_sizes;
  std::uint32_t m_denoiser;
};

class fxaa {
public:
  std::shared_ptr<texture_t> process(texture_provider_t& provider, std::uint32_t texture);

private:
  void create();
  void load_bindings();

  struct {
    std::int32_t input_image = 0;
    std::int32_t output_image = 0;
  } m_bindings;

  std::uint32_t m_input_sampler;
  rnu::vec2i m_group_sizes;
  std::uint32_t m_program;
};

class linear_scale {
public:
  float factor = 1.f;

  std::shared_ptr<texture_t> process(texture_provider_t& provider, std::uint32_t texture);

private:
  void create();

  std::uint32_t m_source;
  std::uint32_t m_framebuffer;
};

class cutoff {
public:
  rnu::vec4 value = {1, 1, 1, 1};
  bool leave_above = true;

  std::shared_ptr<texture_t> process(texture_provider_t& provider, std::uint32_t texture);

private:
  void create();
  void load_bindings();

  struct {
    std::int32_t input_image = 0;
    std::int32_t output_image = 0;
    std::int32_t value = 0;
    std::int32_t leave_above = 0;
  } m_bindings;

  std::uint32_t m_input_sampler;
  rnu::vec2i m_group_sizes;
  std::uint32_t m_program;
};

class blur {
public:
  enum class axis { x = 0, y = 1 } direction;
  float step = 1.25f;

  std::shared_ptr<texture_t> process(texture_provider_t& provider, std::uint32_t texture);

private:
  void create();
  void load_bindings();

  struct {
    std::int32_t input_image = 0;
    std::int32_t output_image = 0;
    std::int32_t direction = 0;
    std::int32_t step = 0;
  } m_bindings;

  std::uint32_t m_input_sampler;
  rnu::vec2i m_group_sizes;
  std::uint32_t m_program;
};

class blur2d {
public:
  float step = 1.25f;
  std::shared_ptr<texture_t> process(texture_provider_t& provider, std::uint32_t texture);

private:
  blur m_blur;
};

class tonemap {
public:
  std::shared_ptr<texture_t> process(texture_provider_t& provider, std::uint32_t texture);

private:
  void create();
  void load_bindings();

  struct {
    std::int32_t input_image = 0;
    std::int32_t output_image = 0;
    std::int32_t direction = 0;
  } m_bindings;

  std::uint32_t m_input_sampler;
  rnu::vec2i m_group_sizes;
  std::uint32_t m_program;
};

class add {
public:
  std::uint32_t overlay_texture = 0;
  float factor = 1;

  std::shared_ptr<texture_t> process(texture_provider_t& provider, std::uint32_t texture);

private:
  void create();
  void load_bindings();

  struct {
    std::int32_t input_image = 0;
    std::int32_t output_image = 0;
    std::int32_t overlay_texture = 0;
    std::int32_t factor = 0;
  } m_bindings;

  std::uint32_t m_input_sampler;
  rnu::vec2i m_group_sizes;
  std::uint32_t m_program;
};


class bloom {
public:
  float blur_step = 1.25f;
  float texture_scaling = 2.0f;
  float overlay_factor = 0.3f;

  std::shared_ptr<texture_t> process(texture_provider_t& provider, std::uint32_t texture);

private:
  cutoff m_cutoff;
  linear_scale m_scale;
  blur2d m_blur;
  add m_add;
};
} // namespace myrt