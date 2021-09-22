#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <memory>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include "parameter.hpp"
#include <myrt/sfml/utils.hpp>

namespace myrt
{
  namespace detail {
    template<typename... PType> requires (std::convertible_to<std::decay_t<PType>, std::shared_ptr<parameter>> && ...)
      std::array<std::shared_ptr<parameter>, sizeof...(PType)> plist(PType&&... params) {
      return {
        std::forward<PType>(params)...
      };
    }

    struct sdf_type {
      enum category {
        op, mod, prim
      };

      std::optional<int> find_parameter_index_by_name(std::string const& name) const;

      category instruction_category;
      std::string glsl_string;
      std::vector<std::shared_ptr<parameter_type>> type_params;
      std::vector<std::string> param_names;
    };

    struct sdf_instruction {

      sdf_instruction(std::shared_ptr<sdf_type> type);

      std::vector<std::shared_ptr<parameter>> params;
      std::shared_ptr<sdf_type> type;
      std::weak_ptr<sdf_instruction> joint_element;
    };

    struct sdf_prim : sdf_instruction, std::enable_shared_from_this<sdf_prim> {
      sdf_prim(std::shared_ptr<sdf_type> type);
      void set_parent(std::shared_ptr<sdf_instruction> p);

      std::shared_ptr<sdf_instruction> child;
    };

    struct sdf_op : sdf_instruction, std::enable_shared_from_this<sdf_op> {
      sdf_op(std::shared_ptr<sdf_type> type);
      void set_lhs(std::shared_ptr<sdf_instruction> l);
      void set_rhs(std::shared_ptr<sdf_instruction> r);

      std::shared_ptr<sdf_instruction> lhs;
      std::shared_ptr<sdf_instruction> rhs;
    };

    struct sdf_mod : sdf_instruction, std::enable_shared_from_this<sdf_mod> {
      sdf_mod(std::shared_ptr<sdf_type> type);
      void set_parent(std::shared_ptr<sdf_instruction> p);

      std::shared_ptr<sdf_instruction> child;
    };

    std::shared_ptr<sdf_type> sdf_create_type(sdf_type::category type, std::string_view glsl, std::span<std::shared_ptr<parameter_type> const> params);
    std::shared_ptr<sdf_type> sdf_create_type(sdf_type::category type, std::string_view glsl, std::initializer_list<std::shared_ptr<parameter_type>> params);
    std::shared_ptr<sdf_type> sdf_create_type(sdf_type::category type, std::string_view glsl, std::shared_ptr<parameter_type> const& param);

    template<std::derived_from<sdf_instruction> T>
    std::shared_ptr<T> sdf_create_instance(std::shared_ptr<sdf_type> type, std::span<std::shared_ptr<parameter> const> params)
    {
      auto const mod = std::make_shared<T>(std::move(type));
      mod->params.insert(mod->params.end(), std::begin(params), std::end(params));
      return mod;
    }

    template<std::derived_from<sdf_instruction> T>
    std::shared_ptr<T> sdf_create_instance(std::shared_ptr<sdf_type> type, std::initializer_list<std::shared_ptr<parameter>> params)
    {
      return sdf_create_instance<T>(std::move(type), std::span<std::shared_ptr<parameter> const>(params));
    }

    template<std::derived_from<sdf_instruction> T>
    std::shared_ptr<T> sdf_create_instance(std::shared_ptr<sdf_type> type, std::shared_ptr<parameter> const& param)
    {
      return sdf_create_instance<T>(std::move(type), { param });
    }

    template<typename ParamList>
    std::shared_ptr<sdf_mod> instantiate_modifier(std::shared_ptr<sdf_type> type, ParamList&& plist)
      requires requires(std::shared_ptr<sdf_type> type, ParamList&& plist) { sdf_create_instance<sdf_mod>(std::move(type), std::forward<ParamList>(plist)); }
    {
      return sdf_create_instance<sdf_mod>(std::move(type), std::forward<ParamList>(plist));
    }

    template<typename ParamList>
    std::shared_ptr<sdf_op> instantiate_operator(std::shared_ptr<sdf_type> type, ParamList&& plist)
      requires requires(std::shared_ptr<sdf_type> type, ParamList&& plist) { sdf_create_instance<sdf_op>(std::move(type), std::forward<ParamList>(plist)); }
    {
      return sdf_create_instance<sdf_op>(std::move(type), std::forward<ParamList>(plist));
    }

