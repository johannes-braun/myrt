#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <memory>

namespace myrt
{
  struct sdf_param {
    enum class type {
      gl_int,
      gl_uint,
      gl_float,
      gl_vec2,
      gl_vec3,
      gl_vec4,
      gl_mat2,
      gl_mat4
    };

    constexpr sdf_param(type t) : param_type(t) {}

    std::string name() const {
      switch (param_type) {
      case type::gl_int: return "int";
      case type::gl_uint: return "uint";
      case type::gl_float: return "float";
      case type::gl_vec2: return "vec2";
      case type::gl_vec3: return "vec3";
      case type::gl_vec4: return "vec4";
      case type::gl_mat2: return "mat2";
      case type::gl_mat4: return "mat4";
      }
    }

    int blocks() const {
      switch (param_type) {
      case type::gl_int: 
      case type::gl_uint:
      case type::gl_float:
        return 1;
      case type::gl_vec2:
        return 2;
      case type::gl_vec3:
        return 3;
      case type::gl_vec4:
        return 4;
      case type::gl_mat2:
        return 4;
      case type::gl_mat4:
        return 16;
      }
    }

    type param_type;
  };
#define param_struct(name) struct name##_param : sdf_param { constexpr name##_param() : sdf_param{ type::gl_##name } {}
#define end_param(name) }; std::shared_ptr<name##_param> make_##name##_param() { return std::make_shared<name##_param>(); }

  param_struct(uint);
  constexpr static std::string_view load_from_addr = "uint sdrd_uint(int addr){return floatBitsToUint(MYRT_SDF_BUF_GET(addr++)); }";
  end_param(uint);

  param_struct(int);
  constexpr static std::string_view load_from_addr = "int sdrd_int(int addr){return floatBitsToInt(MYRT_SDF_BUF_GET(addr++)); }";
  end_param(int);

  param_struct(float);
  constexpr static std::string_view load_from_addr = "float sdrd_float(int addr){return MYRT_SDF_BUF_GET(addr++);}";
  end_param(float);

  param_struct(vec2);
  constexpr static std::string_view load_from_addr = "vec2 sdrd_vec2(int addr){return vec2(MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++));}";
  end_param(vec2);

  param_struct(vec3);
  constexpr static std::string_view load_from_addr = "vec3 sdrd_vec3(int addr){return vec3(MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++));}";
  end_param(vec3);

  param_struct(vec4);
  constexpr static std::string_view load_from_addr = "vec4 sdrd_vec4(int addr){return vec4(MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++));}";
  end_param(vec4);

  param_struct(mat2);
  constexpr static std::string_view load_from_addr = "mat2 sdrd_mat2(int addr){return mat2(MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++));}";
  end_param(mat2);

  param_struct(mat4);
  constexpr static std::string_view load_from_addr = "mat4 sdrd_mat4(int addr){return mat4(MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++),MYRT_SDF_BUF_GET(addr++));}";
  end_param(mat4);

  constexpr sdf_param from_typename(std::string const& name) {
#define name_entry(N) if (name == #N ) return sdf_param{ sdf_param::type::gl_##N };
    name_entry(int);
    name_entry(uint);
    name_entry(float);
    name_entry(vec2);
    name_entry(vec3);
    name_entry(vec4);
    name_entry(mat2);
    name_entry(mat4);
#undef name_entry
  }

  inline std::string_view get_function_string(sdf_param const& param) {
#define case_type(N) case sdf_param::type::gl_##N: return N##_param::load_from_addr;
    switch (param.param_type) {
      case_type(int);
      case_type(uint);
      case_type(float);
      case_type(vec2);
      case_type(vec3);
      case_type(vec4);
      case_type(mat2);
      case_type(mat4);
    }
#undef case_type
  }

  struct sdf_instruction {
    enum class type {
      op, mod, prim
    };

    constexpr sdf_instruction(type ty)
      : instruction_type(ty) {

    }
    type instruction_type;
    std::vector<std::shared_ptr<sdf_param>> params;
    std::string glsl_string;
    std::optional<std::string> glsl_mul_string;
    std::shared_ptr<sdf_instruction> child;
  };

  struct sdf_prim :  sdf_instruction, std::enable_shared_from_this<sdf_prim> {
    sdf_prim() : sdf_instruction(type::prim) {}

    void set_parent(std::shared_ptr<sdf_instruction> p) {
      parent = p;
      if (p != nullptr)
        p->child = shared_from_this();
    }

    std::weak_ptr<sdf_instruction> parent;
  };

  struct sdf_op :  sdf_instruction, std::enable_shared_from_this<sdf_op> {
    sdf_op() : sdf_instruction(type::op) {}

    void set_lhs(std::shared_ptr<sdf_instruction> l) {
      lhs = l;
      if (l != nullptr)
        l->child = shared_from_this();
    }

    void set_rhs(std::shared_ptr<sdf_instruction> r) {
      rhs = r;
      if (r != nullptr)
        r->child = shared_from_this();
    }

    std::weak_ptr<sdf_instruction> lhs;
    std::weak_ptr<sdf_instruction> rhs;
  };

  struct sdf_mod :  sdf_instruction, std::enable_shared_from_this<sdf_mod> {
    sdf_mod() : sdf_instruction(type::mod) {}

    void set_parent(std::shared_ptr<sdf_instruction> p) {
      parent = p;
      if (p != nullptr)
        p->child = shared_from_this();
    }

    std::weak_ptr<sdf_instruction> parent;
  };

  inline std::shared_ptr<sdf_mod> translate(std::shared_ptr<vec3_param> offset)
  {
    // in_position, inout_buf_addr
    auto sdf = std::make_shared<sdf_mod>();
    sdf->glsl_string = "return in_position - in_param0;";
    sdf->params.push_back(std::move(offset));
    return sdf;
  }

  inline std::shared_ptr<sdf_prim> sphere(std::shared_ptr<float_param> radius)
  {
    // in_position, inout_buf_addr
    auto sdf = std::make_shared<sdf_prim>();
    sdf->glsl_string = "return length(in_position) - in_param0;";
    sdf->params.push_back(std::move(radius));
    return sdf;
  }

  inline std::shared_ptr<sdf_op> smooth_union(std::shared_ptr<float_param> size)
  {
    // in_position, inout_buf_addr
    auto sdf = std::make_shared<sdf_op>();
    sdf->glsl_string = R"(
float d1 = in_distance0;
float d2 = in_distance1;
float k = in_param0;
float h = clamp(0.5 + 0.5 * (d2 - d1) / k, 0.0, 1.0);
return mix(d2, d1, h) - k * h * (1.0 - h);)";
    sdf->params.push_back(std::move(size));
    return sdf;
  }

  /*
  mod:
    modname %p %bufoffset -> %p'

  prim:
    primname %p %bufoffset -> %d

  op:
    opname %d_1 %d_2 %bufoffset
  */
}