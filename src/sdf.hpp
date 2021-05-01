#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <memory>

namespace myrt
{
  struct sdf_parameter_type
  {
    sdf_parameter_type(std::string type_name, int buffer_blocks, std::string buffer_load)
      : type_name(type_name), buffer_blocks(buffer_blocks) {
        function = type_name + " _r" + type_name + "(";
        for (int i = 0; i < buffer_blocks; ++i)
        {
          if (i != 0)
            function += ",";
          function += "float in_block" + std::to_string(i);
        }
        function += "){" + buffer_load + "\n;}";
    }

    std::string type_name;
    int buffer_blocks;
    std::string function;
  };

  struct sdf_parameter;
  struct sdf_parameter_link {
    enum class type {
      link,
      offset
    };

    bool is_linked() const {
      return other != nullptr;
    }

    size_t hash() const;

    std::shared_ptr<sdf_parameter> other;
    size_t block;
  };

  struct sdf_parameter {
    sdf_parameter(std::shared_ptr<sdf_parameter_type> type)
      : type(std::move(type)) {
      _value_links.resize(this->type->buffer_blocks);
    }

    std::shared_ptr<sdf_parameter_type> type;

    void link_value_block(size_t block, std::shared_ptr<sdf_parameter> other, size_t other_block) {
      _value_links[block].other = std::move(other);
      _value_links[block].block = other_block;
    }

    void unlink_value_block(size_t block) {
      _value_links[block].other.reset();
      _value_links[block].block = 0;
    }

    sdf_parameter_link const& get_link(size_t block) const {
      return _value_links[block];
    }

  private:
    std::vector<sdf_parameter_link> _value_links;
  };

  inline size_t sdf_parameter_link::hash() const {
    return std::hash<std::shared_ptr<sdf_parameter>>{}(other) * 37 +
      std::hash<size_t>{}(block);
  }

  inline std::shared_ptr<sdf_parameter_type> create_parameter_type(std::string type_name, int buffer_blocks, std::string buffer_load)
  {
    auto const ptr = std::make_shared<sdf_parameter_type>(std::move(type_name), buffer_blocks, std::move(buffer_load));
  }

#define def_param_type(name, blk, glsl) \
  struct name##_type :sdf_parameter_type { name##_type() :sdf_parameter_type(#name, (blk), (glsl)) {} };\
  struct name##_param :sdf_parameter { \
  static inline const std::shared_ptr<name##_type> type = std::make_shared<name##_type>(); \
  name##_param() : sdf_parameter(type) {} };\
  inline std::shared_ptr<name##_param> make_##name##_param() { return std::make_shared<name##_param>(); }
  
  def_param_type(uint, 1, "return uint(in_block0)");
  def_param_type(int, 1, "return int(in_block0)");
  def_param_type(float, 1, "return in_block0");
  def_param_type(vec2, 2, "return vec3(in_block0, in_block1)");
  def_param_type(vec3, 3, "return vec3(in_block0, in_block1, in_block2)");
  def_param_type(vec4, 4, "return vec3(in_block0, in_block1, in_block2, in_block3)");