    template<typename ParamList>
    std::shared_ptr<sdf_prim> instantiate_primitive(std::shared_ptr<sdf_type> type, ParamList&& plist)
      requires requires(std::shared_ptr<sdf_type> type, ParamList&& plist) { sdf_create_instance<sdf_prim>(std::move(type), std::forward<ParamList>(plist)); }
    {
      return sdf_create_instance<sdf_prim>(std::move(type), std::forward<ParamList>(plist));
    }

    std::string resolve_parameter(
      std::shared_ptr<parameter> param,
      std::stringstream& functions,
      std::stringstream& distance_function,
      std::unordered_map<size_t, int>& hash_reducers,
      int& hash_counter,
      std::unordered_set<size_t>& used_objects,
      std::unordered_set<parameter_type*>& used_param_types,
      std::unordered_map<size_t, int>& buffer_offsets,
      int& current_offset);

    struct sdf_build_cache_t
    {
      std::stringstream functions;
      std::stringstream mul = std::stringstream("1.0");
      std::stringstream distance_function;
      std::unordered_set<size_t> used_objects;
      std::unordered_set<size_t> used_functions;
      std::unordered_set<parameter_type*> used_param_types;
      std::unordered_map<size_t, int> hash_reducers;
      int hash_counter = 0;
      int current_offset = 0;
      std::unordered_map<size_t, int> buffer_offsets;
    };

    std::string generate_glsl_impl(
      std::shared_ptr<sdf_instruction> const& root,
      sdf_build_cache_t& cache);

    constexpr bool is_space_or_newline(char c) {
      return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    constexpr bool is_newline(char c) {
      return c == '\n' || c == '\r';
    }

    constexpr bool is_operator(char c)
    {
      switch (c) {
      case '+':
      case '-':
      case '*':
      case '/':
      case '&':
      case '|':
      case '^':
      case '(':
      case ')':
      case '[':
      case ']':
      case '=':
      case '{':
      case '}':
      case '~':
      case '!':
      case '<':
      case '>':
      case '.':
      case ',':
      case ';':
      case '?':
      case ':':
        return true;
      }
      return false;
    }

    std::string minify_glsl(std::string const& original);
    std::string generate_glsl(std::shared_ptr<sdf_instruction> const& root, sdf_build_cache_t& cache);

    template<std::derived_from<sdf_instruction> T>
    std::string generate_glsl(std::shared_ptr<T> const& root, std::unordered_map<size_t, int>& buffer_offsets)
    {
      sdf_build_cache_t cache;
      auto c = generate_glsl(std::static_pointer_cast<sdf_instruction>(root), cache);
      buffer_offsets = cache.buffer_offsets;
      return c;
    }

    struct sdf_glsl_assembly
    {
      parameter_buffer_description buffer_description;

      void initialize_from_default(float* buf, std::shared_ptr<sdf_instruction> const& root);
    };

    struct sdf_glsl_assembler {
      constexpr static auto empty_sdf_glsl = "float map(vec3 p, inout _MT _mt) { switch(sdf_index_current) { default: return 1.0 / 0.0; }}";

      sdf_glsl_assembly append(std::shared_ptr<sdf_instruction> const& root);

      std::string get_assembled_glsl() const;

    private:
      void append_map_case(int id);

      void prepare_build_cache();

      int m_current_id = 0;
      std::string m_full_glsl;
      std::string m_map_function = empty_sdf_glsl;
      sdf_build_cache_t m_build_cache;
    };
  }
}

namespace myrt::sdf {
  struct prim;
  struct op;
  struct mod;
  using category = detail::sdf_type;
  using instruction = detail::sdf_instruction;
  using instruction_type = detail::sdf_type;
  using basic_primitive = detail::sdf_prim;
  using basic_operator = detail::sdf_op;
  using basic_modifier = detail::sdf_mod;
  using glsl_assembly = detail::sdf_glsl_assembly;
  using glsl_assembler = detail::sdf_glsl_assembler;

  template<std::derived_from<instruction> S>
  struct sdf {
    sdf(std::shared_ptr<instruction_type> t)
      : _ptr(std::make_shared<S>(t))
    {
      _ptr->params.resize(t->type_params.size());
      for (size_t i = 0; i < _ptr->params.size(); ++i)
      {
        _ptr->params[i] = std::make_shared<parameter>(t->type_params[i]);
      }
    }

    std::shared_ptr<parameter> get_parameter(std::string const& name) const {
      auto const index = _ptr->type.find_parameter_index_by_name(name);
      if (!index.has_value())
        return nullptr;
      return _ptr->params[index.value()];
    }

