#pragma once
#include "texture_provider.hpp"
#include <rnu/math/math.hpp>
#include <optional>

namespace myrt {
class basic_post_process {
public:
  std::shared_ptr<texture_t> process(texture_provider_t& provider, std::uint32_t texture);
  std::uint32_t sampler() const noexcept;

protected:
  virtual void on_apply_bindings(std::uint32_t program){};
  virtual std::optional<std::uint32_t> on_create_program() = 0;
  virtual void on_load_bindings(std::uint32_t program){};

private:
  void create_program();
  void load_bindings();

  struct {
    std::int32_t input_image = 0;
    std::int32_t output_image = 0;
  } m_base_bindings;

  rnu::vec2i m_group_sizes;
  std::uint32_t m_program;
  std::uint32_t m_sampler;
};

class denoise : public basic_post_process {
public:
  float exponent = 10;
  float strength = 0.05f;

private:
  virtual void on_apply_bindings(std::uint32_t program) override;
  virtual std::optional<std::uint32_t> on_create_program() override;
  virtual void on_load_bindings(std::uint32_t program) override;

  struct {
    std::int32_t strength = 0;
    std::int32_t exponent = 0;
  } m_bindings;
};

class fxaa : public basic_post_process {
private:
  std::optional<std::uint32_t> on_create_program() final;
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

class cutoff : public basic_post_process {
public:
  rnu::vec4 value = {1, 1, 1, 1};
  bool leave_above = true;

private:
  virtual void on_apply_bindings(std::uint32_t program) override;
  virtual std::optional<std::uint32_t> on_create_program() override;
  virtual void on_load_bindings(std::uint32_t program) override;
  struct {
    std::int32_t value = 0;
    std::int32_t leave_above = 0;
  } m_bindings;
};

class blur : public basic_post_process {
public:
  enum class axis { x = 0, y = 1 } direction;
  float step = 1.25f;

private:
  virtual void on_apply_bindings(std::uint32_t program) override;
  virtual std::optional<std::uint32_t> on_create_program() override;
  virtual void on_load_bindings(std::uint32_t program) override;

  struct {
    std::int32_t direction = 0;
    std::int32_t step = 0;
  } m_bindings;
};

class blur2d {
public:
  float step = 1.25f;
  std::shared_ptr<texture_t> process(texture_provider_t& provider, std::uint32_t texture);

private:
  blur m_blur;
};

class tonemap : public basic_post_process {
private:
  virtual std::optional<std::uint32_t> on_create_program() override;
};

class add : public basic_post_process {
public:
  std::uint32_t overlay_texture = 0;
  float factor = 1;

private:
  virtual void on_apply_bindings(std::uint32_t program) override;
  virtual std::optional<std::uint32_t> on_create_program() override;
  virtual void on_load_bindings(std::uint32_t program) override;

  struct {
    std::int32_t overlay_texture = 0;
    std::int32_t factor = 0;
  } m_bindings;
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