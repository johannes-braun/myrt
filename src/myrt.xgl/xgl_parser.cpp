#include "xgl_parser.hpp"
#include <array>
#include <format>
#include <iostream>
#include <algorithm>
#include <span>
#include "xgl_scope_system.hpp"

namespace myrt::xgl
{
  std::optional<resolved_node> parser::expect_identifier(token_iterator& current, token_iterator end)
  {
    if (current == end || current->type != tokenizer::token_type::identifier)
      return std::nullopt;

    resolved_node n;
    n.children.push_back(current);
    current++;
    return n;
  }

  std::optional<resolved_node> parser::expect_literal(std::optional<tokenizer::token_type> type, token_iterator& current, token_iterator end)
  {
    if (current == end || (type && *type != current->type) || (
      current->type != tokenizer::token_type::double_literal &&
      current->type != tokenizer::token_type::float_literal &&
      current->type != tokenizer::token_type::int_literal &&
      current->type != tokenizer::token_type::uint_literal &&
      current->type != tokenizer::token_type::long_literal &&
      current->type != tokenizer::token_type::ulong_literal &&
      current->type != tokenizer::token_type::str_literal))
    {
      return std::nullopt;
    }

    resolved_node n;
    n.children.push_back(current);
    current++;
    return n;
  }

  std::optional<resolved_node> parser::expect_literal(token_iterator& current, token_iterator end)
  {
    return expect_literal(std::nullopt, current, end);
  }

  std::optional<resolved_node> parser::expect_operator(token_iterator& current, token_iterator end)
  {
    if (current == end || current->type != tokenizer::token_type::op)
      return std::nullopt;

    resolved_node n;
    n.children.push_back(current);
    current++;
    return n;
  }

  string_type parser::get_full_arithmetic_operator(resolved_node const& arithmetic_operator_node)
  {
    string_type result;
    result += arithmetic_operator_node.child_node_token(0).value()->text;

    for (int i = 1; i < arithmetic_operator_node.children.size(); ++i)
    {
      result += arithmetic_operator_node.child_node_token(i).value()->text;
    }

    return result;
  }

  std::vector<string_type> resolve_namespaced_identifier(resolved_node const& node)
  {
    std::vector<string_type> path;
    if (node.child_node("global_namespace"))
      path.push_back("__global_ns__");
    for (auto const* ident : node.child_nodes("identifier"))
      path.push_back(string_type(ident->child_token(0)->text));
    return path;
  }

