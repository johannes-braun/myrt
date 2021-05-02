#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <memory>
#include <sstream>
#include <iomanip>

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

    sdf_parameter_link link_value_block(size_t block, std::shared_ptr<sdf_parameter> other, size_t other_block) {
      sdf_parameter_link last = _value_links[block];
      _value_links[block].other = std::move(other);
      _value_links[block].block = other_block;
      return last;
    }

    sdf_parameter_link unlink_value_block(size_t block) {
      sdf_parameter_link last = _value_links[block];
      _value_links[block].other.reset();
      _value_links[block].block = 0;
      return last;
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
  def_param_type(vec4, 4, "return vec4(in_block0, in_block1, in_block2, in_block3)");
  def_param_type(mat2, 4, "return mat2(in_block0, in_block1, in_block2, in_block3)");
  def_param_type(mat4, 16, "return mat4(in_block0, in_block1, in_block2, in_block3, in_block0, in_block1, in_block2, in_block3, in_block0, in_block1, in_block2, in_block3, in_block0, in_block1, in_block2, in_block3)");
#undef def_param_type

  template<typename... PType> requires (std::convertible_to<std::decay_t<PType>, std::shared_ptr<sdf_parameter>> && ...)
    std::array<std::shared_ptr<sdf_parameter>, sizeof...(PType)> plist(PType&&... params) {
    return {
      std::forward<PType>(params)...
    };
  }

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
        functions << "vec3 " << fun_name << "(vec3 _L, inout float _M";
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
        distance_function << "float " << dname << "=" << fun_name << "(" << in_pos << ",_mt" << dname;

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

  inline std::string replace(std::string str, const std::string& sub1, const std::string& sub2)
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

  inline std::string minify_glsl(std::string const& original) {
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

    auto str = functions.str() + "float _SDF(vec3 _L, inout _MT _mt) {" + distance_function.str() + "_mt=_mt" + d + ";return " + mul.str() + "*" + d + ";}";

    str = replace(str, "in_param", "_P");
    str = replace(str, "in_position", "_L");
    str = replace(str, "in_distance", "_D");
    str = replace(str, "out_multiplier", "_M");
    str = replace(str, "in_block", "_B");
    str = replace(str, "in_material", "_mt");
    str = replace(str, "inout_material", "_mt");
    str = replace(str, "mix_material", "_mxm");
    str = replace(str, "load_material", "_ldm");
    return minify_glsl(str);
  }

  template<typename T> requires std::is_base_of_v<sdf_instruction, T>
    inline std::string generate_glsl(std::shared_ptr<T> const& root, std::unordered_map<size_t, int>& buffer_offsets)
    {
      return generate_glsl(std::static_pointer_cast<sdf_instruction>(root), buffer_offsets);
    }

    struct sdf_host {
      sdf_host() = default;

      template<typename T>
      sdf_host(std::shared_ptr<T> const& root, bool create_buffer = true) requires std::is_base_of_v<myrt::sdf_instruction, T> {
        glsl_string = myrt::generate_glsl(root, offsets);
        buffer_size = std::max_element(begin(offsets), end(offsets), [](auto const& pair, auto const& p2) { return pair.second < p2.second; })->second + 1;
        if (create_buffer) {
          buf_backing.resize(buffer_size);
          buf = buf_backing.data();
        }
      }

      template<typename T>
      sdf_host(sdf_host& old, std::shared_ptr<T> const& root, bool create_buffer = true) requires std::is_base_of_v<myrt::sdf_instruction, T> : sdf_host(root, create_buffer) {
        assign_parameters(old, root);
      }

      void override_buffer_ptr(float* ptr) {
        buf = ptr;
      }

      template<typename T>
      void regenerate(std::shared_ptr<T> const& root, bool create_buffer = true) requires std::is_base_of_v<myrt::sdf_instruction, T>
      {
        auto tmp = std::move(*this);
        *this = sdf_host(tmp, root, create_buffer);
      }

      void assign_parameters(sdf_host& old, std::shared_ptr<sdf_instruction> const& ins)
      {
        for (size_t par = 0; par < ins->params.size(); ++par)
        {
          auto const param = ins->params[par];
          std::vector<float> data(param->type->buffer_blocks);
          old.get_value(param, data.data());
          set_value(param, data.data());

          switch (ins->type->instruction_type)
          {
          case sdf_type::type::prim:
          {
            auto const p = std::static_pointer_cast<sdf_prim>(ins);
            if (!p->parent.expired())
              assign_parameters(old, p->parent.lock());
            break;
          }
          case sdf_type::type::mod:
          {
            auto const p = std::static_pointer_cast<sdf_mod>(ins);
            if (!p->parent.expired())
              assign_parameters(old, p->parent.lock());
            break;
          }
          case sdf_type::type::op:
          {
            auto const p = std::static_pointer_cast<sdf_op>(ins);
            if (!p->lhs.expired())
              assign_parameters(old, p->lhs.lock());
            if (!p->rhs.expired())
              assign_parameters(old, p->rhs.lock());
            break;
          }
          }
        }
      }

      void set_value(std::shared_ptr<myrt::sdf_parameter> const& param, float* data) {
        for (size_t i = 0; i < param->type->buffer_blocks; ++i)
        {
          myrt::sdf_parameter_link self{ param, i };
          auto const& link = param->get_link(i);
          if (!link.is_linked())
          {
            auto const hash = self.hash();
            auto index_it = offsets.find(hash);
            if (index_it != offsets.end())
              buf[index_it->second] = data[i];
          }
        }
      }
      void get_value(myrt::sdf_parameter_link const& self, float* data) const {
        auto const& link = self.other->get_link(self.block);
        if (!link.is_linked())
        {
          auto const hash = self.hash();
          auto index_it = offsets.find(hash);
          if(index_it != offsets.end())
            data[0] = buf[index_it->second];
        }
        else
        {
          get_value(link, data);
        }
      }
      void get_value(std::shared_ptr<myrt::sdf_parameter> const& param, float* data) const {
        for (size_t i = 0; i < param->type->buffer_blocks; ++i)
        {
          myrt::sdf_parameter_link self{ param, i };
          get_value(self, data + i);
        }
      }
      void set_value(std::shared_ptr<myrt::sdf_parameter> const& param, int data) {
        float cast = float(data);
        set_value(param, &cast);
      }
      void set_value(std::shared_ptr<myrt::sdf_parameter> const& param, unsigned data) {
        float cast = float(data);
        set_value(param, &cast);
      }
      void set_value(std::shared_ptr<myrt::sdf_parameter> const& param, float data) {
        set_value(param, &data);
      }
      void set_value(std::shared_ptr<myrt::sdf_parameter> const& param, rnu::vec2 data) {
        set_value(param, data.data());
      }
      void set_value(std::shared_ptr<myrt::sdf_parameter> const& param, rnu::vec3 data) {
        set_value(param, data.data());
      }
      void set_value(std::shared_ptr<myrt::sdf_parameter> const& param, rnu::vec4 data) {
        set_value(param, data.data());
      }
      void set_value(std::shared_ptr<myrt::sdf_parameter> const& param, rnu::mat2 data) {
        set_value(param, data.data());
      }
      void set_value(std::shared_ptr<myrt::sdf_parameter> const& param, rnu::mat4 data) {
        set_value(param, data.data());
      }

      void get_value(std::shared_ptr<myrt::sdf_parameter> const& param, int& data) const {
        float cast = float(data);
        get_value(param, &cast);
        data = int(cast);
      }
      void get_value(std::shared_ptr<myrt::sdf_parameter> const& param, unsigned& data) const {
        float cast = float(data);
        get_value(param, &cast);
        data = unsigned(cast);
      }
      void get_value(std::shared_ptr<myrt::sdf_parameter> const& param, float& data) const {
        get_value(param, &data);
      }
      void get_value(std::shared_ptr<myrt::sdf_parameter> const& param, rnu::vec2& data) const {
        get_value(param, data.data());
      }
      void get_value(std::shared_ptr<myrt::sdf_parameter> const& param, rnu::vec3& data) const {
        get_value(param, data.data());
      }
      void get_value(std::shared_ptr<myrt::sdf_parameter> const& param, rnu::vec4& data) const {
        get_value(param, data.data());
      }
      void get_value(std::shared_ptr<myrt::sdf_parameter> const& param, rnu::mat2& data) const {
        get_value(param, data.data());
      }
      void get_value(std::shared_ptr<myrt::sdf_parameter> const& param, rnu::mat4& data) const {
        get_value(param, data.data());
      }

      template<typename T, typename Param>
      T get_value(std::shared_ptr<Param> const& param) const requires std::is_base_of_v<myrt::sdf_parameter, Param>{
        T value;
        get_value(param, value);
        return value;
      }

      std::string glsl_string;
      std::unordered_map<size_t, int> offsets;
      std::vector<float> buf_backing;
      size_t buffer_size;
      float* buf = nullptr;
    };
}



