#include "post_process.hpp"
#include <glsp/glsp.hpp>
#include <myrt/sfml/utils.hpp>
#include "bindings.hpp"

namespace myrt {
const static std::filesystem::path res_dir = "../../../res";

std::shared_ptr<texture_t> basic_post_process::process(texture_provider_t& provider, std::uint32_t texture) {
  if (!glIsProgram(m_program))
    create_program();

  int width = 0;
  int height = 0;
  glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_WIDTH, &width);
  glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_HEIGHT, &height);
  glUseProgram(m_program);
  glBindTextureUnit(m_base_bindings.input_image, texture);
  glBindSampler(m_base_bindings.input_image, m_sampler);
  auto dst = provider.get(GL_TEXTURE_2D, GL_RGBA16F, width, height, 1);
  glBindImageTexture(m_base_bindings.output_image, dst->id(), 0, false, 0, GL_WRITE_ONLY, GL_RGBA16F);

  on_apply_bindings(m_program);

  auto const count_x = (width + m_group_sizes.x - 1) / m_group_sizes.x;
  auto const count_y = (height + m_group_sizes.y - 1) / m_group_sizes.y;
  glDispatchCompute(count_x, count_y, 1);
  glMemoryBarrierByRegion(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  glMemoryBarrierByRegion(GL_TEXTURE_FETCH_BARRIER_BIT);
  glMemoryBarrier(GL_COMMAND_BARRIER_BIT);

  return dst;
}
std::uint32_t basic_post_process::sampler() const noexcept {
  return m_sampler;
}
void basic_post_process::create_program() {
  auto const program = on_create_program();

  if (program) {
    if (glIsProgram(m_program))
      glDeleteProgram(m_program);

    m_program = program.value();
    load_bindings();
  }
  if (glIsSampler(m_sampler))
    glDeleteSamplers(1, &m_sampler);

  glCreateSamplers(1, &m_sampler);
  glSamplerParameterf(m_sampler, GL_TEXTURE_MAX_ANISOTROPY, 16.f);
  glSamplerParameteri(m_sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glSamplerParameteri(m_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glSamplerParameteri(m_sampler, GL_TEXTURE_WRAP_R, GL_MIRRORED_REPEAT);
  glSamplerParameteri(m_sampler, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
  glSamplerParameteri(m_sampler, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
}
void basic_post_process::load_bindings() {
  glUseProgram(m_program);

  m_base_bindings.input_image = if_empty(sampler_binding(m_program, "input_image"));
  m_base_bindings.output_image = if_empty(sampler_binding(m_program, "output_image"));

  on_load_bindings(m_program);

  std::int32_t group_sizes[3]{};
  glGetProgramiv(m_program, GL_COMPUTE_WORK_GROUP_SIZE, group_sizes);
  m_group_sizes = {group_sizes[0], group_sizes[1]};
}

void denoise::on_apply_bindings(std::uint32_t program) {
  glUniform1f(m_bindings.exponent, exponent);
  glUniform1f(m_bindings.strength, strength);
}

std::optional<std::uint32_t> denoise::on_create_program() {
  glsp::preprocess_file_info generate_file;
  generate_file.file_path = res_dir / "../src/pt/pp_denoise.comp";
  auto const preprocessed_file = glsp::preprocess_file(generate_file);
  return make_program(preprocessed_file.contents);
}

void denoise::on_load_bindings(std::uint32_t program) {
  m_bindings.exponent = if_empty(uniform_location(program, "exponent"));
  m_bindings.strength = if_empty(uniform_location(program, "strength"));
}

std::optional<std::uint32_t> fxaa::on_create_program() {
  glsp::preprocess_file_info generate_file;
  generate_file.file_path = res_dir / "../src/pt/pp_fxaa.comp";
  auto const preprocessed_file = glsp::preprocess_file(generate_file);
  return make_program(preprocessed_file.contents);
}

void blur::on_apply_bindings(std::uint32_t program) {
  glUniform1i(m_bindings.direction, int(direction));
  glUniform1f(m_bindings.step, step);
}

std::optional<std::uint32_t> blur::on_create_program() {
  glsp::preprocess_file_info generate_file;
  generate_file.file_path = res_dir / "../src/pt/pp_blur.comp";
  auto const preprocessed_file = glsp::preprocess_file(generate_file);
  return make_program(preprocessed_file.contents);
}

void blur::on_load_bindings(std::uint32_t program) {
  m_bindings.direction = if_empty(uniform_location(program, "direction"));
  m_bindings.step = if_empty(uniform_location(program, "step"));
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

void cutoff::on_apply_bindings(std::uint32_t program) {
  glUniform1i(m_bindings.leave_above, leave_above);
  glUniform4fv(m_bindings.value, 1, value.data());
}

std::optional<std::uint32_t> cutoff::on_create_program() {
  glsp::preprocess_file_info generate_file;
  generate_file.file_path = res_dir / "../src/pt/pp_cutoff.comp";
  auto const preprocessed_file = glsp::preprocess_file(generate_file);
  return make_program(preprocessed_file.contents);
}

void cutoff::on_load_bindings(std::uint32_t program) {
  m_bindings.value = if_empty(uniform_location(program, "value"));
  m_bindings.leave_above = if_empty(uniform_location(program, "leave_above"));
}

std::optional<std::uint32_t> tonemap::on_create_program() {
  glsp::preprocess_file_info generate_file;
  generate_file.file_path = res_dir / "../src/pt/pp_tonemap.comp";
  auto const preprocessed_file = glsp::preprocess_file(generate_file);
  return make_program(preprocessed_file.contents);
}

void add::on_apply_bindings(std::uint32_t program) {
  glUniform1f(m_bindings.factor, factor);
  glBindTextureUnit(m_bindings.overlay_texture, overlay_texture);
  glBindSampler(m_bindings.overlay_texture, sampler());
}
std::optional<std::uint32_t> add::on_create_program() {
  glsp::preprocess_file_info generate_file;
  generate_file.file_path = res_dir / "../src/pt/pp_add.comp";
  auto const preprocessed_file = glsp::preprocess_file(generate_file);
  return make_program(preprocessed_file.contents);
}
void add::on_load_bindings(std::uint32_t program) {
  m_bindings.overlay_texture = if_empty(sampler_binding(program, "overlay_texture"));
  m_bindings.factor = if_empty(uniform_location(program, "factor"));
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