#pragma once

#include <memory>
#include <utility>
#include <string>
#include <vector>
#include <span>
#include <sstream>
#include <stdexcept>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <rnu/math/math.hpp>

namespace myrt
{
  namespace detail
  {
    inline std::string to_hex_string(size_t hash) {
      std::stringstream stream;
      stream << std::hex << (std::uint32_t(hash) ^ std::uint32_t(hash >> 32));
      return stream.str();
    }
    template<typename T>
    inline int hash_reduced(T&& t,
      std::unordered_map<size_t, int>& hash_reducers,
      int& hash_counter)
    {
      auto const hash = std::hash<std::decay_t<T>>{}(std::forward<T>(t));

      if (auto const it = hash_reducers.find(hash); it != hash_reducers.end())
      {
        return it->second;
      }
      else
      {
        hash_reducers.emplace_hint(it, std::make_pair(hash, hash_counter++));
        return hash_counter - 1;
      }
    }
  }

  struct parameter_type
  {
    parameter_type(std::string type_identifier, std::string type_name, int buffer_blocks, std::string buffer_load);

    std::string type_identifier;
    std::string type_name;
    int buffer_blocks;
    std::string function;
  };

  struct parameter;
  struct parameter_link {
    enum class type {
      link,
      offset
    };

    bool is_linked() const;
    size_t hash() const;

    std::shared_ptr<parameter> other;
    size_t block;
  };

  struct parameter {
    parameter(std::shared_ptr<parameter_type> type);

    parameter_link link_value_block(size_t block, std::shared_ptr<parameter> other, size_t other_block);
    parameter_link unlink_value_block(size_t block);
    parameter_link const& get_link(size_t block) const;

    std::vector<float> const& get_default_value() const;
    void set_default_value(std::span<float const> data);

    std::shared_ptr<parameter_type> type;
  private:
    std::vector<float> _default_value;
    std::vector<parameter_link> _value_links;
  };

  std::shared_ptr<parameter_type> create_parameter_type(std::string type_identifier, std::string type_name, int buffer_blocks, std::string buffer_load);

#define def_param_type(name, type_name, blk, glsl) \
  struct name##_type : parameter_type { name##_type() : parameter_type(#name, type_name, (blk), (glsl)) {} };\
  struct name##_param : parameter { \
  static inline const std::shared_ptr<name##_type> type = std::make_shared<name##_type>(); \
  name##_param() : parameter(type) {} };\
  inline std::shared_ptr<name##_param> make_##name##_param() { return std::make_shared<name##_param>(); }

  def_param_type(uint, "uint", 1, "return uint(in_block0)");
  def_param_type(int, "int", 1, "return int(in_block0)");
  def_param_type(float, "float", 1, "return in_block0");
  def_param_type(vec2, "vec2", 2, "return vec2(in_block0, in_block1)");
  def_param_type(uvec2, "uvec2", 2, "return uvec2(floatBitsToUint(in_block0), floatBitsToUint(in_block1))");
  def_param_type(vec2_unorm16, "vec2", 1, "return unpackUnorm2x16(floatBitsToUint(in_block0))");
  def_param_type(vec3, "vec3", 3, "return vec3(in_block0, in_block1, in_block2)");
  def_param_type(vec4, "vec4", 4, "return vec4(in_block0, in_block1, in_block2, in_block3)");
  def_param_type(vec4_unorm8, "vec4", 1, "return unpackUnorm4x8(floatBitsToUint(in_block0))");
  def_param_type(mat2, "mat2", 4, "return mat2(in_block0, in_block1, in_block2, in_block3)");
  def_param_type(mat4, "mat4", 16, "return mat4(in_block0, in_block1, in_block2, in_block3, in_block0, in_block1, in_block2, in_block3, in_block0, in_block1, in_block2, in_block3, in_block0, in_block1, in_block2, in_block3)");
#undef def_param_type

  struct global_info
  {
    std::stringstream auxilliary;
    std::unordered_set<size_t> used_functions;
    std::unordered_map<size_t, int> hash_reducers;
    std::unordered_map<size_t, int> used_types_base_offset;
    int hash_counter = 0;
    int current_offset = 0;
    std::unordered_map<size_t, int> buffer_offsets;
    std::map<std::pair<std::string, int>, int> named_param_offsets;

    template<typename T>
    auto hash(T&& t) { return detail::hash_reduced(std::forward<T>(t), hash_reducers, hash_counter); }
  };

  struct parameter_scope
  {
    std::stringstream inline_code;
    size_t hash;
    std::unordered_set<size_t> used_objects;
  };

  struct parameter_buffer_description
  {
    int id = 0;

    int base_offset = 0;
    size_t blocks_required = 0;
    std::unordered_set<std::shared_ptr<parameter>> parameters;
    std::unordered_map<size_t, int> buffer_offsets;

    void append(global_info& global, std::shared_ptr<parameter> const& param);
    void apply_defaults(std::span<float> buffer);

    void set_value(float* buffer, std::shared_ptr<parameter> const& param, float const* data, size_t buflen = 0) const;
    void set_value(float* buffer, std::shared_ptr<parameter> const& param, int data) const;
    void set_value(float* buffer, std::shared_ptr<parameter> const& param, unsigned data) const;
    void set_value(float* buffer, std::shared_ptr<parameter> const& param, float data) const;
    void set_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::vec2 data) const;
    void set_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::vec3 data) const;
    void set_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::vec4 data) const;
    void set_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::vec4ui8 data) const;
    void set_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::vec2ui data) const;
    void set_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::mat2 data) const;
    void set_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::mat4 data) const;

    void get_link_value(float* buffer, parameter_link const& self, float* data) const;

    void get_value(float* buffer, std::shared_ptr<parameter> const& param, float* data) const;
    void get_value(float* buffer, std::shared_ptr<parameter> const& param, int& data) const;
    void get_value(float* buffer, std::shared_ptr<parameter> const& param, unsigned& data) const;
    void get_value(float* buffer, std::shared_ptr<parameter> const& param, float& data) const;
    void get_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::vec2& data) const;
    void get_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::vec3& data) const;
    void get_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::vec4& data) const;
    void get_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::mat2& data) const;
    void get_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::mat4& data) const;

    template<typename T, typename Param>
    T get_value(float* buffer, std::shared_ptr<Param> const& param) const requires std::is_base_of_v<parameter, Param>{
      T value;
      get_value(buffer, param, value);
      return value;
    }
  };
  
  std::string resolve_parameter(std::shared_ptr<parameter> param, parameter_scope& scope, global_info& globals);
}