    std::shared_ptr<parameter> get_parameter(size_t index) const {
      if (index >= _ptr->params.size())
        return nullptr;
      return _ptr->params[index];
    }

    template<typename PType>
    std::shared_ptr<PType> get_parameter(size_t index) const requires std::is_base_of_v<parameter, PType> {
      std::shared_ptr<parameter> const& ptr = get_parameter(index);
      if (ptr && ptr->type == PType::type)
        return std::static_pointer_cast<PType>(ptr);
      return nullptr;
    }

    std::shared_ptr<parameter> link_parameter(std::shared_ptr<parameter> const& target, std::shared_ptr<parameter> param) {
      if (!target)
        return nullptr;

      auto const result = target->link_value_block(0, param, 0).other;
      for (size_t i = 1; i < target->type->buffer_blocks; ++i) {
        target->link_value_block(i, param, i);
      }
      return result;
    }

    template<typename PType>
    std::shared_ptr<PType> link_parameter(std::shared_ptr<PType> const& target, std::shared_ptr<PType> param) requires std::is_base_of_v<parameter, PType> {
      if (!target)
        return nullptr;

      auto const result = std::static_pointer_cast<PType>(target->link_value_block(0, param, 0).other);
      for (size_t i = 1; i < PType::type->buffer_blocks; ++i) {
        target->link_value_block(i, param, i);
      }
      return result;
    }

    void unlink_parameter(size_t index) {
      auto const target = get_parameter(index);
      if (!target)
        return;

      for (size_t i = 0; i < target->type->buffer_blocks; ++i) {
        target->unlink_value_block(i);
      }
    }

    operator std::shared_ptr<S> const& () const {
      return _ptr;
    }
    std::shared_ptr<S> const& get_pointer() const {
      return _ptr;
    }

  protected:
    std::shared_ptr<S> _ptr;

  public:

    template<size_t Index, typename TValue, typename TParam> requires std::is_base_of_v<parameter, TParam>
      struct static_parameter
      {
        using param_type = TParam;
        using value_type = TValue;
        constexpr static size_t index = Index;

        constexpr static_parameter() = default;

        template<typename T>
        void set_default(std::shared_ptr<S> const& _ptr, T&& value) {
          glsl_assembly temp_asm{};
          temp_asm.buffer_description.set_value(nullptr, _ptr->params[index], std::forward<T>(value));
        }
      };

      template<size_t Index, typename TValue, typename TParam>
      std::shared_ptr<TParam> get_parameter(static_parameter<Index, TValue, TParam> const&) const {
        return get_parameter<TParam>(Index);
      }

      template<size_t Index, typename TValue, typename TParam>
      std::shared_ptr<TParam> link(static_parameter<Index, TValue, TParam> const&, std::shared_ptr<TParam> param) {
        return link_parameter(Index, std::move(param));
      }

      template<size_t Index, typename TValue, typename TParam>
      void unlink(static_parameter<Index, TValue, TParam> const&) {
        return unlink_parameter(Index);
      }

      auto operator[](size_t index) const { return get_parameter(index); }
      auto operator[](std::string const& name) const { return get_parameter(name); }
      template<size_t Index, typename TValue, typename TParam>
      auto operator[](static_parameter<Index, TValue, TParam> const& par) const { return get_parameter(par); }
  };

  struct basic_type {
    basic_type(category::category t, std::string_view glsl, std::span<std::shared_ptr<parameter_type> const> params, std::span<std::string const> param_names = {});
    basic_type(instruction_type::category type, std::string_view glsl, std::initializer_list<std::shared_ptr<parameter_type>> params, std::initializer_list<std::string> param_names = {});
    basic_type(instruction_type::category type, std::string_view glsl, std::shared_ptr<parameter_type> param, std::string param_name = "");
    operator std::shared_ptr<instruction_type> const& () const;

  private:
    std::shared_ptr<instruction_type> _type;
  };

  struct prim : sdf<basic_primitive> {
    prim(basic_type type);

    prim& transform(sdf<basic_modifier> m);
  };

  struct op : sdf<basic_operator> {
    op(basic_type type);

    op& set_left(sdf<basic_operator> lhs);
    op& set_right(sdf<basic_operator> rhs);
    op& set_left(sdf<basic_primitive> lhs);
    op& set_right(sdf<basic_primitive> rhs);

