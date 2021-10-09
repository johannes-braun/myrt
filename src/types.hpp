#pragma once

#include <cstdint>
#include <cstring>
#include <utility>
#include <memory>
#include <vector>
#include <string>
#include <string_view>
#include <variant>
#include <sstream>

namespace myrt {
struct parameter_type;
struct parameter;

struct constant_type {
  std::string_view id;
  std::string_view glsl_type;
  std::string_view glsl_code;
  size_t blocks;

  void (*write)(float* dst, void const* src);
  void (*read)(void* dst, float const* src);
};

enum class parameter_type_id : std::int64_t {
  type_float,
  type_int,
  type_uint,
  type_vec2,
  type_vec3,
  type_vec4,
  type_vec4unorm8,
  type_ivec2,
  type_ivec3,
  type_ivec4,
  type_uvec2,
  type_uvec3,
  type_uvec4,
  type_mat2,
  type_mat3,
  type_mat4,
};

namespace internal_types {
  struct wrapper {
    constexpr static std::array registered_types{constant_type{"float", "float", "return in0", 1,
                                                     [](auto* d, auto const* s) { std::memcpy(d, s, sizeof(float)); },
                                                     [](auto* d, auto const* s) { std::memcpy(d, s, sizeof(float)); }},
        constant_type{"int", "int", "return floatBitsToInt(in0)", 1,
            [](auto* d, auto const* s) { std::memcpy(d, s, sizeof(std::int32_t)); },
            [](auto* d, auto const* s) { std::memcpy(d, s, sizeof(std::int32_t)); }},
        constant_type{"uint", "uint", "return floatBitsToUint(in0)", 1,
            [](auto* d, auto const* s) { std::memcpy(d, s, sizeof(std::uint32_t)); },
            [](auto* d, auto const* s) { std::memcpy(d, s, sizeof(std::uint32_t)); }},
        constant_type{"vec2", "vec2", "return vec2(in0, in1)", 2,
            [](auto* d, auto const* s) { std::memcpy(d, s, 2 * sizeof(float)); },
            [](auto* d, auto const* s) { std::memcpy(d, s, 2 * sizeof(float)); }},
        constant_type{"vec3", "vec3", "return vec3(in0, in1, in2)", 3,
            [](auto* d, auto const* s) { std::memcpy(d, s, 3 * sizeof(float)); },
            [](auto* d, auto const* s) { std::memcpy(d, s, 3 * sizeof(float)); }},
        constant_type{"vec4", "vec4", "return vec4(in0, in1, in2, in3)", 4,
            [](auto* d, auto const* s) { std::memcpy(d, s, 4 * sizeof(float)); },
            [](auto* d, auto const* s) { std::memcpy(d, s, 4 * sizeof(float)); }},
        constant_type{"vec4unorm8", "vec4", "return unpackUnorm4x8(floatBitsToUint(in0))", 1,
            [](auto* d, auto const* s) { std::memcpy(d, s, sizeof(std::uint32_t)); },
            [](auto* d, auto const* s) { std::memcpy(d, s, sizeof(std::uint32_t)); }},
        constant_type{"ivec2", "ivec2", "return ivec2(floatBitsToInt(in0), floatBitsToInt(in1))", 2,
            [](auto* d, auto const* s) { std::memcpy(d, s, 2 * sizeof(std::int32_t)); },
            [](auto* d, auto const* s) { std::memcpy(d, s, 2 * sizeof(std::int32_t)); }},
        constant_type{"ivec3", "ivec3", "return ivec3(floatBitsToInt(in0), floatBitsToInt(in1), floatBitsToInt(in2))",
            3, [](auto* d, auto const* s) { std::memcpy(d, s, 3 * sizeof(std::int32_t)); },
            [](auto* d, auto const* s) { std::memcpy(d, s, 3 * sizeof(std::int32_t)); }},
        constant_type{"ivec4", "ivec4",
            "return ivec4(floatBitsToInt(in0), floatBitsToInt(in1), floatBitsToInt(in2), floatBitsToInt(in3))", 4,
            [](auto* d, auto const* s) { std::memcpy(d, s, 4 * sizeof(std::int32_t)); },
            [](auto* d, auto const* s) { std::memcpy(d, s, 4 * sizeof(std::int32_t)); }},
        constant_type{"uvec2", "uvec2", "return uvec2(floatBitsToUint(in0), floatBitsToUint(in1))", 2,
            [](auto* d, auto const* s) { std::memcpy(d, s, 2 * sizeof(std::uint32_t)); },
            [](auto* d, auto const* s) { std::memcpy(d, s, 2 * sizeof(std::uint32_t)); }},
        constant_type{"uvec3", "uvec3",
            "return uvec3(floatBitsToUint(in0), floatBitsToUint(in1), floatBitsToUint(in2))", 3,
            [](auto* d, auto const* s) { std::memcpy(d, s, 3 * sizeof(std::uint32_t)); },
            [](auto* d, auto const* s) { std::memcpy(d, s, 3 * sizeof(std::uint32_t)); }},
        constant_type{"uvec4", "uvec4",
            "return uvec4(floatBitsToUint(in0), floatBitsToUint(in1), floatBitsToUint(in2), floatBitsToUint(in3))", 4,
            [](auto* d, auto const* s) { std::memcpy(d, s, 4 * sizeof(std::uint32_t)); },
            [](auto* d, auto const* s) { std::memcpy(d, s, 4 * sizeof(std::uint32_t)); }},
        constant_type{"mat2", "mat2", "return mat2(in0, in1, in2, in3)", 4,
            [](auto* d, auto const* s) { std::memcpy(d, s, 4 * sizeof(float)); },
            [](auto* d, auto const* s) { std::memcpy(d, s, 4 * sizeof(float)); }},
        constant_type{"mat3", "mat3", "return mat3(in0, in1, in2, in3, in4, in5, in6, in7, in8)", 9,
            [](auto* d, auto const* s) { std::memcpy(d, s, 9 * sizeof(float)); },
            [](auto* d, auto const* s) { std::memcpy(d, s, 9 * sizeof(float)); }},
        constant_type{"mat4", "mat4",
            "return mat4(in0, in1, in2, in3, in4, in5, in6, in7, in8, in9, in10, in11, in12, in13, in14, in15)", 16,
            [](auto* d, auto const* s) { std::memcpy(d, s, 16 * sizeof(float)); },
            [](auto* d, auto const* s) { std::memcpy(d, s, 16 * sizeof(float)); }}};
  };
  constexpr std::string_view internal_load_identifier = "ldv"; // LoaD Value

