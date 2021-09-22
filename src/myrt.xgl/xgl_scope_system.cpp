#include "xgl_scope_system.hpp"

namespace myrt::xgl
{
  type_name_compare_result name_equal(type_entity_base* type, string_view_type name)
  {
    if (type->status() == type_entity_base::type_status::is_builtin)
      return name == type->as_builtin()->name ? type_name_compare_result::equal : type_name_compare_result::different;
    else if (type->status() == type_entity_base::type_status::is_structure)
      return name == type->as_structure()->name ? type_name_compare_result::equal : type_name_compare_result::different;
    else if (type->status() == type_entity_base::type_status::is_dependent)
      return name == type->as_dependent()->name ? type_name_compare_result::equal : type_name_compare_result::different;
    else if (type->status() == type_entity_base::type_status::is_template)
      return name == type->as_template()->name ? type_name_compare_result::equal : type_name_compare_result::different;
    else if (type->status() == type_entity_base::type_status::is_alias)
      return name == type->as_alias()->name ? type_name_compare_result::equal : type_name_compare_result::different;
    return type_name_compare_result::uncomparable;
  }
  type_name_compare_result name_equal(type_entity const& type, string_view_type name)
  {
    return name_equal(type.data->base.get(), name);
  }

  scope::scope(std::shared_ptr<scope> const& parent, string_type name) : m_parent_scope(parent), m_name(std::move(name)) {}
  scope::scope(std::shared_ptr<scope> const& parent) : m_parent_scope(parent) {}

  void scope::new_variable(var_entity var) {
    m_variables.push_back(std::move(var));
  }

  void scope::new_type(type_entity type) {
    m_types.push_back(std::move(type));
  }
  void scope::new_function(function_entity func) {
    m_functions.push_back(std::move(func));
  }
  void scope::new_statement(statement_entity func) {
    m_statements.push_back(std::move(func));
  }

  std::optional<var_entity> scope::find_variable_by_name(string_view_type name) {
    auto const iter = std::ranges::find_if(m_variables, [&](var_entity const& var) {
      return var.data->name == name;
      });
    if (iter == m_variables.end())
      return std::nullopt;
    return *iter;
  }

  std::optional<function_entity> scope::find_function_by_name(string_view_type name) {
    auto const iter = std::ranges::find_if(m_functions, [&](function_entity const& var) {
      return var.data->name == name;
      });
    if (iter == m_functions.end())
      return std::nullopt;
    return *iter;
  }

  std::optional<type_entity> scope::find_type_by_name(string_view_type name) {
    auto const iter = std::ranges::find_if(m_types, [&](type_entity const& type) {
      return name_equal(type, name) == type_name_compare_result::equal;
      });
    if (iter == m_types.end())
      return std::nullopt;
    return *iter;
  }

  std::vector<var_entity> const& scope::variables() const { return m_variables; }
  std::vector<statement_entity> const& scope::statements() const { return m_statements; }

  std::vector<std::shared_ptr<scope>> const& scope::unnamed_scopes() const
  {
    return m_unnamed_scopes;
  }

  std::unordered_map<string_type, std::shared_ptr<scope>> const& scope::child_scopes() const
  {
    return m_child_scopes;
  }

  scope_system::scope_system() {
    m_global_scope = std::make_shared<scope>(nullptr, "__global_ns__");
    m_current_scope = m_global_scope;
  }

  scope_system::scope_system(std::shared_ptr<scope> root)
    : m_global_scope(std::move(root))
  {
    m_current_scope = m_global_scope;
  }

  std::shared_ptr<scope> const& scope_system::push_persistent(string_type name) {
    auto const scope_it = m_current_scope->m_child_scopes.find(name);
    if (scope_it == m_current_scope->m_child_scopes.end())
    {
      auto s = std::make_shared<scope>(m_current_scope, name);
      m_current_scope->m_child_scopes[name] = s;
      m_current_scope = s;
    }
    else
    {
      m_current_scope = scope_it->second;
    }
    return m_current_scope;
  }

  std::shared_ptr<scope> const& scope_system::push_unnamed() {
    m_current_scope->m_unnamed_scopes.push_back(std::make_shared<scope>(m_current_scope));
    m_current_scope = m_current_scope->m_unnamed_scopes.back();
    return m_current_scope;
  }

