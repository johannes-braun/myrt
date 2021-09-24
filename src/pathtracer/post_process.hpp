#pragma once
#include "texture_provider.hpp"
#include <rnu/math/math.hpp>

namespace myrt
{
  class denoise
  {
  public:
    float exponent = 10;
    float strength = 0.05f;

    std::shared_ptr<texture_t> process(texture_provider_t& provider, std::uint32_t texture);

  private:
    void create_denoiser();
    void load_denoiser_bindings();

    struct {
      std::int32_t input_image = 0;
      std::int32_t output_image = 0;
      std::int32_t strength = 0;
      std::int32_t exponent = 0;
    } m_denoiser_bindings;

    rnu::vec2i m_denoiser_group_sizes;
    std::uint32_t m_denoiser;
  };
}