  //
  //  struct sdf_param {
  //    enum class type {
  //      gl_int,
  //      gl_uint,
  //      gl_float,
  //      gl_vec2,
  //      gl_vec3,
  //      gl_vec4,
  //      gl_mat2,
  //      gl_mat4
  //    };
  //
  //    constexpr sdf_param(type t) : param_type(t) {}
  //
  //    constexpr std::string_view name() const {
  //      switch (param_type) {
  //      case type::gl_int:   return "int";
  //      case type::gl_uint:  return "uint";
  //      case type::gl_float: return "float";
  //      case type::gl_vec2:  return "vec2";
  //      case type::gl_vec3:  return "vec3";
  //      case type::gl_vec4:  return "vec4";
  //      case type::gl_mat2:  return "mat2";
  //      case type::gl_mat4:  return "mat4";
  //      }
  //    }
  //
  //    constexpr std::string_view short_name() const {
  //      switch (param_type) {
  //      case type::gl_int: return "i";
  //      case type::gl_uint: return "u";
  //      case type::gl_float: return "f";
  //      case type::gl_vec2: return "v2";
  //      case type::gl_vec3: return "v3";
  //      case type::gl_vec4: return "v4";
  //      case type::gl_mat2: return "m2";
  //      case type::gl_mat4: return "m4";
  //      }
  //    }
  //
  //    constexpr int blocks() const {
  //      switch (param_type) {
  //      case type::gl_int:
  //      case type::gl_uint:
  //      case type::gl_float:
  //        return 1;
  //      case type::gl_vec2:
  //        return 2;
  //      case type::gl_vec3:
  //        return 3;
  //      case type::gl_vec4:
  //        return 4;
  //      case type::gl_mat2:
  //        return 4;
  //      case type::gl_mat4:
  //        return 16;
  //      }
  //    }
  //
  //    type param_type;
  //  };
  //#define param_struct(name) struct name##_param : sdf_param { constexpr name##_param() : sdf_param{ type::gl_##name } {}
  //#define end_param(name) }; std::shared_ptr<name##_param> make_##name##_param() { return std::make_shared<name##_param>(); }
  //
  //  param_struct(uint);
  //  constexpr static std::string_view load_from_addr = "uint _ru(int a){return floatBitsToUint(_G(a++)); }";
  //  end_param(uint);
  //
  //  param_struct(int);
  //  constexpr static std::string_view load_from_addr = "int _ri(int a){return floatBitsToInt(_G(a++)); }";
  //  end_param(int);
  //
  //  param_struct(float);
  //  constexpr static std::string_view load_from_addr = "float _rf(int a){return _G(a++);}";
  //  end_param(float);
  //
  //  param_struct(vec2);
  //  constexpr static std::string_view load_from_addr = "vec2 _rv2(int a){return vec2(_G(a++),_G(a++));}";
  //  end_param(vec2);
  //
  //  param_struct(vec3);
  //  constexpr static std::string_view load_from_addr = "vec3 _rv3(int a){return vec3(_G(a++),_G(a++),_G(a++));}";
  //  end_param(vec3);
  //
  //  param_struct(vec4);
  //  constexpr static std::string_view load_from_addr = "vec4 _rv4(int a){return vec4(_G(a++),_G(a++),_G(a++),_G(a++));}";
  //  end_param(vec4);
  //
  //  param_struct(mat2);
  //  constexpr static std::string_view load_from_addr = "mat2 _rm2(int a){return mat2(_G(a++),_G(a++),_G(a++),_G(a++));}";
  //  end_param(mat2);
  //
  //  param_struct(mat4);
  //  constexpr static std::string_view load_from_addr = "mat4 _rm4(int a){return mat4(_G(a++),_G(a++),_G(a++),_G(a++),_G(a++),_G(a++),_G(a++),_G(a++),_G(a++),_G(a++),_G(a++),_G(a++),_G(a++),_G(a++),_G(a++),_G(a++));}";
  //  end_param(mat4);
  //
  //  template<typename T>
  //  constexpr auto cast_down(std::shared_ptr<T> const& ptr) requires std::is_base_of_v<sdf_param, T> {
  //    return std::static_pointer_cast<sdf_param>(ptr);
  //  }
  //
    template<typename... PType> requires (std::convertible_to<std::decay_t<PType>, std::shared_ptr<sdf_parameter>> && ...)
      std::array<std::shared_ptr<sdf_parameter>, sizeof...(PType)> plist(PType&&... params) {
      return {
        std::forward<PType>(params)...
      };
    }
  //
  //  constexpr sdf_param from_typename(std::string const& name) {
  //#define name_entry(N) if (name == #N ) return sdf_param{ sdf_param::type::gl_##N };
  //    name_entry(int);
  //    name_entry(uint);
  //    name_entry(float);
  //    name_entry(vec2);
  //    name_entry(vec3);
  //    name_entry(vec4);
  //    name_entry(mat2);
  //    name_entry(mat4);
  //#undef name_entry
  //  }
  //
  //  inline std::string_view get_function_string(sdf_param const& param) {
  //#define case_type(N) case sdf_param::type::gl_##N: return N##_param::load_from_addr;
  //    switch (param.param_type) {
  //      case_type(int);
  //      case_type(uint);
  //      case_type(float);
  //      case_type(vec2);
  //      case_type(vec3);
  //      case_type(vec4);
  //      case_type(mat2);
  //      case_type(mat4);
  //    }
  //#undef case_type
  //  }

  struct sdf_type {
    enum class type {
      op, mod, prim
    };

    type instruction_type;
    std::string glsl_string;
    std::vector<std::shared_ptr<sdf_parameter_type>> type_params;
  };

  struct sdf_instruction {

    sdf_instruction(std::shared_ptr<sdf_type> type)
      : type(type) {

    }

