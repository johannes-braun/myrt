#pragma once

#include <memory>

namespace myrt::dyn
{

  struct property_type
  {
    using block_type = float;
    constexpr static size_t block_size = sizeof(block_type);

    size_t num_blocks = 1;
    void(*set)(property_type const& self, void const* data, block_type* buf) = &default_set;
    void(*get)(property_type const& self, void* data, block_type const* buf) = &default_get;

    struct property_type_glsl
    {
      const char* type_name;
      const char* load_value;
    } glsl_info;

  private:
    static void default_set(property_type const& self, void const* data, block_type* buf) {
      std::memcpy(buf, data, self.num_blocks * block_size);
    }
    static void default_get(property_type const& self, void* data, block_type const* buf) {
      std::memcpy(data, buf, self.num_blocks * block_size);
    }
  };

  std::shared_ptr<property_type const> simple_property(size_t blocks, char const* name, char const* load);

  // unpacked
  static inline const std::shared_ptr<property_type const> float_property = simple_property(1, "float", "in_block0");
  static inline const std::shared_ptr<property_type const> vec2_property = simple_property(2, "vec2", "vec2(in_block0, in_block1)");
  static inline const std::shared_ptr<property_type const> vec3_property = simple_property(3, "vec3", "vec3(in_block0, in_block1, in_block2)");
  static inline const std::shared_ptr<property_type const> vec4_property = simple_property(4, "vec4", "vec4(in_block0, in_block1, in_block2, in_block3)");
  static inline const std::shared_ptr<property_type const> mat2_property = simple_property(4, "mat2", "mat2(in_block0, in_block1, in_block2, in_block3)");
  static inline const std::shared_ptr<property_type const> mat3_property = simple_property(9, "mat3", "mat3(in_block0, in_block1, in_block2, in_block3, in_block4, in_block5, in_block6, in_block7, in_block8)");
  static inline const std::shared_ptr<property_type const> mat4_property = simple_property(16, "mat4", "mat4(in_block0, in_block1, in_block2, in_block3, in_block4, in_block5, in_block6, in_block7, in_block8, in_block9, in_block10, in_block11, in_block12, in_block13, in_block14, in_block15)");
  static inline const std::shared_ptr<property_type const> int_property = simple_property(1, "int", "floatBitsToInt(in_block0)");
  static inline const std::shared_ptr<property_type const> ivec2_property = simple_property(2, "ivec2", "ivec2(floatBitsToInt(in_block0), floatBitsToInt(in_block1))");
  static inline const std::shared_ptr<property_type const> ivec3_property = simple_property(3, "ivec3", "ivec3(floatBitsToInt(in_block0), floatBitsToInt(in_block1), floatBitsToInt(in_block2))");
  static inline const std::shared_ptr<property_type const> ivec4_property = simple_property(4, "ivec4", "ivec4(floatBitsToInt(in_block0), floatBitsToInt(in_block1), floatBitsToInt(in_block2), floatBitsToInt(in_block3))");
  static inline const std::shared_ptr<property_type const> uint_property = simple_property(1, "uint", "floatBitsToUint(in_block0)");
  static inline const std::shared_ptr<property_type const> uvec2_property = simple_property(2, "uvec2", "uvec2(floatBitsToUint(in_block0), floatBitsToUint(in_block1))");
  static inline const std::shared_ptr<property_type const> uvec3_property = simple_property(3, "uvec3", "uvec3(floatBitsToUint(in_block0), floatBitsToUint(in_block1), floatBitsToUint(in_block2))");
  static inline const std::shared_ptr<property_type const> uvec4_property = simple_property(4, "uvec4", "uvec4(floatBitsToUint(in_block0), floatBitsToUint(in_block1), floatBitsToUint(in_block2), floatBitsToUint(in_block3))");

  // packed
  static inline const std::shared_ptr<property_type const> unorm4x8_property = simple_property(1, "vec4", "unpackUnorm4x8(floatBitsToUint(in_block0))");
  static inline const std::shared_ptr<property_type const> unorm2x16_property = simple_property(1, "vec2", "unpackUnorm2x16(floatBitsToUint(in_block0))");
  static inline const std::shared_ptr<property_type const> snorm4x8_property = simple_property(1, "vec4", "unpackSnorm4x8(floatBitsToUint(in_block0))");
  static inline const std::shared_ptr<property_type const> snorm2x16_property = simple_property(1, "vec2", "unpackSnorm2x16(floatBitsToUint(in_block0))");
}