    template<typename Lhs, typename Rhs>
    op& apply(Lhs lhs, Rhs rhs)
      requires (std::is_base_of_v<sdf<basic_operator>, Lhs> || std::is_base_of_v<sdf<basic_primitive>, Lhs>) &&
      (std::is_base_of_v<sdf<basic_operator>, Rhs> || std::is_base_of_v<sdf<basic_primitive>, Rhs>) {

      set_left(std::move(lhs));
      set_right(std::move(rhs));
      return *this;
    }
  };

  struct mod : sdf<basic_modifier> {
    mod(basic_type type);
    mod& transform(sdf<basic_modifier> m);
  };



  //
  //
  //  TODO: Move do separate file...
  //
  //



  struct sphere : prim {
    static inline const basic_type sphere_type = basic_type{
      instruction_type::category::prim,
      "inout_material = load_material(in_param1); return length(in_position) - in_param0;",
      { float_param::type, int_param::type },
      { "radius", "material" }
    };

    sphere(float radius = 1.0f, int material_index = 0) : prim(sphere_type) {
      this->radius.set_default(_ptr, radius);
      this->material.set_default(_ptr, material_index);
    }

    [[no_unique_address]] static_parameter<0, float, float_param> radius{};
    [[no_unique_address]] static_parameter<1, int, int_param> material{};
  };

  struct translate : mod {
    static inline const basic_type translate_type = basic_type{
      instruction_type::category::mod, "return in_position - in_param0;", { vec3_param::type },
      { "offset" }
    };

    translate(rnu::vec3 offset = {}) : mod(translate_type) {
      this->offset.set_default(_ptr, offset);
    }

    [[no_unique_address]] static_parameter<0, rnu::vec3, vec3_param> offset{};
  };

  struct hard_union : op {
    static inline const basic_type hard_union_type = basic_type{
      instruction_type::category::op, R"(
float m = min(in_distance0, in_distance1);
inout_material = mix_material(in_material0, in_material1, float(m == in_distance1));
return m;)", {  }
    };

    hard_union() : op(hard_union_type) {}
  };

  struct hard_subtraction : op {
    static inline const basic_type hard_subtraction_type = basic_type{
      instruction_type::category::op, R"(
float m = max(-in_distance0,in_distance1);
inout_material = mix_material(in_material0, in_material1, float(m == in_distance1));
return m;)", {  }
    };