    std::vector<std::shared_ptr<sdf_parameter>> params;
    std::shared_ptr<sdf_type> type;
    std::shared_ptr<sdf_instruction> child;
  };

  struct sdf_prim : sdf_instruction, std::enable_shared_from_this<sdf_prim> {
    sdf_prim(std::shared_ptr<sdf_type> type) : sdf_instruction(type) {}

    void set_parent(std::shared_ptr<sdf_instruction> p) {
      parent = p;
      if (p != nullptr)
        p->child = shared_from_this();
    }

    std::weak_ptr<sdf_instruction> parent;
  };

  struct sdf_op : sdf_instruction, std::enable_shared_from_this<sdf_op> {
    sdf_op(std::shared_ptr<sdf_type> type) : sdf_instruction(type) {}

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

  struct sdf_mod : sdf_instruction, std::enable_shared_from_this<sdf_mod> {
    sdf_mod(std::shared_ptr<sdf_type> type) : sdf_instruction(type) {}

    void set_parent(std::shared_ptr<sdf_instruction> p) {
      parent = p;
      if (p != nullptr)
        p->child = shared_from_this();
    }

    std::weak_ptr<sdf_instruction> parent;
  };

  inline std::shared_ptr<sdf_type> sdf_create_type(sdf_type::type type, std::string_view glsl, std::span<std::shared_ptr<sdf_parameter_type> const> params)
  {
    auto const mod = std::make_shared<sdf_type>();
    mod->instruction_type = type;
    mod->glsl_string = glsl;
    mod->type_params.insert(mod->type_params.end(), std::begin(params), std::end(params));
    return mod;
  }

  inline std::shared_ptr<sdf_type> sdf_create_type(sdf_type::type type, std::string_view glsl, std::initializer_list<std::shared_ptr<sdf_parameter_type>> params)
  {
    return sdf_create_type(type, std::move(glsl), std::span<std::shared_ptr<sdf_parameter_type> const>(params));
  }

  inline std::shared_ptr<sdf_type> sdf_create_type(sdf_type::type type, std::string_view glsl, std::shared_ptr<sdf_parameter_type> const& param)
  {
    return sdf_create_type(type, std::move(glsl), std::initializer_list<std::shared_ptr<sdf_parameter_type>>{ param });
  }

  template<typename T>
  inline std::shared_ptr<T> sdf_create_instance(std::shared_ptr<sdf_type> type, std::span<std::shared_ptr<sdf_parameter> const> params) requires std::is_base_of_v<sdf_instruction, T>
  {
    auto const mod = std::make_shared<T>(std::move(type));
    mod->params.insert(mod->params.end(), std::begin(params), std::end(params));
    return mod;
  }

  template<typename T>
  inline std::shared_ptr<T> sdf_create_instance(std::shared_ptr<sdf_type> type, std::initializer_list<std::shared_ptr<sdf_parameter>> params) requires std::is_base_of_v<sdf_instruction, T>
  {
    return sdf_create_instance<T>(std::move(type), std::span<std::shared_ptr<sdf_parameter> const>(params));
  }

  template<typename T>
  inline std::shared_ptr<T> sdf_create_instance(std::shared_ptr<sdf_type> type, std::shared_ptr<sdf_parameter> const& param) requires std::is_base_of_v<sdf_instruction, T>
  {
    return sdf_create_instance<T>(std::move(type), { param });
  }

  template<typename ParamList>
  inline std::shared_ptr<sdf_mod> instantiate_modifier(std::shared_ptr<sdf_type> type, ParamList&& plist)
    requires requires(std::shared_ptr<sdf_type> type, ParamList&& plist) { sdf_create_instance<sdf_mod>(std::move(type), std::forward<ParamList>(plist)); }
  {
    return sdf_create_instance<sdf_mod>(std::move(type), std::forward<ParamList>(plist));
  }

  template<typename ParamList>
  inline std::shared_ptr<sdf_op> instantiate_operator(std::shared_ptr<sdf_type> type, ParamList&& plist)
    requires requires(std::shared_ptr<sdf_type> type, ParamList&& plist) { sdf_create_instance<sdf_op>(std::move(type), std::forward<ParamList>(plist)); }
  {
    return sdf_create_instance<sdf_op>(std::move(type), std::forward<ParamList>(plist));
  }

  template<typename ParamList>
  inline std::shared_ptr<sdf_prim> instantiate_primitive(std::shared_ptr<sdf_type> type, ParamList&& plist)
    requires requires(std::shared_ptr<sdf_type> type, ParamList&& plist) { sdf_create_instance<sdf_prim>(std::move(type), std::forward<ParamList>(plist)); }
  {
    return sdf_create_instance<sdf_prim>(std::move(type), std::forward<ParamList>(plist));
  }

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

