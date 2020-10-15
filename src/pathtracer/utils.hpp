#pragma once

#include <glad/glad.h>
#include <span>
#include <string_view>

namespace myrt
{
    GLuint make_program(std::span<std::string_view const> vertex_shader_codes, std::span<std::string_view const> fragment_shader_codes);
    GLuint make_program(std::string_view const vertex_shader_code, std::string_view const fragment_shader_code);
}