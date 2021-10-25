#include "shader_build.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace myrt {
namespace {
  class local_includer : public shaderc::CompileOptions::IncluderInterface {
  public:
    local_includer(std::vector<std::filesystem::path> include_directories)
        : m_include_directories(include_directories) {}
    local_includer() = default;

    // Inherited via IncluderInterface
    virtual shaderc_include_result* GetInclude(const char* requested_source, shaderc_include_type type,
        const char* requesting_source, size_t include_depth) override {
      namespace fs = std::filesystem;

      fs::path requesting(requesting_source);
      fs::path requested(requested_source);

      bool is_absolute = false;
      if (type == shaderc_include_type_relative) {
        auto const relative_to_absolute = requesting.parent_path() / requested;

        if (fs::exists(relative_to_absolute)) {
          requested = relative_to_absolute;
          is_absolute = true;
        }
      }

      if(!is_absolute) {
        for (auto const& include_dir : m_include_directories) {
          if (fs::exists(include_dir / requested)) {
            requested = include_dir / requested;
            break;
          }
        }
      }

      if (!fs::exists(requested))
        return nullptr;

      std::stringstream stream;
      stream << std::ifstream(requested).rdbuf();

      auto const requested_name = requested.string();
      auto const requested_contents = stream.str();
      std::unique_ptr<char[]> name = std::make_unique<char[]>(requested_name.size());
      std::strncpy(name.get(), requested_name.data(), requested_name.size());
      std::unique_ptr<char[]> contents = std::make_unique<char[]>(requested_contents.size());
      std::strncpy(contents.get(), requested_contents.data(), requested_contents.size());

      auto result = std::make_unique<shaderc_include_result>();
      result->source_name = name.get();
      result->source_name_length = requested_name.size();
      result->content = contents.get();
      result->content_length = requested_contents.size();

      name.release();
      contents.release();
      return result.release();
    }
    virtual void ReleaseInclude(shaderc_include_result* data) override {
      if (data) {
        delete[] data->source_name;
        delete[] data->content;
        delete data;
      }
    }

  private:
    std::vector<std::filesystem::path> m_include_directories;
  };
} // namespace
std::vector<std::uint32_t> compile_file(shaderc_shader_kind kind, std::filesystem::path const& file) {
  shaderc::CompileOptions options;
  options.SetIncluder(std::make_unique<local_includer>());

  std::stringstream stream;
  stream << std::ifstream(file).rdbuf();

  shaderc::Compiler compiler;
  auto result = compiler.CompileGlslToSpv(stream.str(), kind, file.string().c_str(), options);
  if(result.GetCompilationStatus() != shaderc_compilation_status_success) {
    std::cout << result.GetErrorMessage() << '\n';
    return {};
  }
  return {result.cbegin(), result.cend()};
}
// namespace
} // namespace myrt