  inline std::string resolve_parameter(
    std::shared_ptr<sdf_parameter> param,

    std::stringstream& functions,
    std::stringstream& distance_function,
    std::unordered_map<size_t, int>& hash_reducers,
    int& hash_counter,
    std::unordered_set<size_t>& used_objects,
    std::unordered_set<sdf_parameter_type*>& used_param_types,
    std::unordered_map<size_t, int>& buffer_offsets,
    int& current_offset)
  {
    auto param_hash = hash_reduced(param, hash_reducers, hash_counter);
    std::string param_name = "par" + to_hex_string(param_hash);
    if (used_objects.emplace(param_hash).second) {
      std::vector<std::string> block_names(param->type->buffer_blocks);

      for (size_t i = 0; i < param->type->buffer_blocks; ++i)
      {
        sdf_parameter_link self{ param, i };
        auto const& link = param->get_link(i);
        size_t const own_hash = self.hash();

        if (!link.is_linked())
        {
          // unlinked. use static block offset
          buffer_offsets[own_hash] = current_offset;
          block_names[i] = "_bk" + std::to_string(current_offset);
          distance_function << "float _bk" << current_offset << "=_G(" << current_offset << ");";
          current_offset++;
        }
        else
        {
          // resolve link.
          auto const other_hash = link.hash();
          if (auto const it = buffer_offsets.find(other_hash); it != buffer_offsets.end())
          {
            block_names[i] = "_bk" + std::to_string(it->second);
          }
          else
          {
            // link is not yet resolved. 
            auto const param_name = resolve_parameter(link.other,
              functions, distance_function, hash_reducers, hash_counter, used_objects, used_param_types, buffer_offsets, current_offset);

            auto const next_try = buffer_offsets.find(other_hash);
            block_names[i] = "_bk" + std::to_string(next_try->second);
          }
        }
      }

      distance_function << param->type->type_name << " " << param_name << "=_r" << param->type->type_name << "(";

      for (int i = 0; i < block_names.size(); ++i)
      {
        if (i != 0)
          distance_function << ",";
        distance_function << block_names[i];
      }

      distance_function << ");";
    }
    return param_name;
  }