  expr_entity parser::make_expression_(resolved_node const& node)
  {
    auto const basic_value = node.child_node("basic_expression_value");

    std::optional<expr_entity> final_expr;
    if (auto n = basic_value->child_node("literal"))
    {
      auto const node0 = n->children[0];
      if (node0.index() == 1)
      {
        final_expr = make_literal_expr(*m_scope_system, std::get<1>(node0).child_token(0));
      }
      else
      {
        final_expr = make_literal_expr(*m_scope_system, std::get<0>(node0));
      }
    }
    else if (auto n = basic_value->child_node("namespaced_identifier"))
    {
      auto const var = m_scope_system->find_variable(resolve_namespaced_identifier(*n));
      if (!var)
        return {};
      final_expr = make_var_expr(*var);
    }
    else if (auto n = basic_value->child_node("construct_object"))
    {
      auto const type = resolve_type(*n->child_node("basic_type"));

      auto const expr = make_construct_expr(*type);
      expr.data->precedence = m_precedence["construct_object"];
      expr.data->associativity = m_associativity[expr.data->precedence];
      for (auto const* param : n->child_nodes("compute_expression"))
        expr.data->parameters.push_back(resolve_expression_(*param));
      final_expr = expr;
    }
    else if (auto n = basic_value->child_node("function_call"))
    {
      auto fun = m_scope_system->find_function(resolve_namespaced_identifier(n->child_node(0)));

      auto func_expr = make_function_expr(*fun); //todo use function 
      func_expr.data->precedence = m_precedence["function_call"];
      func_expr.data->associativity = m_associativity[func_expr.data->precedence];

      for (auto const* chn : n->child_nodes("compute_expression"))
      {
        func_expr.data->parameters.push_back(resolve_expression_(*chn));
      }

      final_expr = func_expr;
    }
    else if (auto n = basic_value->child_node("nested_compute_expression"))
    {
      auto expr = resolve_expression_(n->child_node(0));
      expr.data->precedence = 0;
      final_expr = expr;
    }

    if (!final_expr)
      return {};

    for (auto const* postfix : node.child_nodes("arithmetic_unary_postfix_operator"))
    {
      auto const operator_symbols = "post" + get_full_arithmetic_operator(*postfix);

      std::vector<expr_entity> params;
      params.push_back(*final_expr);

      auto fn = m_scope_system->find_function("__op" + operator_symbols);
      expr_entity op = make_op_expr(*fn);
      op.data->precedence = m_precedence[operator_symbols];
      op.data->associativity = m_associativity[op.data->precedence];
      final_expr = m_scope_system->set_expr_parameters(op, params);
    }

    for (auto const* prefix : node.child_nodes("arithmetic_unary_prefix_operator"))
    {
      auto const operator_symbols = "pre" + get_full_arithmetic_operator(*prefix);

      std::vector<expr_entity> params;
      params.push_back(*final_expr);

      auto fn = m_scope_system->find_function("__op" + operator_symbols);
      expr_entity op = make_op_expr(*fn);
      op.data->precedence = m_precedence[operator_symbols];
      op.data->associativity = m_associativity[op.data->precedence];
      final_expr = m_scope_system->set_expr_parameters(op, params);
    }

    for (auto const* member : node.child_nodes("member_access"))
    {
      auto member_precedence = m_precedence["."];

      std::vector<expr_entity> params;
      params.push_back(*final_expr);

      expr_entity op = make_member_expr(member->child_node(0).child_token(0)->text);
      op.data->precedence = m_precedence["."];
      op.data->associativity = m_associativity[op.data->precedence];
      final_expr = m_scope_system->set_expr_parameters(op, params);
    }

    return *final_expr;
  }

  expr_entity parser::resolve_expression_(resolved_node const& node)
  {
    auto expr = resolve_expression(node);
    m_scope_system->add_expr(expr);
    return expr;
  }

  expr_entity parser::resolve_expression(resolved_node const& node)
  {
    if (node.identifier == "compute_expression")
      return resolve_expression_(node.child_node(0));

    if (node.identifier == "variable_definition")
    {
      parse_node(node);
      auto var = m_scope_system->current()->variables().back();
      m_scope_system->current()->new_variable(var);
      auto expr = make_var_expr(var);
      return expr;
    }
    if (node.identifier == "calculation")
    {
      if (auto ch = node.child_node(0); ch.identifier == "ternary_operation")
      {
        std::vector<expr_entity> params;
        params.push_back(resolve_expression_(ch.child_node(0)));
        params.push_back(resolve_expression_(ch.child_node(1)));
        params.push_back(resolve_expression_(ch.child_node(2)));

        auto fn = m_scope_system->find_function("__op3?:");
        expr_entity ternary_op = make_op_expr(*fn);
        ternary_op.data->precedence = m_precedence["ternary"];
        ternary_op = m_scope_system->set_expr_parameters(ternary_op, params);

        return ternary_op;
      }
      else
      {
        std::vector<expr_entity> params;
        params.push_back(resolve_expression_(node.child_node(0)));
        params.push_back(resolve_expression_(node.child_node(2)));

        auto const operator_symbols = get_full_arithmetic_operator(node.child_node(1));
        auto fn = m_scope_system->find_function("__op" + operator_symbols);
        expr_entity op = make_op_expr(*fn);
        op.data->precedence = m_precedence[operator_symbols];
        op.data->associativity = m_associativity[op.data->precedence];
        op = m_scope_system->set_expr_parameters(op, params);

        return op;
      }
    }
    else if (node.identifier == "expression_value")
    {
      return make_expression_(node);
    }
  }

  parser::parser()
  {
    m_scope_system = std::make_unique<scope_system>();
    build_grammar();
  }

  std::shared_ptr<scope_system> parser::parse(tokenizer const& tokenizer)
  {
    auto const ast = m_grammar.resolve(tokenizer.tokens().begin(), tokenizer.tokens().end());
    for (auto const& n : ast)
      parse_node(n);
    return m_scope_system;
  }

