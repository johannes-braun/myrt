#pragma once

#include "xgl_tokenizer.hpp"
#include <memory>
#include <optional>
#include <unordered_map>
#include <variant>
#include <span>
#include <format>
#include <iostream>

namespace myrt::xgl {

  struct function_overload;

  enum class associativity {
    rtl,
    ltr
  };

  struct type_entity_impl;
  struct type_entity
  {
    std::shared_ptr<type_entity_impl> data;
  };

  struct var_entity_impl;
  struct var_entity
  {
    std::shared_ptr<var_entity_impl> data;
  };

  struct expr_entity_impl;
  struct expr_entity
  {
    std::shared_ptr<expr_entity_impl> data;
  };

  struct statement_entity_impl;
  struct statement_entity
  {
    std::shared_ptr<statement_entity_impl> data;
  };

  struct function_entity_impl;
  struct function_entity
  {
    std::shared_ptr<function_entity_impl> data;
  };

  struct value_entity_impl;
  struct value_entity
  {
    std::shared_ptr<value_entity_impl> data;
  };

  struct builtin_type_entity {
    enum class basic_kind {
      int32_base,
      uint32_base,
      float_base,
      double_base,
      bool_base,
      void_base
    } base;

    size_t scalar_count = 1;
    string_type name;

    template<typename T, size_t Scalars = 1>
    static builtin_type_entity from_type(string_type name) {
      if constexpr (std::same_as<T, int32_t>)
        return builtin_type_entity{ basic_kind::int32_base, Scalars, std::move(name) };
      else if constexpr (std::same_as<T, uint32_t>)
        return builtin_type_entity{ basic_kind::uint32_base, Scalars, std::move(name) };
      else if constexpr (std::same_as<T, float>)
        return builtin_type_entity{ basic_kind::float_base, Scalars, std::move(name) };
      else if constexpr (std::same_as<T, double>)
        return builtin_type_entity{ basic_kind::double_base, Scalars, std::move(name) };
      else if constexpr (std::same_as<T, bool>)
        return builtin_type_entity{ basic_kind::bool_base, Scalars, std::move(name) };
      else
        static_assert(false, "This is false.");
    }
  };

  using builtin_type_kind = builtin_type_entity::basic_kind;
  struct type_entity_base;
  struct scope_system;
  struct scope;

  struct array_type_entity
  {
    std::shared_ptr<type_entity_impl> base;
    expr_entity array_size;
  };

  struct alias_type_entity
  {
    type_entity alias;
    string_type name;
  };

  struct var_entity_impl
  {
    enum class accessibility
    {
      is_public,
      is_private,
      is_protected
    } access = accessibility::is_public;
    type_entity type;
    string_type name;
    std::optional<expr_entity> value;
    bool external = false;
  };

  struct value_entity_impl
  {
    //bool is_constant = false;

    std::vector<std::byte> raw_bytes;
    //std::unordered_map<std::shared_ptr<var_entity_impl>, value_entity> member_values;

    template<typename T>
    std::optional<T> get_as_builtin() {
      if (sizeof(T) != raw_bytes.size())
        return std::nullopt;
      T val;
      std::memcpy(&val, raw_bytes.data(), sizeof(T));
      return val;
    }
  };

  value_entity make_value(void const* data, size_t bytes);

  struct structure_type_entity
  {
    std::vector<type_entity> base_types;
    std::shared_ptr<scope> scope;
    string_type name;
  };

  struct dependent_type_entity
  {
    string_type name;
  };

  struct type_dependency
  {
    type_entity dependent_type;
  };

  struct value_dependency
  {
    var_entity variable;
  };

  struct template_type_entity : structure_type_entity
  {
    using dependency = std::variant<type_dependency, value_dependency>;

    std::vector<dependency> dependencies;
  };

  enum class type_name_compare_result
  {
    equal,
    different,
    uncomparable
  };
  type_name_compare_result name_equal(type_entity_base* type, string_view_type name);
  type_name_compare_result name_equal(type_entity const& type, string_view_type name);

  struct type_entity_base
  {
    enum class type_status
    {
      is_builtin,
      is_array,
      is_alias,
      is_structure,
      is_dependent,
      is_template
    };

    std::variant<
      builtin_type_entity,
      array_type_entity,
      alias_type_entity,
      structure_type_entity,
      dependent_type_entity,
      template_type_entity
    > entity;

    type_status status() const;

    std::optional<type_entity> find_child_type(string_view_type name);
    builtin_type_entity* as_builtin();
    array_type_entity* as_array();
    alias_type_entity* as_alias();
    structure_type_entity* as_structure();
    dependent_type_entity* as_dependent();
    template_type_entity* as_template();

    const builtin_type_entity* as_builtin() const;
    const array_type_entity* as_array() const;
    const alias_type_entity* as_alias() const;
    const structure_type_entity* as_structure() const;
    const dependent_type_entity* as_dependent() const;
    const template_type_entity* as_template() const;
  };

  struct simple_statement_entity
  {
    std::optional<expr_entity> expression;
  };