  inline std::string generate_glsl(
    std::shared_ptr<sdf_instruction> const& root,
    std::stringstream& functions,
    std::stringstream& distance_function,
    std::stringstream& mul,
    std::unordered_map<size_t, int>& hash_reducers,
    int& hash_counter,
    std::unordered_set<size_t>& used_objects,
    std::unordered_set<sdf_parameter_type*>& used_param_types,
    std::unordered_map<size_t, int>& buffer_offsets,
    int& current_offset)
  {
    const auto hash = hash_reduced(root->type, hash_reducers, hash_counter);
    const auto fun_name = "_X" + to_hex_string(hash);
    if (used_objects.emplace(hash).second) {

      for (auto const& par : root->type->type_params)
      {
        if (used_param_types.emplace(par.get()).second)
          functions << par->function;
      }

      switch (root->type->instruction_type)
      {
      case sdf_type::type::prim:
        functions << "float " << fun_name << "(vec3 _L, inout _MT _mt";
        break;
      case sdf_type::type::op:
        functions << "float " << fun_name << "(float _D0, float _D1, _MT _mt0, _MT _mt1, inout _MT _mt";
        break;
      case sdf_type::type::mod:
        functions << "vec3 " << fun_name << "(vec3 _L, out float _M";
        break;
      }

      int i = 0;
      for (auto const& par : root->type->type_params)
        functions << "," << par->type_name << " _P" << i++ << "";

      functions << "){" << root->type->glsl_string << "\n;}";
    }

    switch (root->type->instruction_type)
    {
    case sdf_type::type::prim:
    {
      auto h = hash_reduced(root, hash_reducers, hash_counter);
      auto const dname = "p" + to_hex_string(h);
      if (used_objects.emplace(h).second) {
        int off = current_offset;
        std::vector<std::string> param_names;
        for (auto const& par : root->params)
        {
          auto param_name = resolve_parameter(par,
            functions,
            distance_function,
            hash_reducers,
            hash_counter,
            used_objects,
            used_param_types,
            buffer_offsets,
            current_offset);
          param_names.push_back(std::move(param_name));
        }
        auto const p = std::static_pointer_cast<sdf_prim>(root);
        std::string in_pos = "_L";
        if (!p->parent.expired())
        {
          auto parent = p->parent.lock();
          in_pos = generate_glsl(parent, functions, distance_function, mul, hash_reducers, hash_counter, used_objects, used_param_types, buffer_offsets, current_offset);
        }
        distance_function << "_MT _mt" << dname << "=_mt;";
        distance_function << "float " << dname << "=" << fun_name << "(" << in_pos << ",_mt"<< dname;

        for (auto const& n : param_names)
          distance_function << "," << n;
        distance_function << ");";
      }
      return dname;
    }
    break;
    case sdf_type::type::op:
    {
      auto h = hash_reduced(root, hash_reducers, hash_counter);
      auto const dname = "o" + to_hex_string(h);
      if (used_objects.emplace(h).second) {
        int off = current_offset;
        std::vector<std::string> param_names;
        for (auto const& par : root->params)
        {
          auto param_name = resolve_parameter(par,
            functions,
            distance_function,
            hash_reducers,
            hash_counter,
            used_objects,
            used_param_types,
            buffer_offsets,
            current_offset);
          param_names.push_back(std::move(param_name));
        }
        auto const p = std::static_pointer_cast<sdf_op>(root);
        std::string in_pos1 = "_L";
        std::string in_pos2 = "_L";
        std::string in_mat1 = "_mt";
        std::string in_mat2 = "_mt";
        if (!p->lhs.expired())
        {
          auto parent = p->lhs.lock();
          in_pos1 = generate_glsl(parent, functions, distance_function, mul, hash_reducers, hash_counter, used_objects, used_param_types, buffer_offsets, current_offset);
          in_mat1 = "_mt" + in_pos1;
        }
        if (!p->rhs.expired())
        {
          auto parent = p->rhs.lock();
          in_pos2 = generate_glsl(parent, functions, distance_function, mul, hash_reducers, hash_counter, used_objects, used_param_types, buffer_offsets, current_offset);
          in_mat2 = "_mt" + in_pos2;
        }
        distance_function << "_MT _mt" << dname << "=_mt;";
        distance_function << "float " << dname << "=" << fun_name << "(" << in_pos1 << "," << in_pos2 << ", " << in_mat1 << "," << in_mat2 << ",_mt" << dname;

        for (auto const& n : param_names)
          distance_function << "," << n;
        distance_function << ");";
      }
      return dname;
    }
    break;
    case sdf_type::type::mod:
    {
      auto h = hash_reduced(root, hash_reducers, hash_counter);
      auto const dname = "m" + to_hex_string(h);
      auto mul_name = "j" + dname;
      mul << "*" << mul_name;

      if (used_objects.emplace(h).second) {
        distance_function << "float " << mul_name << "=1.0;";
        int off = current_offset;
        std::vector<std::string> param_names;
        for (auto const& par : root->params)
        {
          auto param_name = resolve_parameter(par,
            functions,
            distance_function,
            hash_reducers,
            hash_counter,
            used_objects,
            used_param_types,
            buffer_offsets,
            current_offset);
          param_names.push_back(std::move(param_name));
        }
        auto const p = std::static_pointer_cast<sdf_mod>(root);
        std::string in_pos = "_L";
        if (!p->parent.expired())
        {
          auto parent = p->parent.lock();
          in_pos = generate_glsl(parent, functions, distance_function, mul, hash_reducers, hash_counter, used_objects, used_param_types, buffer_offsets, current_offset);
        }
        distance_function << "vec3 " << dname << "=" << fun_name << "(" << in_pos << "," << mul_name;

        for (auto const& n : param_names)
          distance_function << "," << n;
        distance_function << ");";
      }
      return dname;
    }
    break;
    }
    throw std::runtime_error("could not determinee instruction type.");
  }

  std::string replace(std::string str, const std::string& sub1, const std::string& sub2)
  {
    if (sub1.empty())
      return str;

    std::size_t pos;
    while ((pos = str.find(sub1)) != std::string::npos)
      str.replace(pos, sub1.size(), sub2);

    return str;
  }

  constexpr bool is_space_or_newline(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
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
      return true;
    }
    return false;
  }