  void parser::build_predefined_functions()
  {
#define nf(OP) m_scope_system->new_function(OP);
    nf("__op+");
    nf("__op-");
    nf("__oppost++");
    nf("__oppost--");
    nf("__oppre++");
    nf("__oppre--");
    nf("__oppre+");
    nf("__oppre-");
    nf("__oppre~");
    nf("__oppre!");
    nf("__op*");
    nf("__op/");
    nf("__op%");
    nf("__op<<");
    nf("__op>>");
    nf("__op<");
    nf("__op>");
    nf("__op>=");
    nf("__op<=");
    nf("__op==");
    nf("__op!=");
    nf("__op=");
    nf("__op|");
    nf("__op&");
    nf("__op^");
    nf("__op||");
    nf("__op&&");

    nf("__op+=");
    nf("__op-=");
    nf("__op*=");
    nf("__op/=");
    nf("__op%=");
    nf("__op^=");
    nf("__op&=");
    nf("__op|=");
    nf("__op<<=");
    nf("__op>>=");
  }

  void parser::build_grammar()
  {
    m_grammar.set_root("global");
    m_grammar += flatten || "global" &&
      "type_alias"_expr |
      "meta_version"_expr |
      "meta_extension"_expr |
      "namespace_block"_expr |
      "class_definition"_expr |
      "function_definition"_expr |
      "function_declaration"_expr |
      "variable_definition ;#"_expr;
    m_grammar += flatten || "global_list" &&
      "global global_list?"_expr;

    m_grammar += "type_alias" &&
      "alias# identifier =# basic_type ;#"_expr;

    m_grammar += "namespace_block" &&
      "namespace# identifier {# global_list? }#"_expr;

    m_grammar += "meta_version" &&
      "version literal_int profile? ;#"_expr;

    m_grammar += "profile" &&
      "core"_expr |
      "compat"_expr;

    m_grammar += "meta_extension" &&
      "enable literal_string ;#"_expr |
      "require literal_string ;#"_expr;

    m_grammar += "class_definition" &&
      "template_signature? class# identifier base_classes_declarator? {# class_members? }# ;#"_expr;

    m_grammar += "template_signature" &&
      "template# <# template_dependencies? >#"_expr;

    m_grammar += flatten || "template_dependencies" &&
      "template_dependency ,# template_dependencies"_expr |
      "template_dependency"_expr;

    m_grammar += "template_dependency" &&
      "template_dependency_type"_expr |
      "template_dependency_value"_expr;
    m_grammar += "template_dependency_type" &&
      "type# identifier"_expr;
    m_grammar += "template_dependency_value" &&
      "basic_type identifier"_expr;

    m_grammar += flatten || "base_classes_declarator" &&
      ":# base_classes"_expr;

    m_grammar += "base_class" &&
      "identifier"_expr;
    m_grammar += flatten || "base_classes" &&
      "base_class , base_classes"_expr |
      "base_class"_expr;
    m_grammar += flatten || "class_members" &&
      "class_member ;# class_members?"_expr;
    m_grammar += "class_member" &&
      "variable_definition"_expr;

    m_grammar += "function_declaration" &&
      "basic_type function_name (# function_parameter_list? )# ;#"_expr;
    m_grammar += "function_definition" &&
      "basic_type function_name (# function_parameter_list? )# function_body"_expr;

    m_grammar += "function_parameter" &&
      "parameter_type identifier?"_expr;
    m_grammar += flatten || "function_parameter_list" &&
      "function_parameter ,# function_parameter_list"_expr |
      "function_parameter"_expr;

    m_grammar += "function_name" &&
      "op arithmetic_operator"_expr |
      "identifier"_expr;

    m_grammar += "function_body" &&
      "function_body_direct"_expr |
      "function_body_indirect"_expr;
    m_grammar += "function_body_direct" &&
      "=# ># compute_expression ;#"_expr;

    m_grammar += "function_body_indirect" &&
      "{# statement_list? }#"_expr;

    m_grammar += "statement" &&
      "type_alias"_expr |
      "return_statement"_expr |
      "if_statement"_expr |
      "for_statement"_expr |
      "while_statement"_expr |
      "do_while_statement"_expr |
      "compute_expression ;#"_expr |
      "variable_definition ;#"_expr |
      ";"_expr;

    m_grammar += "if_statement" &&
      "if# (# compute_expression_or_definition )# if_true_block else_block?"_expr;

    m_grammar += flatten || "else_block" &&
      "else# if_false_block"_expr;

    m_grammar += "if_true_block" &&
      "block_statement"_expr;
    m_grammar += "if_false_block" &&
      "block_statement"_expr;

    m_grammar += "while_statement" &&
      "while# (# while_condition )# block_statement"_expr;
    m_grammar += "do_while_statement" &&
      "do# {# statement_list? }# while# (# while_condition )# ;#"_expr;

    m_grammar += "while_condition" &&
      "compute_expression_or_definition"_expr;

    m_grammar += "for_statement" &&
      "for# (# for_vardef? ;# for_condition? ;# for_increment? )# block_statement"_expr;

    m_grammar += "for_vardef" &&
      "compute_expression_or_definition"_expr;
    m_grammar += "for_condition" &&
      "compute_expression_or_definition"_expr;
    m_grammar += "for_increment" &&
      "compute_expression"_expr;

    m_grammar += flatten || "block_statement" &&
      "{# statement_list? }#"_expr |
      "statement"_expr;

    m_grammar += "return_statement" &&
      "return# compute_expression ;#"_expr;

    m_grammar += flatten || "statement_list" &&
      "statement statement_list?"_expr;

    m_grammar += "variable_definition" &&
      "basic_type variable_name =# variable_init"_expr |
      "basic_type variable_name"_expr;

    m_grammar += "variable_init" &&
      "compute_expression"_expr;

    m_grammar += "variable_name" &&
      "identifier"_expr;

    m_grammar += "array_declarator" &&
      "[# compute_expression ]#"_expr;

    m_grammar += flatten || "compute_expression_or_definition" &&
      "variable_definition"_expr |
      "compute_expression"_expr;

    m_grammar += "compute_expression" &&
      "calculation"_expr |
      "expression_value"_expr;

    m_grammar += "calculation" &&
      "ternary_operation"_expr |
      "expression_value arithmetic_operator compute_expression"_expr;
    m_grammar += "ternary_operation" &&
      "expression_value ?# compute_expression :# compute_expression"_expr;

    m_grammar += "arithmetic_unary_prefix_operator" &&
      "+ +"_expr |
      "- -"_expr |
      "+"_expr |
      "-"_expr |
      "!"_expr |
      "~"_expr;

    m_grammar += "arithmetic_unary_postfix_operator" &&
      "+ +"_expr |
      "- -"_expr;

    m_grammar += "arithmetic_operator" &&
      "< < ="_expr |
      "> > ="_expr |
      "? :"_expr |
      "+ ="_expr |
      "- ="_expr |
      "* ="_expr |
      "/ ="_expr |
      "% ="_expr |
      "& ="_expr |
      "| ="_expr |
      "^ ="_expr |
      "= ="_expr |
      "! ="_expr |
      "< ="_expr |
      "> ="_expr |
      "& &"_expr |
      "| |"_expr |
      "< <"_expr |
      "> >"_expr |
      "+-*/%&|^=<>"_any_char;

    m_associativity[1] = associativity::ltr;
    m_associativity[2] = associativity::ltr;
    m_associativity[3] = associativity::rtl;
    m_associativity[4] = associativity::ltr;
    m_associativity[5] = associativity::ltr;
    m_associativity[6] = associativity::ltr;
    m_associativity[7] = associativity::ltr;
    m_associativity[8] = associativity::ltr;
    m_associativity[9] = associativity::ltr;
    m_associativity[10] = associativity::ltr;
    m_associativity[11] = associativity::ltr;
    m_associativity[12] = associativity::ltr;
    m_associativity[13] = associativity::ltr;
    m_associativity[14] = associativity::ltr;
    m_associativity[15] = associativity::ltr;
    m_associativity[16] = associativity::rtl;
    m_associativity[17] = associativity::ltr;

    m_precedence["definition"] = 2;
    m_precedence["function_call"] = 2;
    m_precedence["construct_object"] = 2;
    m_precedence["post++"] = 2;
    m_precedence["post--"] = 2;
    m_precedence["ternary"] = 16;

    m_precedence["pre++"] = 3;
    m_precedence["pre--"] = 3;
    m_precedence["pre+"] = 3;
    m_precedence["pre-"] = 3;
    m_precedence["pre!"] = 3;
    m_precedence["pre~"] = 3;

    m_precedence["."] = 2;
    m_precedence["[]"] = 2;

    m_precedence["*"] =
      m_precedence["/"] =
      m_precedence["%"] = 5;
    m_precedence["+"] =
      m_precedence["-"] = 6;
    m_precedence["<<"] =
      m_precedence[">>"] = 7;
    m_precedence["<"] =
      m_precedence[">"] =
      m_precedence["<="] =
      m_precedence[">="] = 9;
    m_precedence["=="] =
      m_precedence["!="] = 10;

    m_precedence["&"] = 11;
    m_precedence["^"] = 12;
    m_precedence["|"] = 13;
    m_precedence["&&"] = 14;
    m_precedence["||"] = 15;

    m_precedence["?:"] = 16;
    m_precedence["="] = 16;
    m_precedence["+="] = 16;
    m_precedence["+="] = 16;
    m_precedence["*="] = 16;
    m_precedence["/="] = 16;
    m_precedence["%="] = 16;
    m_precedence["<<="] = 16;
    m_precedence[">>="] = 16;
    m_precedence["&="] = 16;
    m_precedence["|="] = 16;
    m_precedence["^="] = 16;
    m_precedence["|="] = 16;

    m_grammar += flatten || "expression_value_postfix" &&
      "arithmetic_unary_postfix_operator expression_value_postfix?"_expr;
    m_grammar += flatten || "expression_value_prefix" &&
      "arithmetic_unary_prefix_operator expression_value_prefix?"_expr;

    m_grammar += "expression_value" &&
      "expression_value_prefix? basic_expression_value array_declarator? get_member_expression_postfix? expression_value_postfix?"_expr;

    m_grammar += "basic_expression_value" &&
      "nested_compute_expression"_expr |
      "function_call"_expr |
      "construct_object"_expr |
      "literal"_expr |
      "namespaced_identifier"_expr;

    m_grammar += "namespaced_identifier" &&
      "global_namespace? identifier namespace_chain?"_expr;

    m_grammar += flatten || "namespace_chain" &&
      ":# :# identifier namespace_chain?"_expr;

    m_grammar += "global_namespace" &&
      ":# :#"_expr;

    m_grammar += flatten || "get_member_expression_postfix" &&
      ".# member_access get_member_expression_postfix?"_expr;
    m_grammar += "member_access" &&
      "identifier array_declarator?"_expr;

    m_grammar += "function_call" &&
      "namespaced_identifier (# compute_expression_list? )#"_expr;
    m_grammar += "nested_compute_expression" &&
      "(# compute_expression )#"_expr;

    m_grammar += "construct_object" &&
      "basic_type {# compute_expression_list? }#"_expr;

    m_grammar += flatten || "compute_expression_list" &&
      "compute_expression ,# compute_expression_list"_expr |
      "compute_expression"_expr;

    m_grammar += "parameter_type" &&
      "type_decorator_list? basic_type type_decorator_list?"_expr;

    m_grammar += "basic_type" &&
      "namespaced_identifier array_declarator_list?"_expr;

    m_grammar += flatten || "array_declarator_list" &&
      "array_declarator array_declarator_list?"_expr;

    m_grammar += flatten || "type_decorator_list" &&
      "type_decorator type_decorator_list?"_expr;

    m_grammar += "type_decorator" &&
      "const"_expr |
      "in"_expr |
      "out"_expr |
      "inout"_expr;

    m_grammar += "identifier" && "builtin_identifier"_expr;
    m_grammar += "operator" && "builtin_operator"_expr;
    m_grammar += "literal" && "builtin_literal"_expr | "true"_expr | "false"_expr;
    m_grammar += "literal_string" && "builtin_literal_string"_expr;
    m_grammar += "literal_int" && "builtin_literal_int"_expr;

    m_grammar += "builtin_identifier" && [this](token_iterator& c, token_iterator e) { return expect_identifier(c, e); };
    m_grammar += "builtin_operator" && [this](token_iterator& c, token_iterator e) { return expect_operator(c, e); };
    m_grammar += "builtin_literal" && [this](token_iterator& c, token_iterator e) { return expect_literal(c, e); };
    m_grammar += "builtin_literal_string" && [this](token_iterator& c, token_iterator e) { return expect_literal(tokenizer::token_type::str_literal, c, e); };
    m_grammar += "builtin_literal_int" && [this](token_iterator& c, token_iterator e) { return expect_literal(tokenizer::token_type::int_literal, c, e); };


    m_scope_system->current()->new_type(create_builtin_type_from<bool, 1>("bool"));
    m_scope_system->current()->new_type(create_builtin_type_from<bool, 2>("bool2"));
    m_scope_system->current()->new_type(create_builtin_type_from<bool, 3>("bool3"));
    m_scope_system->current()->new_type(create_builtin_type_from<bool, 4>("bool4"));
    m_scope_system->current()->new_type(create_builtin_type_from<int32_t, 1>("int"));
    m_scope_system->current()->new_type(create_builtin_type_from<int32_t, 2>("int2"));
    m_scope_system->current()->new_type(create_builtin_type_from<int32_t, 3>("int3"));
    m_scope_system->current()->new_type(create_builtin_type_from<int32_t, 4>("int4"));
    m_scope_system->current()->new_type(create_builtin_type_from<uint32_t, 1>("uint"));
    m_scope_system->current()->new_type(create_builtin_type_from<uint32_t, 2>("uint2"));
    m_scope_system->current()->new_type(create_builtin_type_from<uint32_t, 3>("uint3"));
    m_scope_system->current()->new_type(create_builtin_type_from<uint32_t, 4>("uint4"));
    m_scope_system->current()->new_type(create_builtin_type_from<float, 1>("float"));
    m_scope_system->current()->new_type(create_builtin_type_from<float, 2>("float2"));
    m_scope_system->current()->new_type(create_builtin_type_from<float, 3>("float3"));
    m_scope_system->current()->new_type(create_builtin_type_from<float, 4>("float4"));
    m_scope_system->current()->new_type(create_builtin_type_from<float, 4>("mat2"));
    m_scope_system->current()->new_type(create_builtin_type_from<float, 9>("mat3"));
    m_scope_system->current()->new_type(create_builtin_type_from<float, 16>("mat4"));

    build_predefined_functions();

    m_parsers["type_alias"] = {
      [&](resolved_node const& n) {
        auto name = n.child_node_token(0).value()->text;
        auto type = m_scope_system->find_type(resolve_namespaced_identifier(n.child_node(1).child_node(0)));
        m_scope_system->current()->new_type(create_alias_type(*type, string_type(name)));
        return true;
      }
    };

    m_parsers["function_body_direct"] = {
      [&](resolved_node const& n) {
        auto expr = resolve_expression_(n.child_node(0));
        m_scope_system->next_statement(make_return_statement(m_scope_system->current(), std::move(expr)));
        return true;
      }
    };

    m_parsers["namespace_block"] = {
      [&](resolved_node const& n) {
        m_scope_system->push_persistent(string_type(n.child_node_token(0).value()->text));
        return false;
      },
      [&](resolved_node const& n) {
        m_scope_system->pop();
      }
    };

    m_parsers["class_definition"] = {
      [&](resolved_node const& n) {

        auto new_type = m_scope_system->begin_structure(string_type(n.child_node("identifier")->child_token(0)->text));
        auto* structure = new_type.data->base->as_structure();

        auto template_signature = n.child_node("template_signature");
        if (template_signature)
        {
          template_type_entity templ;
          static_cast<structure_type_entity&>(templ) = std::move(*structure);

          for (auto const* dep : template_signature->child_nodes("template_dependency"))
          {
            if (auto type_dep = dep->child_node("template_dependency_type"))
            {
              auto const dep_type = create_dependent_type(string_type(type_dep->child_node_token(0).value()->text));
              m_scope_system->current()->new_type(dep_type);
              templ.dependencies.push_back(type_dependency{ dep_type });
            }
            else if (auto val_dep = dep->child_node("template_dependency_value"))
            {
              auto const basic_type = val_dep->child_node("basic_type");
              auto const var_name = val_dep->child_node("identifier")->child_token(0)->text;

              auto type = resolve_type(*basic_type);

              var_entity var{ std::make_shared<var_entity_impl>(var_entity_impl{
                 .type = *type,
                .name = string_type(var_name),
                .value = std::nullopt // todo. ?
              }) };
              m_scope_system->variable(std::move(var));
              templ.dependencies.push_back(value_dependency{ var });
            }
          }

          new_type.data->base->entity = templ;
          structure = static_cast<structure_type_entity*>(new_type.data->base->as_template());
        }

        for (auto const* member : n.child_nodes("class_member"))
        {
          parse_node(*member);
        }

        for (auto const* member : n.child_nodes("base_class"))
        {
          auto const name_token = *member->child_node_token(0);
          std::array x = { string_type(name_token->text) };
          auto ty = m_scope_system->find_type(x);
          if (ty)
            structure->base_types.push_back(*ty);
          else
          {
            std::format_to(std::ostreambuf_iterator(std::cerr), "\033[1;31mError [{},{}]:\033[0m {} is not a type.\n",
              name_token->line + 1, name_token->column_start - 1, name_token->text);
          }
        }

        m_scope_system->end_structure();

        return true;
      }

    };

    m_parsers["function_definition"] = { [&](auto const& n) {
      return parse_function(n);
    } };

    m_parsers["function_declaration"] = { [&](auto const& n) {
      return parse_function(n);
    } };

    m_parsers["calculation"] = { [&](resolved_node const& n) {
      auto const expr = resolve_expression_(n);
      return true;
    } };
    m_parsers["expression_value"] = { [&](resolved_node const& n) {
      auto const expr = resolve_expression_(n);
      return true;
    } };
    m_parsers["compute_expression"] = { [&](resolved_node const& n) {
      auto const expr = resolve_expression_(n);
      return true;
    } };

    m_parsers["statement"] = {
      [&](resolved_node const& node) {
        auto const child0 = node.child_node(0);
        if (child0.identifier == "return_statement")
        {
          std::optional<expr_entity> val;
          if (!child0.children.empty())
          {
            auto const value = child0.child_node(0);
            val = resolve_expression_(value);
          }

          m_scope_system->next_statement(make_return_statement(m_scope_system->current(), val));
        }
        else if (child0.identifier == "variable_definition")
        {
          parse_node(child0);
          auto last_var = m_scope_system->current()->variables().back();
          m_scope_system->next_statement(make_var_statement(m_scope_system->current(), last_var));
        }
        else if (child0.identifier == "compute_expression")
        {
          m_scope_system->next_statement(make_simple_statement(m_scope_system->current(), resolve_expression_(child0)));
        }
        else if (child0.identifier == "expression_value" || child0.identifier == "calculation")
        {
          m_scope_system->next_statement(make_simple_statement(m_scope_system->current(), resolve_expression_(child0)));
        }
        else if (child0.identifier == "if_statement")
        {
          m_scope_system->push_unnamed();
          auto if_st = make_if_statement(m_scope_system->current(), resolve_expression_(child0.child_node(0)));
          auto scope = m_scope_system->push_unnamed();
          auto& if_internal = std::get<if_statement_entity>(if_st.data->data);
          parse_node(*child0.child_node("if_true_block"));
          if_internal.true_block = make_block_statement(m_scope_system->current(), scope->statements());
          m_scope_system->pop();

          auto false_block = child0.child_node("if_false_block");
          if (false_block)
          {
            auto scope = m_scope_system->push_unnamed();
            parse_node(*false_block);
            if_internal.false_block = make_block_statement(m_scope_system->current(), scope->statements());
            m_scope_system->pop();
          }
         }
         else if (child0.identifier == "while_statement")
         {
           m_scope_system->push_unnamed();
           auto while_st = make_while_statement(m_scope_system->current(), resolve_expression_(child0.child_node("while_condition")->child_node(0)));
           auto scope = m_scope_system->push_unnamed();
           auto& while_internal = std::get<while_statement_entity>(while_st.data->data);
           auto const body = child0.child_node("block_statement");
           if (body)
             parse_node(*body);
           while_internal.block = make_block_statement(m_scope_system->current(), scope->statements());
           m_scope_system->pop();
         }
         else if (child0.identifier == "do_while_statement")
         {
           m_scope_system->push_unnamed();
           auto while_st = make_do_while_statement(m_scope_system->current(), resolve_expression_(child0.child_node("while_condition")->child_node(0)));
           auto scope = m_scope_system->push_unnamed();
           auto& while_internal = std::get<do_while_statement_entity>(while_st.data->data);
           for (auto const* body : child0.child_nodes("statement"))
             parse_node(*body);
           while_internal.block = make_block_statement(m_scope_system->current(), scope->statements());
           m_scope_system->pop();
          }
          else if (child0.identifier == "for_statement")
          {
            m_scope_system->push_unnamed();
            auto const init = resolve_expression_(child0.child_node("for_vardef")->child_node(0));
            auto const cond = resolve_expression_(child0.child_node("for_condition")->child_node(0));
            auto const incr = resolve_expression_(child0.child_node("for_increment")->child_node(0));
            auto for_st = make_for_statement(m_scope_system->current(),
              init, cond, incr
            );
            auto scope = m_scope_system->push_unnamed();
            auto& while_internal = std::get<for_statement_entity>(for_st.data->data);
            auto const body = child0.child_node("block_statement");
            if (body)
              parse_node(*body);
            while_internal.block = make_block_statement(m_scope_system->current(), scope->statements());
            m_scope_system->pop();
          }
          else
        {
          parse_node(child0);
        }
          return true;
        }
    };

    m_parsers["variable_definition"] = { [&](resolved_node const& n) {
      auto const type_node = *n.child_node("basic_type");
      std::optional<expr_entity> value;
      if (auto opt_init = n.child_node("variable_init"))
        value = resolve_expression_(opt_init->child_node(0));

       m_scope_system->current()->new_variable(var_entity{
         .data = std::make_shared<var_entity_impl>(var_entity_impl{
           .type = *resolve_type(type_node),
           .name = string_type(n.child_node("variable_name")->child_node_token(0).value()->text),
           .value = value
         })
         });
       return true;
     } };
  }