namespace myrt::sdfs {
  struct prim;
  struct op;
  struct mod;

  template<typename S> requires std::is_base_of_v<sdf_instruction, S>
    struct sdf {
      sdf(std::shared_ptr<sdf_type> type)
        : _ptr(std::make_shared<S>(type))
      {
        _ptr->params.resize(type->type_params.size());
        for (size_t i = 0; i < _ptr->params.size(); ++i)
        {
          _ptr->params[i] = std::make_shared<sdf_parameter>(type->type_params[i]);
        }
      }

      void set(sdf_host& host, size_t parameter, float* value_array, size_t length) {
        // TODO: check length;
        host.set_value(_ptr->params[parameter], value_array);
      }

      template<typename T>
      void set(sdf_host& host, size_t parameter, T&& value) {
        host.set_value(_ptr->params[parameter], std::forward<T>(value));
      }

      void get(sdf_host& host, size_t parameter, float* value_array, size_t length) {
        // TODO: check length;
        host.get_value(_ptr->params[parameter], value_array);
      }

      template<typename T>
      void get(sdf_host& host, size_t parameter, T& value) const {
        host.get_value(_ptr->params[parameter], value);
      }

      template<typename T>
      T get(sdf_host& host, size_t parameter) const {
        return host.get_value<T>(_ptr->params[parameter]);
      }