  std::shared_ptr<scope> const& scope_system::current() const {
    return m_current_scope;
  }
  std::shared_ptr<scope> const& scope_system::root() const {
    return m_global_scope;
  }

  std::optional<type_entity> scope_system::find_type(string_type const& path) const {
    return find_type({ &path, 1 });
  }

  std::optional<type_entity> scope_system::find_type(std::span<string_type const> const& path) const {
    std::shared_ptr<scope> found_scope;
    size_t end_of_namespace = 0;
    for (size_t i = 0; i < path.size(); ++i) {
      auto query = find_namespace(path.subspan(0, i + 1));
      if (query)
      {
        found_scope = query;
        end_of_namespace = i + 1;
      }
      else
        break;
    }

    if (!found_scope && end_of_namespace == 0)
      found_scope = current();

    if (!found_scope)
      return std::nullopt;

    std::optional<type_entity> base_type;
    while (true)
    {
      base_type = found_scope->find_type_by_name(path[end_of_namespace]);
      bool next_parent = !base_type;
      // todo: if end_of_namespace == 0, check parents if child==nullptr
      for (size_t i = end_of_namespace + 1; i < path.size(); ++i)
      {
        auto child = base_type->data->base->find_child_type(path[i]);
        if (!child)
        {
          if (end_of_namespace != 0)
            return std::nullopt;
          if (!found_scope->m_parent_scope.expired())
          {
            next_parent = true;
          }
          else
            return std::nullopt;
          break;
        }
        base_type = std::move(child);
      }

      if (next_parent)
      {
        found_scope = found_scope->m_parent_scope.lock();
        continue;
      }
      break;
    }
    return base_type;
  }

  std::optional<function_entity> scope_system::find_function(string_type const& fun) const {
    return find_function({ &fun, 1 });
  }

  std::optional<function_entity> scope_system::find_function(std::span<string_type const> const& path) const {
    std::shared_ptr<scope> found_scope;
    size_t end_of_namespace = 0;
    for (size_t i = 0; i < path.size(); ++i) {
      auto query = find_namespace(path.subspan(0, i + 1));
      if (query)
      {
        found_scope = query;
        end_of_namespace = i + 1;
      }
      else
        break;
    }

    if (!found_scope && end_of_namespace == 0)
      found_scope = current();

    if (!found_scope || end_of_namespace != path.size() - 1)
      return std::nullopt;

    auto fn = found_scope->find_function_by_name(path[end_of_namespace]);
    while (!fn && !found_scope->m_parent_scope.expired())
    {
      found_scope = found_scope->m_parent_scope.lock();
      fn = found_scope->find_function_by_name(path[end_of_namespace]);
    }
    return fn;
  }

  std::optional<function_entity> scope_system::find_function(std::initializer_list<string_type>&& path) const
  {
    return find_function(std::span<string_type const>(std::data(path), std::size(path)));
  }

  function_entity scope_system::new_function(string_type const& fun)
  {
    auto func = function_entity{ .data = std::make_shared<function_entity_impl>(function_entity_impl{
      .name = fun,
      .type = function_entity_impl::type::is_function
      }) };
    current()->new_function(func);
    return func;
  }

  std::shared_ptr<function_overload> scope_system::push_function_overload(function_entity fun, std::initializer_list<var_entity> parameters)
  {
    return push_function_overload(std::move(fun), std::span(parameters));
  }

  std::shared_ptr<function_overload> scope_system::push_function_overload(function_entity fun, std::span<var_entity const> parameters)
  {
    std::shared_ptr<function_overload> overload = std::make_shared<function_overload>();
    overload->parameters = { parameters.begin(), parameters.end() };
    auto& scope = push_unnamed();
    for (auto const& var : parameters)
      scope->new_variable(var);
    overload->body = scope;
    fun.data->available_overloads.push_back(std::move(overload));
    m_current_function = fun.data->available_overloads.back().get();

    return fun.data->available_overloads.back();
  }

  void scope_system::next_statement(statement_entity entity)
  {
    current()->new_statement(entity);
  }

