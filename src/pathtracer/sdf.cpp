#include "sdf.hpp"

namespace myrt
{
  namespace detail {
    std::optional<int> sdf_type::find_parameter_index_by_name(std::string const& name) const {
      auto const it = std::find(begin(param_names), end(param_names), name);
      if (it == param_names.end())
        return std::nullopt;
      return static_cast<int>(std::distance(begin(param_names), it));
    }

    sdf_instruction::sdf_instruction(std::shared_ptr<sdf_type> type)
      : type(type) {

    }

    sdf_prim::sdf_prim(std::shared_ptr<sdf_type> type) : sdf_instruction(type) {}

    void sdf_prim::set_parent(std::shared_ptr<sdf_instruction> p) {
      child = p;
      if (p != nullptr)
        p->joint_element = shared_from_this();
    }

    sdf_op::sdf_op(std::shared_ptr<sdf_type> type) : sdf_instruction(type) {}

    void sdf_op::set_lhs(std::shared_ptr<sdf_instruction> l) {
      lhs = l;
      if (l != nullptr)
        l->joint_element = shared_from_this();
    }

    void sdf_op::set_rhs(std::shared_ptr<sdf_instruction> r) {
      rhs = r;
      if (r != nullptr)
        r->joint_element = shared_from_this();
    }

    sdf_mod::sdf_mod(std::shared_ptr<sdf_type> type) : sdf_instruction(type) {}

    void sdf_mod::set_parent(std::shared_ptr<sdf_instruction> p) {
      child = p;
      if (p != nullptr)
        p->joint_element = shared_from_this();
    }

    std::shared_ptr<sdf_type> sdf_create_type(sdf_type::category type, std::string_view glsl, std::span<std::shared_ptr<parameter_type>const> params)
    {
      auto const mod = std::make_shared<sdf_type>();
      mod->instruction_category = type;
      mod->glsl_string = glsl;
      mod->type_params.insert(mod->type_params.end(), std::begin(params), std::end(params));
      return mod;
    }

    std::shared_ptr<sdf_type> sdf_create_type(sdf_type::category type, std::string_view glsl, std::initializer_list<std::shared_ptr<parameter_type>> params)
    {
      return sdf_create_type(type, std::move(glsl), std::span<std::shared_ptr<parameter_type> const>(params));
    }

    std::shared_ptr<sdf_type> sdf_create_type(sdf_type::category type, std::string_view glsl, std::shared_ptr<parameter_type> const& param)
    {
      return sdf_create_type(type, std::move(glsl), std::initializer_list<std::shared_ptr<parameter_type>>{ param });
    }

    std::string resolve_parameter(std::shared_ptr<parameter> param, std::stringstream& functions, std::stringstream& distance_function, std::unordered_map<size_t, int>& hash_reducers, int& hash_counter, std::unordered_set<size_t>& used_objects, std::unordered_set<parameter_type*>& used_param_types, std::unordered_map<size_t, int>& buffer_offsets, int& current_offset)
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

    std::string generate_glsl_impl(std::shared_ptr<sdf_instruction> const& root, sdf_build_cache_t& cache)
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
    std::string generate_glsl(std::shared_ptr<sdf_instruction> const& root, sdf_build_cache_t& cache)
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
    void sdf_glsl_assembly::initialize_from_default(float* buf, std::shared_ptr<sdf_instruction> const& root) {
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
    }

    sdf_glsl_assembly sdf_glsl_assembler::append(std::shared_ptr<sdf_instruction> const& root)
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

    std::string sdf_glsl_assembler::get_assembled_glsl() const {
      return "#line 0 \"myrt_gen_sdf\"\n" + m_full_glsl + m_map_function;
    }

    void sdf_glsl_assembler::append_map_case(int id) {
      m_map_function.pop_back(); //}
      m_map_function.pop_back(); //}

      auto const id_str = std::to_string(id);
      m_map_function += "case " + id_str + ": return _SDF" + id_str + "(p, _mt);}}";
    }

    void sdf_glsl_assembler::prepare_build_cache() {
      m_build_cache.buffer_offsets.clear();
      m_build_cache.used_objects.clear();
      m_build_cache.functions.str("");
      m_build_cache.mul.str("");
      m_build_cache.distance_function.str("");
    }
  }

  namespace sdf 
  {
    basic_type::basic_type(category::category t, std::string_view glsl, std::span<std::shared_ptr<parameter_type> const> params, std::span<std::string const> param_names)
    {
      _type = std::make_shared<instruction_type>();
      _type->instruction_category = t;
      _type->glsl_string = glsl;
      _type->type_params.insert(_type->type_params.end(), std::begin(params), std::end(params));
      _type->param_names.resize(params.size());
      if (param_names.size() == params.size())
        _type->param_names.assign(std::begin(param_names), std::end(param_names));
    }

    basic_type::basic_type(instruction_type::category type, std::string_view glsl, std::initializer_list<std::shared_ptr<parameter_type>> params, std::initializer_list<std::string> param_names)
      :basic_type(type, std::move(glsl), std::span<std::shared_ptr<parameter_type> const>(params)) {
    }

    basic_type::basic_type(instruction_type::category type, std::string_view glsl, std::shared_ptr<parameter_type> param, std::string param_name)
      :basic_type(type, std::move(glsl), std::initializer_list<std::shared_ptr<parameter_type>>{ std::move(param) }, std::initializer_list<std::string const>{std::move(param_name)}) {
    }

    basic_type::operator std::shared_ptr<instruction_type> const& () const {
      return _type;
    }
    prim::prim(basic_type type)
      : sdf(type)
    {
    }
    prim& prim::transform(sdf<basic_modifier> m) {
      _ptr->set_parent(m.get_pointer());
      return *this;
    }
    op::op(basic_type type)
      : sdf(type)
    {
    }
    op& op::set_left(sdf<basic_primitive> lhs) {
      _ptr->set_lhs(lhs.get_pointer());
      return *this;
    }
    op& op::set_right(sdf<basic_primitive> rhs) {
      _ptr->set_rhs(rhs.get_pointer());
      return *this;
    }
    op& op::set_right(sdf<basic_operator> rhs) {
      _ptr->set_rhs(rhs.get_pointer());
      return *this;
    }
    op& op::set_left(sdf<basic_operator> lhs) {
      _ptr->set_lhs(lhs.get_pointer());
      return *this;
    }
    mod::mod(basic_type type)
      : sdf(type)
    {
    }
    mod& mod::transform(sdf<basic_modifier> m) {
      _ptr->set_parent(m.get_pointer());
      return *this;
    }
  }
}