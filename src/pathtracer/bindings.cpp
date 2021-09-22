#include "bindings.hpp"
#include <mygl/mygl.hpp>

namespace myrt
{
  std::optional<std::int32_t> input_location(std::uint32_t program, const char* name)
  {
    auto const location = glGetProgramResourceLocation(program, GL_PROGRAM_INPUT, name);
    if (location == GL_INVALID_INDEX)
      return std::nullopt;
    return location;
  }
  std::optional<std::int32_t> uniform_location(std::uint32_t program, const char* name)
  {
    auto const location = glGetProgramResourceLocation(program, GL_UNIFORM, name);
    if (location == GL_INVALID_INDEX)
      return std::nullopt;
    return location;
  }
  std::optional<std::int32_t> storage_buffer_binding(std::uint32_t program, const char* name)
  {
    std::int32_t index = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, name);
    if (index == GL_INVALID_INDEX)
      return std::nullopt;

    constexpr GLenum prop = GL_BUFFER_BINDING;
    std::int32_t len;
    std::int32_t binding = GL_INVALID_INDEX;
    glGetProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, index, 1, &prop, 1, &len, &binding);

    if (binding == GL_INVALID_INDEX)
      return std::nullopt;
    return binding;
  }
  std::optional<std::int32_t> sampler_binding(std::uint32_t program, const char* name)
  {
    std::int32_t binding = GL_INVALID_INDEX;
    auto const location = uniform_location(program, name);
    if (!location)
      return std::nullopt;
    glGetUniformiv(program, location.value(), &binding);
    if (binding == GL_INVALID_INDEX)
      return std::nullopt;
    return binding;
  }
  std::uint32_t if_empty(std::optional<std::int32_t> const& val)
  {
    return val ? *val : -1;
  }
}