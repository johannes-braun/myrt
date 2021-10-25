#pragma once
#include <filesystem>
#include <shaderc/shaderc.hpp>

namespace myrt {
std::vector<std::uint32_t> compile_file(shaderc_shader_kind kind, std::filesystem::path const& file);
}