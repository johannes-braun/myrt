#pragma once

#include "glad/glad.h"
#include <span>
#include <string_view>
#include <optional>

namespace myrt
{
  std::string replace(std::string str, const std::string& sub1, const std::string& sub2);

  std::optional<GLuint> make_program(std::span<std::string_view const> vertex_shader_codes, std::span<std::string_view const> fragment_shader_codes);
  std::optional<GLuint> make_program(std::string_view const vertex_shader_code, std::string_view const fragment_shader_code);

  std::optional<GLuint> make_program(std::span<std::string_view const> compute_shader_codes);
  std::optional<GLuint> make_program(std::string_view const compute_shader_code);
}