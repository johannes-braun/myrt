#pragma once

#include "xgl_parser.hpp"

namespace myrt::xgl {

  struct structure_type_entity;

  struct scope
  {
  public:
    friend struct scope_system;

    scope(std::shared_ptr<scope> const& parent, string_type name);
    scope(std::shared_ptr<scope> const& parent);

    void new_variable(var_entity var);

    void new_type(type_entity type);
    void new_function(function_entity func);
    void new_statement(statement_entity func);
    std::optional<var_entity> find_variable_by_name(string_view_type name);
    std::optional<function_entity> find_function_by_name(string_view_type name);
    std::optional<type_entity> find_type_by_name(string_view_type name);

    std::vector<var_entity> const& variables() const;
    std::vector<statement_entity> const& statements() const;

    std::vector<std::shared_ptr<scope>> const& unnamed_scopes() const;
    std::unordered_map<string_type, std::shared_ptr<scope>> const& child_scopes() const;

  private:
    std::optional<string_type> m_name;
    std::vector<type_entity> m_types;
    std::vector<var_entity> m_variables;
    std::vector<function_entity> m_functions;
    std::vector<statement_entity> m_statements;

    std::weak_ptr<scope> m_parent_scope;
    std::vector<std::shared_ptr<scope>> m_unnamed_scopes;
    std::unordered_map<string_type, std::shared_ptr<scope>> m_child_scopes;
  };

  struct scope_system
  {
  public:
    scope_system();
    scope_system(std::shared_ptr<scope> root);
    std::shared_ptr<scope> const& push_persistent(string_type name);
    std::shared_ptr<scope> const& push_unnamed();
    std::shared_ptr<scope> const& current() const;
    std::shared_ptr<scope> const& root() const;
    std::optional<type_entity> find_type(string_type const& path) const;
    std::optional<type_entity> find_type(std::span<string_type const> const& path) const;
    std::optional<function_entity> find_function(string_type const& fun) const;
    std::optional<function_entity> find_function(std::span<string_type const> const& path) const;
    std::optional<function_entity> find_function(std::initializer_list<string_type>&& path) const;
    function_entity new_function(string_type const& fun);
    std::shared_ptr<function_overload> push_function_overload(function_entity fun, std::initializer_list<var_entity> parameters);
    std::shared_ptr<function_overload> push_function_overload(function_entity fun, std::span<var_entity const> parameters);
    void next_statement(statement_entity entity);
    void pop_function_overload();
    std::optional<var_entity> find_variable(string_type const& var) const;
    std::optional<var_entity> find_variable(std::span<string_type const> const& path) const;
    std::shared_ptr<scope> find_namespace(std::span<string_type const> const& path) const;
    type_entity begin_structure(string_type name);
    void end_structure();
    void variable(var_entity var);

    void add_expr(expr_entity e) {
      m_all_expressions.push_back(e);
    }
    std::vector<expr_entity> const& all_expressions() const {
      return m_all_expressions;
    };

    void pop();

    expr_entity rotate_left(expr_entity expr);
    expr_entity rotate_right(expr_entity expr);
    expr_entity set_expr_parameters(expr_entity expr, std::span<expr_entity> parameters);

  private:
    std::optional<function_overload*> m_current_function;

    std::vector<expr_entity> m_all_expressions;
    std::shared_ptr<scope> m_current_scope;
    std::shared_ptr<scope> m_global_scope;
  };
}