      std::shared_ptr<sdf_parameter> get_parameter(size_t index) const {
        if (index >= _ptr->params.size())
          return nullptr;
        return _ptr->params[index];
      }

      template<typename PType>
      std::shared_ptr<PType> get_parameter(size_t index) const requires std::is_base_of_v<sdf_parameter, PType> {
        std::shared_ptr<sdf_parameter> const& ptr = get_parameter(index);
        if (ptr && ptr->type == PType::type)
          return std::static_pointer_cast<PType>(ptr);
        return nullptr;
      }

      template<typename PType>
      std::shared_ptr<PType> link_parameter(size_t index, std::shared_ptr<PType> param) requires std::is_base_of_v<sdf_parameter, PType> {
        auto const target = get_parameter<PType>(index);
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

      template<size_t Index, typename TValue, typename TParam> requires std::is_base_of_v<sdf_parameter, TParam>
        struct parameter
        {
          using param_type = TParam;
          using value_type = TValue;
          constexpr static size_t index = Index;

          constexpr parameter() = default;
        };

        template<size_t Index, typename TValue, typename TParam>
        void set(parameter<Index, TValue, TParam> const&, sdf_host& host, TValue value) {
          set(host, Index, value);
        }

        template<size_t Index, typename TValue, typename TParam>
        TValue get(parameter<Index, TValue, TParam> const&, sdf_host& host) {
          return get<TValue>(host, Index);
        }

        template<size_t Index, typename TValue, typename TParam>
        std::shared_ptr<TParam> get_parameter(parameter<Index, TValue, TParam> const&) {
          return get_parameter<TParam>(Index);
        }

        template<size_t Index, typename TValue, typename TParam>
        std::shared_ptr<TParam> link(parameter<Index, TValue, TParam> const&, std::shared_ptr<TParam> param) {
          return link_parameter(Index, std::move(param));
        }

        template<size_t Index, typename TValue, typename TParam>
        void unlink(parameter<Index, TValue, TParam> const&) {
          return unlink_parameter(Index);
        }
    };

    struct basic_type {
      basic_type(sdf_type::type type, std::string_view glsl, std::span<std::shared_ptr<sdf_parameter_type> const> params)
      {
        _type = std::make_shared<sdf_type>();
        _type->instruction_type = type;
        _type->glsl_string = glsl;
        _type->type_params.insert(_type->type_params.end(), std::begin(params), std::end(params));
      }

      basic_type(sdf_type::type type, std::string_view glsl, std::initializer_list<std::shared_ptr<sdf_parameter_type>> params)
        :basic_type(type, std::move(glsl), std::span<std::shared_ptr<sdf_parameter_type> const>(params)) {
      }

      basic_type(sdf_type::type type, std::string_view glsl, std::shared_ptr<sdf_parameter_type> const& param)
        :basic_type(type, std::move(glsl), std::initializer_list<std::shared_ptr<sdf_parameter_type>>{ param }) {
      }

      operator std::shared_ptr<sdf_type> const& () const {
        return _type;
      }

    private:
      std::shared_ptr<sdf_type> _type;
    };

    struct prim : sdf<sdf_prim> {
      prim(basic_type type)
        : sdf(type)
      {
      }

