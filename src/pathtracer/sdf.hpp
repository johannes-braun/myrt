#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <memory>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include "parameter.hpp"
#include "utils.hpp"

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

      std::optional<int> find_parameter_index_by_name(std::string const& name) const {
        auto const it = std::find(begin(param_names), end(param_names), name);
        if (it == param_names.end())
          return std::nullopt;
        return static_cast<int>(std::distance(begin(param_names), it));
      }

      category instruction_category;
      std::string glsl_string;
      std::vector<std::shared_ptr<parameter_type>> type_params;
      std::vector<std::string> param_names;
    };

    struct sdf_instruction {

      sdf_instruction(std::shared_ptr<sdf_type> type)
        : type(type) {

      }

      std::vector<std::shared_ptr<parameter>> params;
      std::shared_ptr<sdf_type> type;
      std::weak_ptr<sdf_instruction> joint_element;
    };

    struct sdf_prim : sdf_instruction, std::enable_shared_from_this<sdf_prim> {
      sdf_prim(std::shared_ptr<sdf_type> type) : sdf_instruction(type) {}

      void set_parent(std::shared_ptr<sdf_instruction> p) {
        child = p;
        if (p != nullptr)
          p->joint_element = shared_from_this();
      }

      std::shared_ptr<sdf_instruction> child;
    };

    struct sdf_op : sdf_instruction, std::enable_shared_from_this<sdf_op> {
      sdf_op(std::shared_ptr<sdf_type> type) : sdf_instruction(type) {}

      void set_lhs(std::shared_ptr<sdf_instruction> l) {
        lhs = l;
        if (l != nullptr)
          l->joint_element = shared_from_this();
      }

      void set_rhs(std::shared_ptr<sdf_instruction> r) {
        rhs = r;
        if (r != nullptr)
          r->joint_element = shared_from_this();
      }

      std::shared_ptr<sdf_instruction> lhs;
      std::shared_ptr<sdf_instruction> rhs;
    };

    struct sdf_mod : sdf_instruction, std::enable_shared_from_this<sdf_mod> {
      sdf_mod(std::shared_ptr<sdf_type> type) : sdf_instruction(type) {}

      void set_parent(std::shared_ptr<sdf_instruction> p) {
        child = p;
        if (p != nullptr)
          p->joint_element = shared_from_this();
      }

      std::shared_ptr<sdf_instruction> child;
    };

    inline std::shared_ptr<sdf_type> sdf_create_type(sdf_type::category type, std::string_view glsl, std::span<std::shared_ptr<parameter_type> const> params)
    {
      auto const mod = std::make_shared<sdf_type>();
      mod->instruction_category = type;
      mod->glsl_string = glsl;
      mod->type_params.insert(mod->type_params.end(), std::begin(params), std::end(params));
      return mod;
    }

    inline std::shared_ptr<sdf_type> sdf_create_type(sdf_type::category type, std::string_view glsl, std::initializer_list<std::shared_ptr<parameter_type>> params)
    {
      return sdf_create_type(type, std::move(glsl), std::span<std::shared_ptr<parameter_type> const>(params));
    }

    inline std::shared_ptr<sdf_type> sdf_create_type(sdf_type::category type, std::string_view glsl, std::shared_ptr<parameter_type> const& param)
    {
      return sdf_create_type(type, std::move(glsl), std::initializer_list<std::shared_ptr<parameter_type>>{ param });
    }

    template<typename T>
    inline std::shared_ptr<T> sdf_create_instance(std::shared_ptr<sdf_type> type, std::span<std::shared_ptr<parameter> const> params) requires std::is_base_of_v<sdf_instruction, T>
    {
      auto const mod = std::make_shared<T>(std::move(type));
      mod->params.insert(mod->params.end(), std::begin(params), std::end(params));
      return mod;
    }

    template<typename T>
    inline std::shared_ptr<T> sdf_create_instance(std::shared_ptr<sdf_type> type, std::initializer_list<std::shared_ptr<parameter>> params) requires std::is_base_of_v<sdf_instruction, T>
    {
      return sdf_create_instance<T>(std::move(type), std::span<std::shared_ptr<parameter> const>(params));
    }

    template<typename T>
    inline std::shared_ptr<T> sdf_create_instance(std::shared_ptr<sdf_type> type, std::shared_ptr<parameter> const& param) requires std::is_base_of_v<sdf_instruction, T>
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

    //inline std::string to_hex_string(size_t hash) {
    //  std::stringstream stream;
    //  stream << std::hex << (std::uint32_t(hash) ^ std::uint32_t(hash >> 32));
    //  return stream.str();
    //}

    //template<typename T>
    //inline int hash_reduced(T&& t,
    //  std::unordered_map<size_t, int>& hash_reducers,
    //  int& hash_counter)
    //{
    //  auto const hash = std::hash<std::decay_t<T>>{}(std::forward<T>(t));

    //  if (auto const it = hash_reducers.find(hash); it != hash_reducers.end())
    //  {
    //    return it->second;
    //  }
    //  else
    //  {
    //    hash_reducers.emplace_hint(it, std::make_pair(hash, hash_counter++));
    //    return hash_counter - 1;
    //  }
    //}

    inline std::string resolve_parameter(
      std::shared_ptr<parameter> param,

      std::stringstream& functions,
      std::stringstream& distance_function,
      std::unordered_map<size_t, int>& hash_reducers,
      int& hash_counter,
      std::unordered_set<size_t>& used_objects,
      std::unordered_set<parameter_type*>& used_param_types,
      std::unordered_map<size_t, int>& buffer_offsets,
      int& current_offset)
    {
      auto param_hash = hash_reduced(param, hash_reducers, hash_counter);
      std::string param_name = "par" + to_hex_string(param_hash);
      if (used_objects.emplace(param_hash).second) {
        std::vector<std::string> block_names(param->type->buffer_blocks);

        for (size_t i = 0; i < param->type->buffer_blocks; ++i)
        {
          parameter_link self{ param, i };
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

    inline std::string generate_glsl_impl(
      std::shared_ptr<sdf_instruction> const& root,
      sdf_build_cache_t& cache)
    {
      const auto hash = hash_reduced(root->type, cache.hash_reducers, cache.hash_counter);
      const auto fun_name = "_X" + to_hex_string(hash);
      if (cache.used_functions.emplace(hash).second) {

        for (auto const& par : root->type->type_params)
        {
          if (cache.used_param_types.emplace(par.get()).second)
            cache.functions << par->function;
        }

        switch (root->type->instruction_category)
        {
        case sdf_type::category::prim:
          cache.functions << "float " << fun_name << "(vec3 _L, inout _MT _mt";
          break;
        case sdf_type::category::op:
          cache.functions << "float " << fun_name << "(float _D0, float _D1, _MT _mt0, _MT _mt1, inout _MT _mt";
          break;
        case sdf_type::category::mod:
          cache.functions << "vec3 " << fun_name << "(vec3 _L, inout float _M";
          break;
        }

        int i = 0;
        for (auto const& par : root->type->type_params)
          cache.functions << "," << par->type_name << " _P" << i++ << "";

        cache.functions << "){" << root->type->glsl_string << "\n;}";
      }

      switch (root->type->instruction_category)
      {
      case sdf_type::category::prim:
      {
        auto h = hash_reduced(root, cache.hash_reducers, cache.hash_counter);
        auto const dname = "p" + to_hex_string(h);
        if (cache.used_objects.emplace(h).second) {
          int off = cache.current_offset;
          std::vector<std::string> param_names;
          for (auto const& par : root->params)
          {
            auto param_name = resolve_parameter(par,
              cache.functions,
              cache.distance_function,
              cache.hash_reducers,
              cache.hash_counter,
              cache.used_objects,
              cache.used_param_types,
              cache.buffer_offsets,
              cache.current_offset);
            param_names.push_back(std::move(param_name));
          }
          auto const p = std::static_pointer_cast<sdf_prim>(root);
          std::string in_pos = "_L";
          if (p->child)
          {
            in_pos = generate_glsl_impl(p->child, cache);
          }
          cache.distance_function << "_MT _mt" << dname << "=_mt;";
          cache.distance_function << "float " << dname << "=" << fun_name << "(" << in_pos << ",_mt" << dname;

          for (auto const& n : param_names)
            cache.distance_function << "," << n;
          cache.distance_function << ");";
        }
        return dname;
      }
      break;
      case sdf_type::category::op:
      {
        auto h = hash_reduced(root, cache.hash_reducers, cache.hash_counter);
        auto const dname = "o" + to_hex_string(h);
        if (cache.used_objects.emplace(h).second) {
          int off = cache.current_offset;
          std::vector<std::string> param_names;
          for (auto const& par : root->params)
          {
            auto param_name = resolve_parameter(par,
              cache.functions,
              cache.distance_function,
              cache.hash_reducers,
              cache.hash_counter,
              cache.used_objects,
              cache.used_param_types,
              cache.buffer_offsets,
              cache.current_offset);
            param_names.push_back(std::move(param_name));
          }
          auto const p = std::static_pointer_cast<sdf_op>(root);
          std::string in_pos1 = "_L";
          std::string in_pos2 = "_L";
          std::string in_mat1 = "_mt";
          std::string in_mat2 = "_mt";
          if (p->lhs)
          {
            in_pos1 = generate_glsl_impl(p->lhs, cache);
            in_mat1 = "_mt" + in_pos1;
          }
          if (p->rhs)
          {
            in_pos2 = generate_glsl_impl(p->rhs, cache);
            in_mat2 = "_mt" + in_pos2;
          }
          cache.distance_function << "_MT _mt" << dname << "=_mt;";
          cache.distance_function << "float " << dname << "=" << fun_name << "(" << in_pos1 << "," << in_pos2 << ", " << in_mat1 << "," << in_mat2 << ",_mt" << dname;

          for (auto const& n : param_names)
            cache.distance_function << "," << n;
          cache.distance_function << ");";
        }
        return dname;
      }
      break;
      case sdf_type::category::mod:
      {
        auto h = hash_reduced(root, cache.hash_reducers, cache.hash_counter);
        auto const dname = "m" + to_hex_string(h);
        auto mul_name = "j" + dname;
        cache.mul << "*" << mul_name;

        if (cache.used_objects.emplace(h).second) {
          cache.distance_function << "float " << mul_name << "=1.0;";
          int off = cache.current_offset;
          std::vector<std::string> param_names;
          for (auto const& par : root->params)
          {
            auto param_name = resolve_parameter(par,
              cache.functions,
              cache.distance_function,
              cache.hash_reducers,
              cache.hash_counter,
              cache.used_objects,
              cache.used_param_types,
              cache.buffer_offsets,
              cache.current_offset);
            param_names.push_back(std::move(param_name));
          }
          auto const p = std::static_pointer_cast<sdf_mod>(root);
          std::string in_pos = "_L";
          if (p->child)
          {
            in_pos = generate_glsl_impl(p->child, cache);
          }
          cache.distance_function << "vec3 " << dname << "=" << fun_name << "(" << in_pos << "," << mul_name;

          for (auto const& n : param_names)
            cache.distance_function << "," << n;
          cache.distance_function << ");";
        }
        return dname;
      }
      break;
      }
      throw std::runtime_error("could not determinee instruction type.");
    }

    //inline std::string replace(std::string str, const std::string& sub1, const std::string& sub2)
    //{
    //  if (sub1.empty())
    //    return str;

    //  std::size_t pos;
    //  while ((pos = str.find(sub1)) != std::string::npos)
    //    str.replace(pos, sub1.size(), sub2);

    //  return str;
    //}

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
        case state::space:
          if (!is_space_or_newline(c) && !(c == '\n' || c == '\r'))
          {
            current_state = state::normal;

            if (alphanumeric && !is_operator(c))
              minified.push_back(' ');
          }
        case state::normal:
          if (is_space_or_newline(c))
            current_state = state::space;
          else if (c == '#') {
            if (!minified.empty() && !is_newline(minified.back()))
              minified.push_back('\n');
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
        }
      }
      minified.shrink_to_fit();
      return minified;
    }

    inline std::string generate_glsl(std::shared_ptr<sdf_instruction> const& root, sdf_build_cache_t& cache)
    {
      cache.mul << "1.0";
      auto d = generate_glsl_impl(root, cache);

      auto str = cache.functions.str() + "float _SDF(vec3 _L, inout _MT _mt) {" + cache.distance_function.str() + "_mt=_mt" + d + ";return " + cache.mul.str() + "*" + d + ";}";

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
        sdf_build_cache_t cache;
        auto c = generate_glsl(std::static_pointer_cast<sdf_instruction>(root), cache);
        buffer_offsets = cache.buffer_offsets;
        return c;
      }

      struct sdf_glsl_assembly
      {
        parameter_buffer_description buffer_description;

       /*size_t buffer_blocks_required;
        std::unordered_map<size_t, int> buffer_offsets;*/

        void initialize_from_default(float* buf, std::shared_ptr<sdf_instruction> const& root) {
          for (size_t i = 0; i < root->params.size(); ++i)
            buffer_description.set_value(buf, root->params[i], root->params[i]->get_default_value().data());

          switch (root->type->instruction_category)
          {
          case sdf_type::category::prim:
          {
            auto const p = std::static_pointer_cast<sdf_prim>(root);
            if (p->child)
              initialize_from_default(buf, p->child);
            break;
          }
          case sdf_type::category::mod:
          {
            auto const p = std::static_pointer_cast<sdf_mod>(root);
            if (p->child)
              initialize_from_default(buf, p->child);
            break;
          }
          case sdf_type::category::op:
          {
            auto const p = std::static_pointer_cast<sdf_op>(root);
            if (p->lhs)
              initialize_from_default(buf, p->lhs);
            if (p->rhs)
              initialize_from_default(buf, p->rhs);
            break;
          }
          }
        }/*

        void set_value(float* buffer, std::shared_ptr<parameter> const& param, float const* data) const {
          param->set_default_value(std::span<float const>(data, param->type->buffer_blocks));
          for (size_t i = 0; buffer && i < param->type->buffer_blocks; ++i)
          {
            parameter_link self{ param, i };
            auto const& link = param->get_link(i);
            if (!link.is_linked())
            {
              auto const hash = self.hash();
              auto index_it = buffer_offsets.find(hash);
              if (index_it != buffer_offsets.end())
                buffer[index_it->second] = data[i];
            }
          }
        }

        void set_value(float* buffer, std::shared_ptr<parameter> const& param, int data) const {
          float cast = float(data);
          set_value(buffer, param, &cast);
        }
        void set_value(float* buffer, std::shared_ptr<parameter> const& param, unsigned data) const {
          float cast = float(data);
          set_value(buffer, param, &cast);
        }
        void set_value(float* buffer, std::shared_ptr<parameter> const& param, float data) const {
          set_value(buffer, param, &data);
        }
        void set_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::vec2 data) const {
          set_value(buffer, param, data.data());
        }
        void set_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::vec3 data) const {
          set_value(buffer, param, data.data());
        }
        void set_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::vec4 data) const {
          set_value(buffer, param, data.data());
        }
        void set_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::mat2 data) const {
          set_value(buffer, param, data.data());
        }
        void set_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::mat4 data) const {
          set_value(buffer, param, data.data());
        }

        void get_link_value(float* buffer, parameter_link const& self, float* data) const {
          auto const& link = self.other->get_link(self.block);
          if (!link.is_linked())
          {
            auto const hash = self.hash();
            auto index_it = buffer_offsets.find(hash);
            if (index_it != buffer_offsets.end())
              data[0] = buffer[index_it->second];
          }
          else
          {
            get_link_value(buffer, link, data);
          }
        }

        void get_value(float* buffer, std::shared_ptr<parameter> const& param, float* data) const {
          if (!buffer)
          {
            std::copy(param->get_default_value().begin(), param->get_default_value().end(), data);
            return;
          }
          for (size_t i = 0; i < param->type->buffer_blocks; ++i)
          {
            parameter_link self{ param, i };
            get_link_value(buffer, self, data + i);
          }
        }

        void get_value(float* buffer, std::shared_ptr<parameter> const& param, int& data) const {
          float cast = float(data);
          get_value(buffer, param, &cast);
          data = int(cast);
        }
        void get_value(float* buffer, std::shared_ptr<parameter> const& param, unsigned& data) const {
          float cast = float(data);
          get_value(buffer, param, &cast);
          data = unsigned(cast);
        }
        void get_value(float* buffer, std::shared_ptr<parameter> const& param, float& data) const {
          get_value(buffer, param, &data);
        }
        void get_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::vec2& data) const {
          get_value(buffer, param, data.data());
        }
        void get_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::vec3& data) const {
          get_value(buffer, param, data.data());
        }
        void get_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::vec4& data) const {
          get_value(buffer, param, data.data());
        }
        void get_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::mat2& data) const {
          get_value(buffer, param, data.data());
        }
        void get_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::mat4& data) const {
          get_value(buffer, param, data.data());
        }

        template<typename T, typename Param>
        T get_value(float* buffer, std::shared_ptr<Param> const& param) const requires std::is_base_of_v<parameter, Param>{
          T value;
          get_value(buffer, param, value);
          return value;
        }*/
      };

      struct sdf_glsl_assembler {
        constexpr static auto empty_sdf_glsl = "float map(vec3 p, inout _MT _mt) { switch(sdf_index_current) { default: return 1.0 / 0.0; }}";

        sdf_glsl_assembly append(std::shared_ptr<sdf_instruction> const& root)
        {
          sdf_glsl_assembly new_assembly;
          size_t const offset_before = m_build_cache.current_offset;
          prepare_build_cache();
          const auto glsl_string = generate_glsl(root, m_build_cache);
          new_assembly.buffer_description.buffer_offsets = std::move(m_build_cache.buffer_offsets);
          new_assembly.buffer_description.id = m_current_id++;
          new_assembly.buffer_description.blocks_required = m_build_cache.current_offset - offset_before;

          std::stringstream stream;
          stream << "#define _SDF _SDF" << new_assembly.buffer_description.id << '\n';
          stream << glsl_string << "\n#undef _SDF\n";
          m_full_glsl += stream.str();

          append_map_case(new_assembly.buffer_description.id);
          return new_assembly;
        }

        std::string get_assembled_glsl() const {
          return "#line 0 \"myrt_gen_sdf\"\n" + m_full_glsl + m_map_function;
        }

      private:
        void append_map_case(int id) {
          m_map_function.pop_back(); //}
          m_map_function.pop_back(); //}

          auto const id_str = std::to_string(id);
          m_map_function += "case " + id_str + ": return _SDF" + id_str + "(p, _mt);}}";
        }

        void prepare_build_cache() {
          m_build_cache.buffer_offsets.clear();
          m_build_cache.used_objects.clear();
          m_build_cache.functions.str("");
          m_build_cache.mul.str("");
          m_build_cache.distance_function.str("");
        }

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

  template<typename S> requires std::is_base_of_v<instruction, S>
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
      basic_type(category::category t, std::string_view glsl, std::span<std::shared_ptr<parameter_type> const> params, std::span<std::string const> param_names = {})
      {
        _type = std::make_shared<instruction_type>();
        _type->instruction_category = t;
        _type->glsl_string = glsl;
        _type->type_params.insert(_type->type_params.end(), std::begin(params), std::end(params));
        _type->param_names.resize(params.size());
        if(param_names.size() == params.size())
          _type->param_names.assign(std::begin(param_names), std::end(param_names));
      }

      basic_type(instruction_type::category type, std::string_view glsl, std::initializer_list<std::shared_ptr<parameter_type>> params, std::initializer_list<std::string> param_names = {})
        :basic_type(type, std::move(glsl), std::span<std::shared_ptr<parameter_type> const>(params)) {
      }

      basic_type(instruction_type::category type, std::string_view glsl, std::shared_ptr<parameter_type> param, std::string param_name="")
        :basic_type(type, std::move(glsl), std::initializer_list<std::shared_ptr<parameter_type>>{ std::move(param) }, std::initializer_list<std::string const>{std::move(param_name)}) {
      }

      operator std::shared_ptr<instruction_type> const& () const {
        return _type;
      }

    private:
      std::shared_ptr<instruction_type> _type;
    };

    struct prim : sdf<basic_primitive> {
      prim(basic_type type)
        : sdf(type)
      {
      }

      prim& transform(sdf<basic_modifier> m) {
        _ptr->set_parent(m.get_pointer());
        return *this;
      }
    };

    struct op : sdf<basic_operator> {
      op(basic_type type)
        : sdf(type)
      {
      }

      op& set_left(sdf<basic_operator> lhs) {
        _ptr->set_lhs(lhs.get_pointer());
        return *this;
      }
      op& set_right(sdf<basic_operator> rhs) {
        _ptr->set_rhs(rhs.get_pointer());
        return *this;
      }
      op& set_left(sdf<basic_primitive> lhs) {
        _ptr->set_lhs(lhs.get_pointer());
        return *this;
      }
      op& set_right(sdf<basic_primitive> rhs) {
        _ptr->set_rhs(rhs.get_pointer());
        return *this;
      }

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
      mod(basic_type type)
        : sdf(type)
      {
      }

      mod& transform(sdf<basic_modifier> m) {
        _ptr->set_parent(m.get_pointer());
        return *this;
      }
    };

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