  void scope_system::pop_function_overload()
  {
    m_current_function = std::nullopt;
    pop();
  }

  std::optional<var_entity> scope_system::find_variable(string_type const& var) const {
    return find_variable({ &var, 1 });
  }

  std::optional<var_entity> scope_system::find_variable(std::span<string_type const> const& path) const {
    std::shared_ptr<scope> found_scope;
    size_t end_of_namespace = 0;
    for (size_t i = 0; i < path.size(); ++i) {
      auto query = find_namespace(path.subspan(0, i + 1));
      if (query)
      {
        found_scope = query;
        end_of_namespace = i + 1;
      }
      else
        break;
    }

    if (!found_scope && end_of_namespace == 0)
      found_scope = current();

    if (!found_scope || end_of_namespace != path.size() - 1)
      return std::nullopt;

    auto var = found_scope->find_variable_by_name(path[end_of_namespace]);
    while (!var && !found_scope->m_parent_scope.expired())
    {
      found_scope = found_scope->m_parent_scope.lock();
      var = found_scope->find_variable_by_name(path[end_of_namespace]);
    }
    return var;
  }

  std::shared_ptr<scope> scope_system::find_namespace(std::span<string_type const> const& path) const {
    if (path.empty())
      return current();

    auto cur = current();
    while (true)
    {
      //if (cur->m_name && cur->m_name.value() == path[0]) {
      auto check = cur;
      for (size_t i = 0; i < path.size(); ++i)
      {
        auto iter = check->m_child_scopes.find(path[i]);
        if (iter == check->m_child_scopes.end()) {
          auto const iter2 = check->m_child_scopes.find(path[i]);
          if (iter2 != check->m_child_scopes.end())
          {
            iter = iter2;
          }
          if (iter == check->m_child_scopes.end()) {
            check = nullptr;
            break;
          }
        }

        check = iter->second;
      }

      if (check)
        return check;
      //}
      if (cur->m_parent_scope.expired())
        break;
      cur = cur->m_parent_scope.lock();
    }
    return cur->m_name.value() == path.back() ? cur : nullptr;
  }

  type_entity scope_system::begin_structure(string_type name)
  {
    auto current_scope = current();
    auto type_scope = std::make_shared<scope>(current_scope, name);
    structure_type_entity entity{
      .scope = type_scope,
      .name = name
    };
    auto const base = std::make_shared<type_entity_impl>(type_entity_base{
      .entity = std::move(entity)
      });

    auto* e = base->base->as_structure();

    type_entity ty{ std::move(base) };
    current_scope->new_type(ty);

    m_current_scope->m_child_scopes["__class_" + name] = type_scope;
    return ty;
  }

  void scope_system::end_structure() {
    pop();
  }

  void scope_system::variable(var_entity var) {
    current()->new_variable(var);
  }

  void scope_system::pop() {
    if (!m_current_scope->m_parent_scope.expired())
      m_current_scope = m_current_scope->m_parent_scope.lock();
  }

  expr_entity scope_system::rotate_left(expr_entity expr)
  {
    auto child_left = expr.data->parameters.front();
    auto child_right_of_left = child_left.data->parameters.back();

    expr.data->parameters.front() = child_right_of_left;
    child_left.data->parameters.back() = expr;
    return child_left;
  }

  expr_entity scope_system::rotate_right(expr_entity expr)
  {
    auto child_right = expr.data->parameters.back();
    auto child_left_of_right = child_right.data->parameters.front();

    expr.data->parameters.back() = child_left_of_right;
    child_right.data->parameters.front() = expr;
    return child_right;
  }

  expr_entity scope_system::set_expr_parameters(expr_entity expr, std::span<expr_entity> parameters)
  {
    if (parameters.empty())
      return expr;

    for (auto const& p : parameters)
      expr.data->parameters.push_back(p);

    auto root = expr;
    while (true)
    {
      if (precedence_weaker(expr.data->parameters.front(), expr))
      {
        root = rotate_left(expr);
      }
      else if (precedence_stronger(expr, expr.data->parameters.back()))
      {
        root = rotate_right(expr);
      }
      else
      {
        break;
      }
    }

    return root;
  }
}