    hard_subtraction() : op(hard_subtraction_type) {}
  };

  struct hard_intersection : op {
    static inline const basic_type hard_intersection_type = basic_type{
      instruction_type::category::op, R"(
float m = max(in_distance0,in_distance1);
inout_material = mix_material(in_material0, in_material1, float(m == in_distance1));
return m;)", {  }
    };

    hard_intersection() : op(hard_intersection_type) {}
  };

  struct smooth_union : op {
    constexpr static auto glsl = R"(
float d1 = in_distance0;
float d2 = in_distance1;
float k = in_param0;
float h = clamp(0.5 + 0.5 * (d2 - d1) / k, 0.0, 1.0);

inout_material = mix_material(in_material0, in_material1, 1.0-h);

return mix(d2, d1, h) - k * h * (1.0 - h);)";
    static inline const basic_type smooth_union_type = basic_type{
      instruction_type::category::op, glsl, { float_param::type },
      { "factor" }
    };

    smooth_union(float factor = 0.1f) : op(smooth_union_type) {
      this->factor.set_default(_ptr, factor);
    }

    [[no_unique_address]] static_parameter<0, float, float_param> factor{};
  };

  struct smooth_subtraction : op {
    constexpr static auto glsl = R"(
float d1 = in_distance0;
float d2 = in_distance1;
float k = in_param0;
inout_material = mix_material(in_material0, in_material1, 1.0-h);
float h = clamp( 0.5 - 0.5*(d2+d1)/k, 0.0, 1.0 );
return mix( d2, -d1, h ) + k*h*(1.0-h);)";
    static inline const basic_type smooth_subtraction_type = basic_type{
      instruction_type::category::op, glsl, { float_param::type },
      { "factor" }
    };

    smooth_subtraction(float factor = 0.1f) : op(smooth_subtraction_type) {
      this->factor.set_default(_ptr, factor);
    }

    [[no_unique_address]] static_parameter<0, float, float_param> factor{};
  };

  struct smooth_intersection : op {
    constexpr static auto glsl = R"(
float d1 = in_distance0;
float d2 = in_distance1;
float k = in_param0;
inout_material = mix_material(in_material0, in_material1, 1.0-h);
float h = clamp( 0.5 - 0.5*(d2-d1)/k, 0.0, 1.0 );
return mix( d2, d1, h ) + k*h*(1.0-h);)";
    static inline const basic_type smooth_intersection_type = basic_type{
      instruction_type::category::op, glsl, { float_param::type },
      { "factor" }
    };

    smooth_intersection(float factor = 0.1f) : op(smooth_intersection_type) {
      this->factor.set_default(_ptr, factor);
    }

    [[no_unique_address]] static_parameter<0, float, float_param> factor{};
  };

  struct box : prim {
    static inline const basic_type box_type = basic_type{
      instruction_type::category::prim, R"(
  vec3 b = in_param0;
  vec3 p = in_position;
inout_material = load_material(in_param1); 
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);)", {vec3_param::type, int_param::type},
        { "size", "material" }
    };

    box(rnu::vec3 size = rnu::vec3{ 1,1,1 }, int material_index = 0) : prim(box_type) {
      this->size.set_default(_ptr, size);
      this->material.set_default(_ptr, material_index);
    }

    [[no_unique_address]] static_parameter<0, rnu::vec3, vec3_param> size{};
    [[no_unique_address]] static_parameter<1, int, int_param> material{};
  };

  struct torus : prim {
    static inline const basic_type torus_type = basic_type{
      instruction_type::category::prim, R"(
  float rlarge = in_param0;
  float rsmall = in_param1;
inout_material = load_material(in_param2);
  vec3 p = in_position; 
  vec2 q = vec2(length(p.xz)-rlarge,p.y);
  return length(q)-rsmall;)", {float_param::type, float_param::type, int_param::type},
        { "radius_large", "radius_small", "material" }
    };

    torus(float radius_large = 1.f, float radius_small = 0.2f, int material_index = 0) : prim(torus_type) {
      this->radius_large.set_default(_ptr, radius_large);
      this->radius_small.set_default(_ptr, radius_small);
      this->material.set_default(_ptr, material_index);
    }

    [[no_unique_address]] static_parameter<0, float, float_param> radius_large{};
    [[no_unique_address]] static_parameter<1, float, float_param> radius_small{};
    [[no_unique_address]] static_parameter<2, int, int_param> material{};
  };

  struct menger_fractal : prim {
    static inline const basic_type menger_fractal_type = basic_type{
      instruction_type::category::prim, R"(vec3 p = in_position;
inout_material = load_material(in_param0); 
		for (int n = 0; n < 4; n++)
		{
		  p = abs(p);

		  if (p.x<p.y) p.xy = p.yx;
		  if (p.x<p.z) p.xz = p.zx;
		  if (p.y<p.z) p.zy = p.yz;

		  p.z -= 1. / 3.;
		  p.z = -abs(p.z);
		  p.z += 1. / 3.;

		  p *= 3.;
		  p.x -= 2.;
		  p.y -= 2.;
		}

		vec3 d = abs(p) - vec3(1);
		float dis = min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));

		dis *= pow(3., float(-4));

		return dis;)", {int_param::type},
        { "material" }
    };

    menger_fractal(int material_index = 0) : prim(menger_fractal_type) {
      this->material.set_default(_ptr, material_index);
    }

    [[no_unique_address]] static_parameter<0, int, int_param> material{};
  };

  struct vertical_capsule : prim {
    static inline const basic_type vertical_capsule_type = basic_type{
      instruction_type::category::prim, R"(inout_material = load_material(in_param2); in_position.y -= clamp( in_position.y, 0.0, in_param0 ); return length( in_position ) - in_param1;)",
      { float_param::type, float_param::type, int_param::type },
      { "height", "radius", "material" }
    };

    vertical_capsule(float height = 1.0f, float radius = 0.2f, int material = 0) : prim(vertical_capsule_type) {
      this->height.set_default(_ptr, height);
      this->radius.set_default(_ptr, radius);
      this->material.set_default(_ptr, material);
    }
    [[no_unique_address]] static_parameter<0, float, float_param> height{};
    [[no_unique_address]] static_parameter<1, float, float_param> radius{};
    [[no_unique_address]] static_parameter<2, int, int_param> material{};
  };

  struct mirror_axis : mod {
    static inline const basic_type mirror_axis_type = basic_type{
      instruction_type::category::mod, R"(in_position[in_param0] = abs(in_position[in_param0]); return in_position;)",
      { int_param::type },
      { "axis" }
    };

    mirror_axis(int axis) : mod(mirror_axis_type) {
      this->axis.set_default(_ptr, axis);
    }

    [[no_unique_address]] static_parameter<0, int, int_param> axis{};
  };
}