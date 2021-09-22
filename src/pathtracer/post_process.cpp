#include "post_process.hpp"
#include <glsp/glsp.hpp>
#include <myrt/sfml/utils.hpp>
#include "bindings.hpp"

namespace myrt
{
  const static std::filesystem::path res_dir = "../../../res";

  std::shared_ptr<texture_t> denoise::process(texture_provider_t& provider, std::uint32_t texture)
  {
    if (!glIsProgram(m_denoiser))
      create_denoiser();

    int width = 0;
    int height = 0;
    glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_WIDTH, &width);
    glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_HEIGHT, &height);
    auto dst = provider.get(GL_TEXTURE_2D, GL_RGBA16F, width, height, 1);

    glUseProgram(m_denoiser);
    glBindTextureUnit(m_denoiser_bindings.input_image, texture);
    glBindImageTexture(m_denoiser_bindings.output_image, dst->id(), 0, false, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glUniform1f(m_denoiser_bindings.exponent, exponent);
    glUniform1f(m_denoiser_bindings.strength, strength);

    auto const count_x = (width + m_denoiser_group_sizes.x - 1) / m_denoiser_group_sizes.x;
    auto const count_y = (height + m_denoiser_group_sizes.y - 1) / m_denoiser_group_sizes.y;
    glDispatchCompute(count_x, count_y, 1);
    glMemoryBarrierByRegion(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glMemoryBarrierByRegion(GL_TEXTURE_FETCH_BARRIER_BIT);
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT);

    return dst;
  }
  void denoise::create_denoiser()
  {
    glsp::preprocess_file_info generate_file;
    generate_file.file_path = res_dir / "../src/pt/pp_denoise.comp";
    auto const preprocessed_file = glsp::preprocess_file(generate_file);

    auto const program = make_program(preprocessed_file.contents);

    if (program) {
      if (glIsProgram(m_denoiser))
        glDeleteProgram(m_denoiser);

      m_denoiser = program.value();
      load_denoiser_bindings();
    }
  }
  void denoise::load_denoiser_bindings()
  {
    glUseProgram(m_denoiser);
    m_denoiser_bindings.input_image = if_empty(sampler_binding(m_denoiser, "input_image"));
    m_denoiser_bindings.output_image = if_empty(sampler_binding(m_denoiser, "output_image"));
    m_denoiser_bindings.exponent = if_empty(uniform_location(m_denoiser, "exponent"));
    m_denoiser_bindings.strength = if_empty(uniform_location(m_denoiser, "strength"));

    std::int32_t group_sizes[3]{};
    glGetProgramiv(m_denoiser, GL_COMPUTE_WORK_GROUP_SIZE, group_sizes);
    m_denoiser_group_sizes = { group_sizes[0], group_sizes[1] };
  }
}