#include "xgl_entities.hpp"
#include "xgl_scope_system.hpp"
#include "xgl_tokenizer.hpp"
#include <optional>
#include <unordered_map>
#include <variant>
#include <span>
#include <format>
#include <iostream>

namespace myrt::xgl
{
  using builtin_type_kind = builtin_type_entity::basic_kind;
  struct type_entity_base;
  struct scope_system;
  struct scope;

  type_name_compare_result name_equal(type_entity_base* type, string_view_type name);
  type_name_compare_result name_equal(type_entity const& type, string_view_type name);

  type_entity_base::type_status type_entity_base::status() const { return static_cast<type_status>(entity.index()); }

  builtin_type_entity* type_entity_base::as_builtin() {
    if (status() != type_status::is_builtin) [[unlikely]]
      return nullptr;
    return &std::get<builtin_type_entity>(entity);
  }
  array_type_entity* type_entity_base::as_array() {
    if (status() != type_status::is_array) [[unlikely]]
      return nullptr;
    return &std::get<array_type_entity>(entity);
  }
  alias_type_entity* type_entity_base::as_alias() {
    if (status() != type_status::is_alias) [[unlikely]]
      return nullptr;
    return &std::get<alias_type_entity>(entity);
  }
  structure_type_entity* type_entity_base::as_structure() {
    if (status() != type_status::is_structure) [[unlikely]]
      return nullptr;
    return &std::get<structure_type_entity>(entity);
  }
  dependent_type_entity* type_entity_base::as_dependent() {
    if (status() != type_status::is_dependent) [[unlikely]]
      return nullptr;
    return &std::get<dependent_type_entity>(entity);
  }
  template_type_entity* type_entity_base::as_template() {
    if (status() != type_status::is_template) [[unlikely]]
      return nullptr;
    return &std::get<template_type_entity>(entity);
  }

  const builtin_type_entity* type_entity_base::as_builtin() const {
    if (status() != type_status::is_builtin) [[unlikely]]
      return nullptr;
    return &std::get<builtin_type_entity>(entity);
  }
  const array_type_entity* type_entity_base::as_array() const {
    if (status() != type_status::is_array) [[unlikely]]
      return nullptr;
    return &std::get<array_type_entity>(entity);
  }
  const alias_type_entity* type_entity_base::as_alias() const {
    if (status() != type_status::is_alias) [[unlikely]]
      return nullptr;
    return &std::get<alias_type_entity>(entity);
  }
  const structure_type_entity* type_entity_base::as_structure() const {
    if (status() != type_status::is_structure) [[unlikely]]
      return nullptr;
    return &std::get<structure_type_entity>(entity);
  }
  const dependent_type_entity* type_entity_base::as_dependent() const {
    if (status() != type_status::is_dependent) [[unlikely]]
      return nullptr;
    return &std::get<dependent_type_entity>(entity);
  }
  const template_type_entity* type_entity_base::as_template() const {
    if (status() != type_status::is_template) [[unlikely]]
      return nullptr;
    return &std::get<template_type_entity>(entity);
  }

  statement_entity make_return_statement(std::shared_ptr<scope> scope, std::optional<expr_entity> value)
  {
    return { std::make_shared<statement_entity_impl>(statement_entity_impl{
      .scope = std::move(scope),
      .data = return_statement_entity{std::move(value)
      }}) };
  }

  statement_entity make_simple_statement(std::shared_ptr<scope> scope, std::optional<expr_entity> value)
  {
    return { std::make_shared<statement_entity_impl>(statement_entity_impl{
      .scope = std::move(scope),
      .data = simple_statement_entity{std::move(value)
      }}) };
  }

  statement_entity make_if_statement(std::shared_ptr<scope> scope, expr_entity condition)
  {
    return { std::make_shared<statement_entity_impl>(statement_entity_impl{
      .scope = std::move(scope),
      .data = if_statement_entity{std::move(condition)}}) };
  }