  namespace generate_loaders {
    template <size_t S> struct c_str { char string[S]; };

    constexpr size_t log10(size_t v) {
      size_t l = 1;
      while (v /= 10) l++;
      return l;
    }
    constexpr double powr(double x, int exp) {
      int sign = 1;
      if (exp < 0) {
        sign = -1;
        exp = -exp;
      }
      if (exp == 0)
        return x < 0 ? -1.0 : 1.0;
      double ret = x;
      while (--exp) ret *= x;
      return sign > 0 ? ret : 1.0 / ret;
    }

    template <size_t... I> constexpr size_t par_size(std::index_sequence<I...>) {
      return ((std::size(",float in") - 1 + log10(I)) + ... + -1);
    }
    static_assert(par_size(std::make_index_sequence<4>{}) == std::size("float in0,float in1,float in2,float in3") - 1);

    constexpr size_t type_str_len(size_t index, constant_type const& type) {
      return type.glsl_code.length() + type.glsl_type.length() + 6 + internal_load_identifier.length() + log10(index);
    }

    constexpr void append(char** ptr_ptr, char c) {
      char* ptr = *ptr_ptr;
      *ptr = c;
      *ptr_ptr += 1;
    }

    constexpr void append(char** ptr_ptr, std::string_view str) {
      for (size_t i = 0; i < str.size(); ++i) append(ptr_ptr, str[i]);
    }