  struct variable_declaration_entity
  {
    var_entity variable;
  };

  struct if_statement_entity
  {
    expr_entity condition;
    statement_entity true_block;
    statement_entity false_block;
  };

  struct for_statement_entity
  {
    expr_entity init_statement;
    expr_entity condition;
    expr_entity increment;
    statement_entity block;
  };

  struct while_statement_entity
  {
    expr_entity condition;
    statement_entity block;
  };

  struct do_while_statement_entity
  {
    expr_entity condition;
    statement_entity block;
  };

  struct block_statement_entity
  {
    std::vector<statement_entity> block;
  };

  struct return_statement_entity
  {
    std::optional<expr_entity> variable;
  };

  struct statement_entity_impl
  {
    enum class type {
      is_simple, // Simple function call, computation, etc.
      is_variable_declaration, // variable definition or declaration
      is_if, // if... or if...else statement blocks
      is_for, // for loop
      is_while, // while loop
      is_do_while, // do while loop
      is_block, // separately scoped block statement
      is_return
    };

    std::shared_ptr<scope> scope;
    std::variant<
      simple_statement_entity,
      variable_declaration_entity,
      if_statement_entity,
      for_statement_entity,
      while_statement_entity,
      do_while_statement_entity,
      block_statement_entity,
      return_statement_entity
    > data;
  };

  statement_entity make_return_statement(std::shared_ptr<scope> scope, std::optional<expr_entity> value = std::nullopt);
  statement_entity make_simple_statement(std::shared_ptr<scope> scope, std::optional<expr_entity> value = std::nullopt);
  statement_entity make_if_statement(std::shared_ptr<scope> scope, expr_entity condition);
  statement_entity make_while_statement(std::shared_ptr<scope> scope, expr_entity condition);
  statement_entity make_do_while_statement(std::shared_ptr<scope> scope, expr_entity condition);
  statement_entity make_for_statement(std::shared_ptr<scope> scope, expr_entity init, expr_entity condition, expr_entity increment);
  statement_entity make_var_statement(std::shared_ptr<scope> scope, var_entity var);
  statement_entity make_block_statement(std::shared_ptr<scope> scope, std::span<statement_entity const> block);

 /* struct expr_entity_operator {
    string_type op;
  };*/
  struct expr_entity_function {
    function_entity func;
  };
  struct expr_entity_member {
    string_view_type member_name;
  };
  struct expr_entity_construct {
    type_entity type;
  };

  struct expr_entity_variable {
    var_entity var;
  };

  struct expr_entity_literal {
    string_type value;
    type_entity type;
  };

  struct expr_entity_impl
  {
    enum class type {
      is_operator,
      is_function,
      is_literal,
      is_constructor,
      is_variable,
      is_member
    } type;

    associativity associativity = associativity::ltr;

    size_t precedence;
    string_type name;
    std::vector<expr_entity> parameters;
    //type_entity result_type;

    std::optional<value_entity> evaluated_value;

    std::variant<
      //expr_entity_operator,
      expr_entity_function,
      expr_entity_literal,
      expr_entity_construct,
      expr_entity_variable,
      expr_entity_member
    > data;
  };

  struct function_overload
  {
    // Todo: dependent types here?

    std::vector<var_entity> parameters;
    std::optional<type_entity> return_type = std::nullopt;
    std::shared_ptr<scope> body;
  };

  struct function_entity_impl
  {
    string_type name;
    enum class type {
      is_function,
      is_operator
    } type;

    std::vector<std::shared_ptr<function_overload>> available_overloads;
  };

  bool precedence_weaker(expr_entity a, expr_entity b);
  bool precedence_stronger(expr_entity a, expr_entity b);

  expr_entity make_function_expr(function_entity func);
  expr_entity make_op_expr(function_entity op);
  expr_entity make_member_expr(string_view_type op);
  expr_entity make_var_expr(var_entity var);
  expr_entity make_construct_expr(type_entity type);
  std::optional<type_entity> get_literal_type(scope_system& scope, token_iterator iter);
  expr_entity make_literal_expr(scope_system& scope, token_iterator token);

  struct type_entity_impl
  {
    type_entity_impl(type_entity_base base);
    type_entity_impl(std::shared_ptr<type_entity_base> base);

    std::shared_ptr<type_entity_base> base;
  };

  std::shared_ptr<type_entity_impl> dealias(std::shared_ptr<type_entity_impl> type);

  type_entity copy(type_entity base);

  template<typename T, size_t S>
  type_entity create_builtin_type_from(string_type name)
  {
    builtin_type_entity entity = builtin_type_entity::from_type<T, S>(std::move(name));

    auto const base = std::make_shared<type_entity_impl>(type_entity_base{
      .entity = std::move(entity)
      });

    return type_entity{
      .data = std::move(base)
    };
  }

  type_entity create_array_type(type_entity array_base, expr_entity size);
  type_entity create_alias_type(type_entity alias, string_type name);
  type_entity create_dependent_type(string_type name);
  std::optional<type_entity> get_literal_type(scope_system& scope, token_iterator iter); 
}