  statement_entity make_while_statement(std::shared_ptr<scope> scope, expr_entity condition)
  {
    return { std::make_shared<statement_entity_impl>(statement_entity_impl{
      .scope = std::move(scope),
      .data = while_statement_entity{std::move(condition)}}) };
  }

  statement_entity make_do_while_statement(std::shared_ptr<scope> scope, expr_entity condition)
  {
    return { std::make_shared<statement_entity_impl>(statement_entity_impl{
      .scope = std::move(scope),
      .data = do_while_statement_entity{std::move(condition)}}) };
  }

  statement_entity make_for_statement(std::shared_ptr<scope> scope, expr_entity init, expr_entity condition, expr_entity increment)
  {
    return { std::make_shared<statement_entity_impl>(statement_entity_impl{
      .scope = std::move(scope),
      .data = for_statement_entity{std::move(init), std::move(condition), std::move(increment)}}) };
  }

  statement_entity make_var_statement(std::shared_ptr<scope> scope, var_entity var)
  {
    return { std::make_shared<statement_entity_impl>(statement_entity_impl{
      .scope = std::move(scope),
      .data = variable_declaration_entity{
        std::move(var)
      }}) };
  }

  statement_entity make_block_statement(std::shared_ptr<scope> scope, std::span<statement_entity const> block)
  {
    return { std::make_shared<statement_entity_impl>(statement_entity_impl{
      .scope = std::move(scope),
      .data = block_statement_entity{
        std::vector<statement_entity>{block.begin(), block.end()}
      }}) };
  }

  bool precedence_weaker(expr_entity a, expr_entity b)
  {
    const bool weaker = a.data->precedence > b.data->precedence;
    if (weaker)
      return true;
    const bool equal = a.data->precedence == b.data->precedence;
    if (equal && a.data->associativity == associativity::rtl)
      return true;
    return false;
  }

  bool precedence_stronger(expr_entity a, expr_entity b)
  {
    const bool weaker = a.data->precedence < b.data->precedence;
    if (weaker)
      return true;
    const bool equal = a.data->precedence == b.data->precedence;
    if (equal && b.data->associativity == associativity::ltr)
      return true;
    return false;
  }

  expr_entity make_function_expr(function_entity func)
  {
    return expr_entity{ std::make_shared<expr_entity_impl>(
      expr_entity_impl{
        .type = expr_entity_impl::type::is_function,
        .associativity = associativity::ltr,
        .precedence = 2,
        .name = func.data->name,
        //.result_type = var.data->type, // Todo: determine type when inserting into full expr.
        .data = expr_entity_function {.func = func }
      }
      ) };
  }

  expr_entity make_op_expr(function_entity op)
  {
    return expr_entity{ std::make_shared<expr_entity_impl>(
      expr_entity_impl{
        .type = expr_entity_impl::type::is_operator,
        .associativity = associativity::ltr,
        .precedence = 0,
        .name = op.data->name,
        //.result_type = var.data->type, // Todo: determine type when inserting into full expr.
        .data = expr_entity_function { std::move(op) }
      }
      ) };
  }

  expr_entity make_member_expr(string_view_type name)
  {
    return expr_entity{ std::make_shared<expr_entity_impl>(
    expr_entity_impl{
      .type = expr_entity_impl::type::is_member,
      .associativity = associativity::ltr,
      .precedence = 2,
      .name = ".",
      //.result_type = var.data->type,
      .data = expr_entity_member { .member_name = name }
    }
    ) };
  }

  expr_entity make_var_expr(var_entity var) {
    return expr_entity{ std::make_shared<expr_entity_impl>(
      expr_entity_impl{
        .type = expr_entity_impl::type::is_variable,
        .associativity = associativity::ltr,
        .precedence = 0,
        .name = var.data->name,
        //.result_type = var.data->type,
        .data = expr_entity_variable {.var = var }
      }
      ) };
  }

  expr_entity make_construct_expr(type_entity type) {
    return expr_entity{ std::make_shared<expr_entity_impl>(
      expr_entity_impl{
        .type = expr_entity_impl::type::is_constructor,
        .associativity = associativity::ltr,
        .precedence = 2,
        .name = "__ctor",
        //.result_type = type,
        .data = expr_entity_construct {.type = type }
      }
      ) };
  }

