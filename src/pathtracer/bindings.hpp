#pragma once
#include <optional>
#include <cstdint>

namespace myrt {
  std::optional<std::int32_t> input_location(std::uint32_t program, const char* name);
  std::optional<std::int32_t> uniform_location(std::uint32_t program, const char* name);
  std::optional<std::int32_t> storage_buffer_binding(std::uint32_t program, const char* name);
  std::optional<std::int32_t> sampler_binding(std::uint32_t program, const char* name);
  std::uint32_t if_empty(std::optional<std::int32_t> const& val);
}