//#include "material.hpp"
//#include <myrt/sfml/utils.hpp>
//
//namespace myrt
//{
//  std::pair<int, std::string> make_material_definition(
//    global_info& globals, std::shared_ptr<material> const& mat)
//  {
//    auto const type = mat;
//    auto const hash = globals.hash(type);
//    auto const mtype_name = "MT" + std::to_string(hash);
//    if (globals.used_functions.emplace(hash).second) {
//      globals.auxilliary << "struct " << mtype_name << " {";
//      for (auto const& [pname, ptype] : type->parameters())
//        globals.auxilliary << ptype->type->type_name << ' ' << pname << ';';
//      globals.auxilliary << "};";
//
//      parameter_scope scope{};
//      scope.hash = hash;
//      scope.inline_code << "MT" << hash << " _l" << mtype_name << "(int _b){" << mtype_name << " m;";
//      scope.inline_code << "\n#define _G" << scope.hash << "(X) _G(_b + (X))\n";
//      for (auto const& [pname, par] : mat->parameters())
//      {
//        auto const name = resolve_parameter(par, scope, globals);
//        scope.inline_code << "m." << pname << "=" << name << ";";
//      }
//      scope.inline_code << "return m;}";
//      scope.inline_code << "\n#undef _G" << scope.hash << "\n";
//
//      globals.auxilliary << scope.inline_code.str();
//      auto s = replace(mat->type()->glsl, "material_load", mtype_name + "l");
//      s = replace(s, "material_state", mtype_name + "s");
//      s = replace(s, "material_sample", mtype_name + "r");
//      s = replace(s, "material_continue_ray", mtype_name + "c");
//      s = replace(s, "material", mtype_name);
//      globals.auxilliary << s;
//    }
//    return { hash, mtype_name };
//  }
//
//  material_glsl_assembler::material_glsl_assembler()
//  {
//    m_load_scope.inline_code << "int _cmt = -1;void material_load(int i){_cmt=i;switch(_MTY(_cmt)){";
//    m_load_scope.inline_code << "}}";
//
//    m_sample_scope.inline_code << "void material_sample(vec3 point, vec2 uv, vec3 normal, vec3 towards_light, vec3 towards_viewer, out vec3 reflectance, out float pdf){reflectance = vec3(1, 0, 0); pdf=1;switch(_MTY(_cmt)){";
//    m_sample_scope.inline_code << "}}";
//
//    m_continue_scope.inline_code << "vec3 material_continue_ray(vec2 r,vec3 v,vec3 n) {switch(_MTY(_cmt)){default:return vec3(0);";
//    m_continue_scope.inline_code << "}}";
//  }
//  parameter_buffer_description material_glsl_assembler::append(std::shared_ptr<material> material) {
//    parameter_buffer_description desc;
//
//    auto const last_offset = m_globals.current_offset;
//    bool const is_new = !m_globals.used_functions.contains(m_globals.hash(material->type()));
//    auto [id, name] = make_material_definition(m_globals, material);
//
//    for (auto const& [n, p] : material->parameters()) {
//      desc.append(m_globals, p);
//    }
//    desc.id = id;
//    desc.blocks_required = m_globals.current_offset - last_offset;
//    //m_globals.current_offset += desc.blocks_required;
//
//    if (is_new)
//    {
//      m_load_scope.inline_code.seekp(-2, std::ios_base::end);
//      m_load_scope.inline_code << "case " << id << ':' << name << "l(_l" << name << "(_MBO(i))); break;";
//      m_load_scope.inline_code << "}}";
//
//      m_sample_scope.inline_code.seekp(-2, std::ios_base::end);
//      m_sample_scope.inline_code << "case " << id << ':' << name << "r(point, uv, normal, towards_light, towards_viewer, reflectance, pdf); break;";
//      m_sample_scope.inline_code << "}}";
//
//      m_continue_scope.inline_code.seekp(-2, std::ios_base::end);
//      m_continue_scope.inline_code << "case " << id << ":return " << name << "c(r,v,n);";
//      m_continue_scope.inline_code << "}}";
//    }
//    return desc;
//  }
//  std::string material_glsl_assembler::get_assembled_glsl() const {
//    return "#line 0 \"myrt_gen_material\"\n" + m_globals.auxilliary.str() + m_load_scope.inline_code.str() + m_sample_scope.inline_code.str() + m_continue_scope.inline_code.str();
//  }
//}