  std::string minify_glsl(std::string const& original) {
    std::string minified;
    //preallocate a bit more than needed.
    minified.reserve(original.size());

    enum class state {
      normal,
      space,
      preprocessor
    } current_state = state::normal;

    bool alphanumeric = false;

    for (char c : original) {
      switch (current_state) {
      case state::normal:
        if (is_space_or_newline(c))
          current_state = state::space;
        else if (c == '#') {
          current_state = state::preprocessor;
          minified.push_back(c);
        }
        else {
          alphanumeric = !is_operator(c);
          minified.push_back(c);
        }
        break;

      case state::preprocessor:
        if (c == '\n' || c == '\r')
          current_state = state::space;
        minified.push_back(c);
        break;

      case state::space:
        if (!is_space_or_newline(c) && !(c == '\n' || c == '\r'))
        {
          current_state = state::normal;

          if (alphanumeric && !is_operator(c))
            minified.push_back(' ');

          alphanumeric = !is_operator(c);
          minified.push_back(c);
        }
        break;
      }
    }
    minified.shrink_to_fit();
    return minified;
  }

  inline std::string generate_glsl(std::shared_ptr<sdf_instruction> const& root, std::unordered_map<size_t, int>& buffer_offsets)
  {
    std::stringstream functions;
    std::stringstream mul;
    mul << "1.0";
    std::stringstream distance_function;
    std::unordered_set<size_t> used_objects;
    std::unordered_set<sdf_parameter_type*> used_param_types;
    std::unordered_map<size_t, int> hash_reducers;
    int hash_counter = 0;
    int current_offset = 0;
    auto d = generate_glsl(root, functions, distance_function, mul, hash_reducers, hash_counter, used_objects, used_param_types, buffer_offsets, current_offset);

    auto str = functions.str() + "float map(vec3 _L, inout _MT _mt) {" + distance_function.str() + "_mt=_mt" +d+ ";return " + mul.str() + "*" + d + ";}";

    str = replace(str, "in_param", "_P");
    str = replace(str, "in_position", "_L");
    str = replace(str, "in_distance", "_D");
    str = replace(str, "out_multiplier", "_M");
    str = replace(str, "in_block", "_B");
    str = replace(str, "in_material", "_mt");
    str = replace(str, "inout_material", "_mt");
    return minify_glsl(str);
  }

  template<typename T> requires std::is_base_of_v<sdf_instruction, T>
    inline std::string generate_glsl(std::shared_ptr<T> const& root, std::unordered_map<size_t, int>& buffer_offsets)
    {
      return generate_glsl(std::static_pointer_cast<sdf_instruction>(root), buffer_offsets);
    }
}



namespace myrt::sdfs {
  inline auto translate(std::shared_ptr<vec3_param> offset)
  {
    static auto type = sdf_create_type(sdf_type::type::mod, "return in_position - in_param0;", offset->type);
    return instantiate_modifier(type, plist(std::move(offset)));
  }

  inline auto sphere(std::shared_ptr<float_param> radius, std::shared_ptr<int_param> material)
  {
    static auto type = sdf_create_type(sdf_type::type::prim, "inout_material = load_material(in_param1); return length(in_position) - in_param0;", { radius->type, material->type });
    return instantiate_primitive(type, plist(std::move(radius), std::move(material)));
  }

  inline auto box(std::shared_ptr<vec3_param> sizes, std::shared_ptr<int_param> material)
  {
    static auto type = sdf_create_type(sdf_type::type::prim, "inout_material = load_material(in_param1); return length(max(abs(in_position) - in_param0, 0.0));", { sizes->type, material->type });
    return instantiate_primitive(type, plist(std::move(sizes), std::move(material)));
  }

  inline auto box_rounded(std::shared_ptr<vec3_param> sizes, std::shared_ptr<float_param> radius, std::shared_ptr<int_param> material)
  {
    static auto type = sdf_create_type(sdf_type::type::prim, "inout_material = load_material(in_param2); return length(max(abs(in_position) - in_param0, 0.0)) - in_param1;", { sizes->type, radius->type, material->type });
    return instantiate_primitive(type, plist(std::move(sizes), std::move(radius), std::move(material)));
  }

  inline auto smooth_union(std::shared_ptr<float_param> size)
  {
    constexpr static auto glsl = R"(
float d1 = in_distance0;
float d2 = in_distance1;
float k = in_param0;
float h = clamp(0.5 + 0.5 * (d2 - d1) / k, 0.0, 1.0);

inout_material = mix_material(in_material0, in_material1, h);

return mix(d2, d1, h) - k * h * (1.0 - h);)";

    static auto type = sdf_create_type(sdf_type::type::op, glsl, size->type);
    return instantiate_operator(type, size);
  }
}