    constexpr void append_size_t(char** ptr_ptr, size_t s) {
      auto const l10 = log10(s) - 1;
      auto div = size_t(powr(10, l10));
      do {
        append(ptr_ptr, '0' + (s / div) % 10);
      } while (div /= 10);
    }

    constexpr void insert(char*& ptr, size_t index, constant_type const& type) {
      append(&ptr, type.glsl_type);
      append(&ptr, ' ');
      append(&ptr, internal_load_identifier);
      append_size_t(&ptr, index);
      append(&ptr, '(');
      for (size_t pindex = 0; pindex < type.blocks; ++pindex) {
        if (pindex != 0)
          append(&ptr, ',');
        append(&ptr, "float in");
        append_size_t(&ptr, pindex);
      }
      append(&ptr, ')');
      append(&ptr, '{');
      append(&ptr, type.glsl_code);
      append(&ptr, ";}");
    }

    template <size_t... I> constexpr auto build_loaders_impl(std::index_sequence<I...>) {
      c_str<((par_size(std::make_index_sequence<wrapper::registered_types[I].blocks>{}) +
                 type_str_len(I, wrapper::registered_types[I])) +
             ... + 1)>
          result{};

      char* ptr = result.string;

      (insert(ptr, I, wrapper::registered_types[I]), ...);

      return result;
    }

    constexpr auto build_loaders() {
      return build_loaders_impl(std::make_index_sequence<std::size(wrapper::registered_types)>{});
    }

    constexpr auto result = build_loaders();
  } // namespace generate_loaders

  constexpr std::string_view loader_glsl = {generate_loaders::result.string,
      std::find((const char*)generate_loaders::result.string,
          generate_loaders::result.string + std::size(generate_loaders::result.string), '\0')};

  constexpr std::optional<parameter_type_id> get_id(std::string_view identifier) {
    auto const iter = std::find_if(begin(wrapper::registered_types), end(wrapper::registered_types),
        [&](constant_type const& type) { return type.id == identifier; });
    if (iter == end(wrapper::registered_types))
      return std::nullopt;
    return parameter_type_id{std::distance(begin(wrapper::registered_types), iter)};
  }
  constexpr constant_type const* get_type(parameter_type_id id) {
    if (size_t(id) >= wrapper::registered_types.size())
      return nullptr;
    return &wrapper::registered_types[size_t(id)];
  }
  constexpr parameter_type_id get_id(constant_type const* type) {
    return parameter_type_id{std::distance(data(wrapper::registered_types), type)};
  }
  constexpr constant_type const* get_type(std::string_view identifier) {
    auto const id = get_id(identifier);
    if (!id)
      return nullptr;
    return get_type(*id);
  }
}; // namespace internal_types

struct object_host {
  std::vector<float> m_buffer;
};

struct object_type {
public:
  struct parameter_info {
    std::int64_t relative_offset;
    std::variant<constant_type const*, std::shared_ptr<object_type const>> type;

    size_t blocks() const {
      const struct {
        size_t operator()(constant_type const* type) const {
          return type->blocks;
        }
        size_t operator()(std::shared_ptr<object_type const> type) const {
          return type->blocks();
        }
      } block_count_visitor;

      return std::visit(block_count_visitor, type);
    }
  };

  friend std::string generate_object_loader(object_type const& obj);

  object_type(std::string name) : m_name(std::move(name)) {}

  std::string const& name() const noexcept {
    return m_name;
  }
  size_t blocks() const {
    return m_size;
  }

  void add_parameter(std::string name, parameter_type_id type_id) {
    auto& back = m_parameters.emplace_back(
        std::move(name), parameter_info{static_cast<std::int64_t>(m_size), internal_types::get_type(type_id)});
    m_size += back.second.blocks();
  }
  void add_parameter(std::string name, std::variant<constant_type const*, std::shared_ptr<object_type const>> type) {
    auto& back = m_parameters.emplace_back(std::move(name), parameter_info{static_cast<std::int64_t>(m_size), type});
    m_size += back.second.blocks();
  }

