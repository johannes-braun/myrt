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
      create();

    int width = 0;
    int height = 0;
    glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_WIDTH, &width);
    glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_HEIGHT, &height);
    auto dst = provider.get(GL_TEXTURE_2D, GL_RGBA16F, width, height, 1);

    glUseProgram(m_denoiser);
    glBindTextureUnit(m_denoiser_bindings.input_image, texture);
    glBindSampler(m_denoiser_bindings.input_image, m_input_sampler);
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
  void denoise::create() {
    glsp::preprocess_file_info generate_file;
    generate_file.file_path = res_dir / "../src/pt/pp_denoise.comp";
    auto const preprocessed_file = glsp::preprocess_file(generate_file);

    auto const program = make_program(preprocessed_file.contents);

    if (program) {
      if (glIsProgram(m_denoiser))
        glDeleteProgram(m_denoiser);

      m_denoiser = program.value();
      load_bindings();
    }
    if (glIsSampler(m_input_sampler))
      glDeleteSamplers(1, &m_input_sampler);

    glCreateSamplers(1, &m_input_sampler);
    glSamplerParameterf(m_input_sampler, GL_TEXTURE_MAX_ANISOTROPY, 16.f);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_WRAP_R, GL_MIRRORED_REPEAT);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

  }
  void denoise::load_bindings()
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
 
  std::shared_ptr<texture_t> fxaa::process(texture_provider_t& provider, std::uint32_t texture) {
    if (!glIsProgram(m_program))
      create();

    int width = 0;
    int height = 0;
    glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_WIDTH, &width);
    glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_HEIGHT, &height);
    auto dst = provider.get(GL_TEXTURE_2D, GL_RGBA16F, width, height, 1);

    glUseProgram(m_program);
    glBindTextureUnit(m_bindings.input_image, texture);
    glBindSampler(m_bindings.input_image, m_input_sampler);
    glBindImageTexture(m_bindings.output_image, dst->id(), 0, false, 0, GL_WRITE_ONLY, GL_RGBA16F);

    auto const count_x = (width + m_group_sizes.x - 1) / m_group_sizes.x;
    auto const count_y = (height + m_group_sizes.y - 1) / m_group_sizes.y;
    glDispatchCompute(count_x, count_y, 1);
    glMemoryBarrierByRegion(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glMemoryBarrierByRegion(GL_TEXTURE_FETCH_BARRIER_BIT);
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT);

    return dst;
  }
  void fxaa::create() {
    glsp::preprocess_file_info generate_file;
    generate_file.file_path = res_dir / "../src/pt/pp_fxaa.comp";
    auto const preprocessed_file = glsp::preprocess_file(generate_file);

    auto const program = make_program(preprocessed_file.contents);

    if (program) {
      if (glIsProgram(m_program))
        glDeleteProgram(m_program);

      m_program = program.value();
      load_bindings();
    }
    if (glIsSampler(m_input_sampler))
      glDeleteSamplers(1, &m_input_sampler);

    glCreateSamplers(1, &m_input_sampler);
    glSamplerParameterf(m_input_sampler, GL_TEXTURE_MAX_ANISOTROPY, 16.f);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_WRAP_R, GL_MIRRORED_REPEAT);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
  }
  void fxaa::load_bindings() {
    glUseProgram(m_program);
    m_bindings.input_image = if_empty(sampler_binding(m_program, "input_image"));
    m_bindings.output_image = if_empty(sampler_binding(m_program, "output_image"));

    std::int32_t group_sizes[3]{};
    glGetProgramiv(m_program, GL_COMPUTE_WORK_GROUP_SIZE, group_sizes);
    m_group_sizes = {group_sizes[0], group_sizes[1]};
  }

  
  std::shared_ptr<texture_t> blur::process(texture_provider_t& provider, std::uint32_t texture) {
    if (!glIsProgram(m_program))
      create();

    int width = 0;
    int height = 0;
    glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_WIDTH, &width);
    glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_HEIGHT, &height);
    auto dst = provider.get(GL_TEXTURE_2D, GL_RGBA16F, width, height, 1);

    glUseProgram(m_program);
    glUniform1i(m_bindings.direction, int(direction));
    glUniform1f(m_bindings.step, step);
    glBindTextureUnit(m_bindings.input_image, texture);
    glBindSampler(m_bindings.input_image, m_input_sampler);
    glBindImageTexture(m_bindings.output_image, dst->id(), 0, false, 0, GL_WRITE_ONLY, GL_RGBA16F);

    auto const count_x = (width + m_group_sizes.x - 1) / m_group_sizes.x;
    auto const count_y = (height + m_group_sizes.y - 1) / m_group_sizes.y;
    glDispatchCompute(count_x, count_y, 1);
    glMemoryBarrierByRegion(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glMemoryBarrierByRegion(GL_TEXTURE_FETCH_BARRIER_BIT);
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT);

    return dst;
  }
  void blur::create() {
    glsp::preprocess_file_info generate_file;
    generate_file.file_path = res_dir / "../src/pt/pp_blur.comp";
    auto const preprocessed_file = glsp::preprocess_file(generate_file);

    auto const program = make_program(preprocessed_file.contents);

    if (program) {
      if (glIsProgram(m_program))
        glDeleteProgram(m_program);

      m_program = program.value();
      load_bindings();
    }
    if (glIsSampler(m_input_sampler))
      glDeleteSamplers(1, &m_input_sampler);

    glCreateSamplers(1, &m_input_sampler);
    glSamplerParameterf(m_input_sampler, GL_TEXTURE_MAX_ANISOTROPY, 16.f);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_WRAP_R, GL_MIRRORED_REPEAT);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
  }
  void blur::load_bindings() {
    glUseProgram(m_program);
    m_bindings.input_image = if_empty(sampler_binding(m_program, "input_image"));
    m_bindings.output_image = if_empty(sampler_binding(m_program, "output_image"));
    m_bindings.direction = if_empty(uniform_location(m_program, "direction"));
    m_bindings.step = if_empty(uniform_location(m_program, "step"));

    std::int32_t group_sizes[3]{};
    glGetProgramiv(m_program, GL_COMPUTE_WORK_GROUP_SIZE, group_sizes);
    m_group_sizes = {group_sizes[0], group_sizes[1]};
  }

  std::shared_ptr<texture_t> linear_scale::process(texture_provider_t& provider, std::uint32_t texture) {
    if (!glIsFramebuffer(m_framebuffer))
      create();

    int width = 0;
    int height = 0;
    glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_WIDTH, &width);
    glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_HEIGHT, &height);
    auto dst = provider.get(GL_TEXTURE_2D, GL_RGBA16F, width * factor, height * factor, 1);

    glNamedFramebufferTexture(m_framebuffer, GL_COLOR_ATTACHMENT0, dst->id(), 0);
    glNamedFramebufferTexture(m_source, GL_COLOR_ATTACHMENT0, texture, 0);
    glBlitNamedFramebuffer(m_source, m_framebuffer, 0, 0, width, height, 0, 0, width * factor, height * factor,
        GL_COLOR_BUFFER_BIT, GL_LINEAR);

    return dst;
  }

  void linear_scale::create() {
    if (!glIsFramebuffer(m_framebuffer))
      glCreateFramebuffers(1, &m_framebuffer);
    if (!glIsFramebuffer(m_source))
      glCreateFramebuffers(1, &m_source);
  }

 
  
  std::shared_ptr<texture_t> cutoff::process(texture_provider_t& provider, std::uint32_t texture) {
    if (!glIsProgram(m_program))
      create();

    int width = 0;
    int height = 0;
    glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_WIDTH, &width);
    glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_HEIGHT, &height);
    auto dst = provider.get(GL_TEXTURE_2D, GL_RGBA16F, width, height, 1);

    glUseProgram(m_program);
    glUniform1i(m_bindings.leave_above, leave_above);
    glUniform4fv(m_bindings.value, 1, value.data());
    glBindTextureUnit(m_bindings.input_image, texture);
    glBindSampler(m_bindings.input_image, m_input_sampler);
    glBindImageTexture(m_bindings.output_image, dst->id(), 0, false, 0, GL_WRITE_ONLY, GL_RGBA16F);

    auto const count_x = (width + m_group_sizes.x - 1) / m_group_sizes.x;
    auto const count_y = (height + m_group_sizes.y - 1) / m_group_sizes.y;
    glDispatchCompute(count_x, count_y, 1);
    glMemoryBarrierByRegion(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glMemoryBarrierByRegion(GL_TEXTURE_FETCH_BARRIER_BIT);
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT);

    return dst;
  }
  void cutoff::create() {
    glsp::preprocess_file_info generate_file;
    generate_file.file_path = res_dir / "../src/pt/pp_cutoff.comp";
    auto const preprocessed_file = glsp::preprocess_file(generate_file);

    auto const program = make_program(preprocessed_file.contents);

    if (program) {
      if (glIsProgram(m_program))
        glDeleteProgram(m_program);

      m_program = program.value();
      load_bindings();
    }
    if (glIsSampler(m_input_sampler))
      glDeleteSamplers(1, &m_input_sampler);

    glCreateSamplers(1, &m_input_sampler);
    glSamplerParameterf(m_input_sampler, GL_TEXTURE_MAX_ANISOTROPY, 16.f);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_WRAP_R, GL_MIRRORED_REPEAT);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
  }
  void cutoff::load_bindings() {
    glUseProgram(m_program);
    m_bindings.input_image = if_empty(sampler_binding(m_program, "input_image"));
    m_bindings.output_image = if_empty(sampler_binding(m_program, "output_image"));
    m_bindings.value = if_empty(uniform_location(m_program, "value"));
    m_bindings.leave_above = if_empty(uniform_location(m_program, "leave_above"));

    std::int32_t group_sizes[3]{};
    glGetProgramiv(m_program, GL_COMPUTE_WORK_GROUP_SIZE, group_sizes);
    m_group_sizes = {group_sizes[0], group_sizes[1]};
  }
 
  std::shared_ptr<texture_t> tonemap::process(texture_provider_t& provider, std::uint32_t texture) {
    if (!glIsProgram(m_program))
      create();

    int width = 0;
    int height = 0;
    glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_WIDTH, &width);
    glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_HEIGHT, &height);
    auto dst = provider.get(GL_TEXTURE_2D, GL_RGBA16F, width, height, 1);

    glUseProgram(m_program);
    glBindTextureUnit(m_bindings.input_image, texture);
    glBindSampler(m_bindings.input_image, m_input_sampler);
    glBindImageTexture(m_bindings.output_image, dst->id(), 0, false, 0, GL_WRITE_ONLY, GL_RGBA16F);

    auto const count_x = (width + m_group_sizes.x - 1) / m_group_sizes.x;
    auto const count_y = (height + m_group_sizes.y - 1) / m_group_sizes.y;
    glDispatchCompute(count_x, count_y, 1);
    glMemoryBarrierByRegion(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glMemoryBarrierByRegion(GL_TEXTURE_FETCH_BARRIER_BIT);
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT);

    return dst;
  }
  void tonemap::create() {
    glsp::preprocess_file_info generate_file;
    generate_file.file_path = res_dir / "../src/pt/pp_tonemap.comp";
    auto const preprocessed_file = glsp::preprocess_file(generate_file);

    auto const program = make_program(preprocessed_file.contents);

    if (program) {
      if (glIsProgram(m_program))
        glDeleteProgram(m_program);

      m_program = program.value();
      load_bindings();
    }
    if (glIsSampler(m_input_sampler))
      glDeleteSamplers(1, &m_input_sampler);

    glCreateSamplers(1, &m_input_sampler);
    glSamplerParameterf(m_input_sampler, GL_TEXTURE_MAX_ANISOTROPY, 16.f);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_WRAP_R, GL_MIRRORED_REPEAT);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
  }
  void tonemap::load_bindings() {
    glUseProgram(m_program);
    m_bindings.input_image = if_empty(sampler_binding(m_program, "input_image"));
    m_bindings.output_image = if_empty(sampler_binding(m_program, "output_image"));

    std::int32_t group_sizes[3]{};
    glGetProgramiv(m_program, GL_COMPUTE_WORK_GROUP_SIZE, group_sizes);
    m_group_sizes = {group_sizes[0], group_sizes[1]};
  }

  std::shared_ptr<texture_t> add::process(texture_provider_t& provider, std::uint32_t texture) {
    if (!glIsProgram(m_program))
      create();

    int width = 0;
    int height = 0;
    glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_WIDTH, &width);
    glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_HEIGHT, &height);
    auto dst = provider.get(GL_TEXTURE_2D, GL_RGBA16F, width, height, 1);

    glUseProgram(m_program);
    glUniform1f(m_bindings.factor, factor);
    glBindTextureUnit(m_bindings.input_image, texture);
    glBindSampler(m_bindings.input_image, m_input_sampler);
    glBindTextureUnit(m_bindings.overlay_texture, overlay_texture);
    glBindSampler(m_bindings.overlay_texture, m_input_sampler);
    glBindImageTexture(m_bindings.output_image, dst->id(), 0, false, 0, GL_WRITE_ONLY, GL_RGBA16F);

    auto const count_x = (width + m_group_sizes.x - 1) / m_group_sizes.x;
    auto const count_y = (height + m_group_sizes.y - 1) / m_group_sizes.y;
    glDispatchCompute(count_x, count_y, 1);
    glMemoryBarrierByRegion(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glMemoryBarrierByRegion(GL_TEXTURE_FETCH_BARRIER_BIT);
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT);

    return dst;
  }
  void add::create() {
    glsp::preprocess_file_info generate_file;
    generate_file.file_path = res_dir / "../src/pt/pp_add.comp";
    auto const preprocessed_file = glsp::preprocess_file(generate_file);

    auto const program = make_program(preprocessed_file.contents);

    if (program) {
      if (glIsProgram(m_program))
        glDeleteProgram(m_program);

      m_program = program.value();
      load_bindings();
    }
    if (glIsSampler(m_input_sampler))
      glDeleteSamplers(1, &m_input_sampler);

    glCreateSamplers(1, &m_input_sampler);
    glSamplerParameterf(m_input_sampler, GL_TEXTURE_MAX_ANISOTROPY, 16.f);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_WRAP_R, GL_MIRRORED_REPEAT);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glSamplerParameteri(m_input_sampler, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
  }
  void add::load_bindings() {
    glUseProgram(m_program);
    m_bindings.input_image = if_empty(sampler_binding(m_program, "input_image"));
    m_bindings.output_image = if_empty(sampler_binding(m_program, "output_image"));
    m_bindings.overlay_texture = if_empty(sampler_binding(m_program, "overlay_texture"));
    m_bindings.factor = if_empty(uniform_location(m_program, "factor"));

    std::int32_t group_sizes[3]{};
    glGetProgramiv(m_program, GL_COMPUTE_WORK_GROUP_SIZE, group_sizes);
    m_group_sizes = {group_sizes[0], group_sizes[1]};
  }

  std::shared_ptr<texture_t> bloom::process(texture_provider_t& provider, std::uint32_t texture) {
    auto result = m_cutoff.process(provider, texture);

    m_blur.step = blur_step;

    result = m_cutoff.process(provider, result->id());
    m_scale.factor = 1.f / texture_scaling;
    result = m_scale.process(provider, result->id());
    result = m_blur.process(provider, result->id());
    m_scale.factor = texture_scaling;
    result = m_scale.process(provider, result->id());
   // result = m_blur.process(provider, result->id());
    m_add.overlay_texture = result->id();
    m_add.factor = overlay_factor;
    result = m_add.process(provider, texture);
    return result;
  }

  std::shared_ptr<texture_t> blur2d::process(texture_provider_t& provider, std::uint32_t texture) {
    m_blur.step = step;

    m_blur.direction = blur::axis::x;
    auto result = m_blur.process(provider, texture);
    m_blur.direction = blur::axis::y;
    return m_blur.process(provider, result->id());
  }

} // namespace myrt