  std::optional<type_entity> get_literal_type(scope_system& scope, token_iterator iter);

  expr_entity make_literal_expr(scope_system& scope, token_iterator token) {
    auto const type = get_literal_type(scope, token);

    if (!type) {
      std::format_to(std::ostreambuf_iterator(std::cerr), "\033[1;31mError [{},{}]:\033[0m Could not determine literal type for literal \"{}\".\n",
        token->line + 1, token->column_start - 1, token->text);
    }

    auto const text = string_type(token->text);
    return expr_entity{ std::make_shared<expr_entity_impl>(
      expr_entity_impl{
        .type = expr_entity_impl::type::is_literal,
        .associativity = associativity::ltr,
        .precedence = 0,
        .name = text,
        //.result_type = *type,
        .data = expr_entity_literal {.value = text, .type = *type }
      }
      ) };
  }

  value_entity make_value(void const* data, size_t bytes)
  {
    std::vector<std::byte> vec(bytes);
    std::memcpy(vec.data(), data, bytes);

    return value_entity(std::make_shared<value_entity_impl>(
      value_entity_impl{
        .raw_bytes = std::move(vec)
      }
    ));
  }

  type_entity_impl::type_entity_impl(type_entity_base base)
    : base(std::make_shared<type_entity_base>(std::move(base))) {
  }
  type_entity_impl::type_entity_impl(std::shared_ptr<type_entity_base> base)
    : base(std::move(base)) {
  }

  std::shared_ptr<type_entity_impl> dealias(std::shared_ptr<type_entity_impl> type)
  {
    while (auto* alias = type->base->as_alias())
      type = alias->alias.data;
    return type;
  }

  std::optional<type_entity> type_entity_base::find_child_type(string_view_type name) {
    switch (status()) {
    case type_status::is_structure:
    {
      auto const* str = as_structure();
      return str->scope->find_type_by_name(name);
    }
    case type_status::is_dependent:
    case type_status::is_template:
      // todo: if(!found) return make_dependent_type(name) else return found;
    case type_status::is_builtin:
    case type_status::is_array:
      return std::nullopt;
    }
  }

  type_entity copy(type_entity base)
  {
    type_entity resolved;
    resolved.data = std::make_shared<type_entity_impl>(base.data->base);
    return resolved;
  }

  type_entity create_array_type(type_entity array_base, expr_entity size)
  {
    array_type_entity entity{ .base = array_base.data, .array_size = size };

    auto const base = std::make_shared<type_entity_impl>(type_entity_base{
      .entity = std::move(entity)
      });

    return type_entity{
      .data = std::move(base)
    };
  }

  type_entity create_alias_type(type_entity alias, string_type name) {
    alias_type_entity entity{ .alias = alias, .name = name };

    auto const base = std::make_shared<type_entity_impl>(type_entity_base{
      .entity = std::move(entity)
      });

    return type_entity{
      .data = std::move(base)
    };
  }

  type_entity create_dependent_type(string_type name)
  {
    dependent_type_entity entity{ .name = name };

    auto const base = std::make_shared<type_entity_impl>(type_entity_base{
      .entity = std::move(entity)
      });

    return type_entity{
      .data = std::move(base)
    };
  }

  std::optional<type_entity> get_literal_type(scope_system& scope, token_iterator iter)
  {
    switch (iter->type)
    {
    case tokenizer::token_type::identifier:
    {
      if (iter->text == "true" || iter->text == "false")
        return scope.find_type("bool");
      break;
    }
    case tokenizer::token_type::int_literal:
      return scope.find_type("int");
    case tokenizer::token_type::uint_literal:
      return scope.find_type("uint");
    case tokenizer::token_type::float_literal:
      return scope.find_type("float");
    case tokenizer::token_type::double_literal:
      return scope.find_type("double");
    }

    return std::nullopt;
  }
}