  std::vector<std::pair<std::string, parameter_info>> const& parameters() const {
    return m_parameters;
  }

private:
  std::string m_name;
  std::size_t m_size = 0;
  std::vector<std::pair<std::string, parameter_info>> m_parameters;
};

inline void write(constant_type const* type, float* dst, void const* src) {
  type->write(dst, src);
}
inline void write(std::shared_ptr<object_type const> const& type, float* dst, void const* src) {
  float const* src_ptr = reinterpret_cast<float const*>(src);
  for (auto const& par : type->parameters()) {
    std::visit([&](auto const& x) { write(x, dst + par.second.relative_offset, src_ptr + par.second.relative_offset); },
        par.second.type);
  }
}

struct types_registry {
public:
  template <typename T, typename... Args> std::shared_ptr<object_type const> const& create(Args&&... args) {
    return m_object_types.emplace_back(std::make_shared<T const>(*this, std::forward<Args>(args)...));
  }

  void push_back(std::shared_ptr<object_type const> type) {
    m_object_types.push_back(std::move(type));
  }

  std::variant<constant_type const*, std::shared_ptr<object_type const>> find(std::string_view name) {
    auto const iter =
        std::find_if(begin(m_object_types), end(m_object_types), [&](auto const& ptr) { return ptr->name() == name; });
    if (iter == end(m_object_types))
      return internal_types::get_type(name);
    return *iter;
  }

  std::shared_ptr<object_type const> find_object_type(std::string_view name) {
    auto const iter =
        std::find_if(begin(m_object_types), end(m_object_types), [&](auto const& ptr) { return ptr->name() == name; });
    if (iter == end(m_object_types))
      return nullptr;
    return *iter;
  }

  std::vector<std::shared_ptr<object_type const>> const& object_types() const {
    return m_object_types;
  }

private:
  std::vector<std::shared_ptr<object_type const>> m_object_types;
};

inline std::string generate_object_loader(myrt::object_type const& obj) {
  std::stringstream stream;
  std::stringstream generator;

  generator << obj.name() << " " << obj.name() << "_load_(uint buffer_offset){";
  generator << obj.name() << " obj;";

  int num_float = 0;
  stream << "struct " << obj.name() << "{";
  for (auto const& par : obj.m_parameters) {
    if (std::holds_alternative<std::shared_ptr<myrt::object_type const>>(par.second.type)) {
      auto const param_type = std::get<std::shared_ptr<myrt::object_type const>>(par.second.type);
      stream << param_type->name() << ' ' << par.first << ';';

      generator << "obj." << par.first << "=" << param_type->name() << "_load_(buffer_offset+" << num_float << ");";
      num_float += param_type->blocks();

    } else if (std::holds_alternative<myrt::constant_type const*>(par.second.type)) {
      auto const param_type = std::get<myrt::constant_type const*>(par.second.type);
      stream << param_type->glsl_type << ' ' << par.first << ';';

      for (size_t block = 0; block < param_type->blocks; ++block) {
        generator << "float f" << num_float + block << "=load_float(buffer_offset+"
                  << (block + par.second.relative_offset) << ");";
      }
      generator << "obj." << par.first << "=" << myrt::internal_types::internal_load_identifier
                << size_t(myrt::internal_types::get_id(param_type)) << "(";
      for (size_t block = 0; block < param_type->blocks; ++block) {
        if (block != 0)
          generator << ",";
        generator << "f" << num_float + block;
      }
      generator << ");";
      num_float += param_type->blocks;
    }
  }
  stream << "};";
  generator << "return obj;}";
  return stream.str() + generator.str();
}

inline std::string generate_object_loaders(myrt::types_registry const& registry) {
  std::ostringstream stream;
  stream << myrt::internal_types::loader_glsl;
  for (auto const& type : registry.object_types()) {
    stream << myrt::generate_object_loader(*type);
  }
  return stream.str();
}
} // namespace myrt