      prim& transform(sdf<sdf_mod> m) {
        _ptr->set_parent(m.get_pointer());
        return *this;
      }
    };

    struct op : sdf<sdf_op> {
      op(basic_type type)
        : sdf(type)
      {
      }

      op& set_left(sdf<sdf_op> lhs) {
        _ptr->set_lhs(lhs.get_pointer());
        return *this;
      }
      op& set_right(sdf<sdf_op> rhs) {
        _ptr->set_rhs(rhs.get_pointer());
        return *this;
      }
      op& set_left(sdf<sdf_prim> lhs) {
        _ptr->set_lhs(lhs.get_pointer());
        return *this;
      }
      op& set_right(sdf<sdf_prim> rhs) {
        _ptr->set_rhs(rhs.get_pointer());
        return *this;
      }
    };

    struct mod : sdf<sdf_mod> {
      mod(basic_type type)
        : sdf(type)
      {
      }

      mod& transform(sdf<sdf_mod> m) {
        _ptr->set_parent(m.get_pointer());
        return *this;
      }
    };

    struct sphere : prim {
      static inline const basic_type sphere_type = basic_type{
        sdf_type::type::prim, "inout_material = load_material(in_param1); return length(in_position) - in_param0;", { float_param::type, int_param::type }
      };

      sphere() : prim(sphere_type) {}

      constexpr static parameter<0, float, float_param> radius{};
      constexpr static parameter<1, int, int_param> material{};
    };

    struct translate : mod {
      static inline const basic_type translate_type = basic_type{
        sdf_type::type::mod, "return in_position - in_param0;", { vec3_param::type }
      };

      translate() : mod(translate_type) {}

      constexpr static parameter<0, rnu::vec3, vec3_param> offset{};
    };

    struct hard_union : op {
      static inline const basic_type hard_union_type = basic_type{
        sdf_type::type::op, R"(
float m = min(in_distance0, in_distance1);
inout_material = mix_material(in_material0, in_material1, float(m == in_distance1));
return m;)", {  }
      };

      hard_union() : op(hard_union_type) {}
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
        sdf_type::type::op, glsl, { float_param::type }
      };

      smooth_union() : op(smooth_union_type) {}

      constexpr static parameter<0, float, float_param> factor{};
    };

    struct box : prim {
      static inline const basic_type box_type = basic_type{
        sdf_type::type::prim, R"(
  vec3 b = in_param0;
  vec3 p = in_position;
inout_material = load_material(in_param1); 
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);)", {vec3_param::type, int_param::type}
      };

      box() : prim(box_type) {}

      constexpr static parameter<0, rnu::vec3, vec3_param> size{};
      constexpr static parameter<1, int, int_param> material{};
    };

    struct box_rounded : prim {
      static inline const basic_type rounded_box_type = basic_type{
        sdf_type::type::prim, R"(
  vec3 b = in_param0;
  float r = in_param1;
  vec3 p = in_position; 
inout_material = load_material(in_param2);
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0) - r;)", {vec3_param::type, float_param::type, int_param::type}
      };

      box_rounded() : prim(rounded_box_type) {}

      constexpr static parameter<0, rnu::vec3, vec3_param> size{};
      constexpr static parameter<1, float, float_param> radius{};
      constexpr static parameter<2, int, int_param> material{};
    };

    struct torus : prim {
      static inline const basic_type torus_type = basic_type{
        sdf_type::type::prim, R"(
  float rlarge = in_param0;
  float rsmall = in_param1;
inout_material = load_material(in_param2);
  vec3 p = in_position; 
  vec2 q = vec2(length(p.xz)-rlarge,p.y);
  return length(q)-rsmall;)", {float_param::type, float_param::type, int_param::type}
      };

      torus() : prim(torus_type) {}

      constexpr static parameter<0, float, float_param> radius_large{};
      constexpr static parameter<1, float, float_param> radius_small{};
      constexpr static parameter<2, int, int_param> material{};
    };

    struct menger_fractal : prim {
      static inline const basic_type menger_fractal_type = basic_type{
        sdf_type::type::prim, R"(vec3 p = in_position;
inout_material = load_material(in_param1); 
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

		vec3 d = abs(p) - in_param0;
		float dis = min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));

		dis *= pow(3., float(-4));

		return dis;)", {vec3_param::type, int_param::type}
      };

      menger_fractal() : prim(menger_fractal_type) {}

      constexpr static parameter<0, rnu::vec3, vec3_param> size{};
      constexpr static parameter<1, int, int_param> material{};
    };
}