//#pragma once
//
//#include <string>
//#include <string_view>
//#include <vector>
//#include <map>
//#include "parameter.hpp"
//#include "parameterized.hpp"
//
//namespace myrt
//{
//  struct material_type : parameterized_type
//  {
//    std::string glsl;
//  };
//
//  struct material : parameterized<material_type>
//  {
//    material(std::shared_ptr<material_type> type) : parameterized(std::move(type)) {}
//  };
//
//  struct basic_material_type : basic_type<material_type>
//  {
//    basic_material_type(std::map<std::string, std::shared_ptr<parameter_type>> parameter_types, std::string glsl)
//      : basic_type<material_type>(std::move(parameter_types)) {
//      m_type->glsl = std::move(glsl);
//    }
//  };
//
//  struct basic_material : basic_object<material, material_type>
//  {
//    using basic_object<material, material_type>::basic_object;
//  };
//
//  struct material_glsl_assembler
//  {
//    material_glsl_assembler();
//
//    parameter_buffer_description append(std::shared_ptr<material> material);
//
//    std::string get_assembled_glsl() const;
//
//  private:
//    int m_current_id;
//    parameter_scope m_load_scope;
//    parameter_scope m_sample_scope;
//    parameter_scope m_continue_scope;
//
//    global_info m_globals;
//  };
//}