  std::optional<type_entity> parser::resolve_type(resolved_node const& basic_type)
  {
    auto type = m_scope_system->find_type(resolve_namespaced_identifier(basic_type.child_node(0)));
    if (!type)
      return std::nullopt;

    for (auto const* arr : basic_type.child_nodes("array_declarator"))
    {
      auto const expr = resolve_expression_(arr->child_node(0));
      type = create_array_type(*type, expr);
    }
    return type;
  }

  void parser::parse_node(resolved_node const& node)
  {
    auto const it = m_parsers.find(node.identifier);
    if (it != m_parsers.end() && it->second.on_begin && it->second.on_begin.value()(node))
    {
      if (it->second.on_end)
        it->second.on_end.value()(node);
      return;
    }

    for (auto const& child : node.children)
    {
      if (child.index() == 1) [[likely]] {
        parse_node(std::get<1>(child));
      }
      else
      {
        std::format_to(std::ostreambuf_iterator(std::cout), "\033[1;35m{}\033[0m\n", std::get<0>(child)->text);
      }
    }

    if (it != m_parsers.end() && it->second.on_end)
    {
      it->second.on_end.value()(node);
    }
  }

  std::optional<decorator> get_decorator(string_view_type str)
  {
    if (str == "const")
      return decorator::const_dec;
    else if (str == "in")
      return decorator::in_dec;
    else if (str == "out")
      return decorator::out_dec;
    else if (str == "inout")
      return decorator::inout_dec;
    return std::nullopt;
  }

  bool parser::parse_function(resolved_node const& node)
  {
    auto fn_name = string_type(node.child_node("function_name")->child_node_token(0).value()->text);
    auto func = m_scope_system->find_function(fn_name);
    if (!func)
      func = m_scope_system->new_function(fn_name);

    std::vector<var_entity> variables;
    for (auto const* par : node.child_nodes("function_parameter"))
    {
      auto var_ident = par->child_node_token("identifier");
      string_type var_name;

      if (var_ident)
      {
        var_name = (par->child_node_token("identifier").value()->text);
      }

      auto const basic_type = par->child_node("parameter_type")->child_node("basic_type");

      auto type = resolve_type(*basic_type);

      var_entity var{ std::make_shared<var_entity_impl>(var_entity_impl{
         .type = *type,
        .name = var_name,
        .value = std::nullopt // todo.
      }) };

      variables.push_back(std::move(var));
    }

    m_scope_system->push_function_overload(*func, variables);

    if (node.identifier == "function_definition")
      parse_node(*node.child_node("function_body"));

    m_scope_system->pop_function_overload();
    return true;
  }
}