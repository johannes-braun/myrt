#include "xgl_evaluator.hpp"
#include "xgl_scope_system.hpp"

namespace myrt::xgl
{
  template<typename T>
  value_entity make_value(string_view_type token)
  {
    T val;
    std::from_chars(token.data(), token.data() + token.size(), val);
    return make_value(&val, sizeof(T));
  }
  template<>
  value_entity make_value<bool>(string_view_type token)
  {
    bool val = token == "false" ? false : true;
    return make_value(&val, sizeof(val));
  }

  void evaluator::load(std::shared_ptr<scope_system> root)
  {
    m_scope_system = root;

    for (auto& var : root->root()->variables())
    {
      if (var.data->value)
        reduce_expression(*var.data->value);
    }

    //for (auto const& expr : root->all_expressions())
    //  reduce_expression(expr);
    //__debugbreak();
  }
  void evaluator::load(std::shared_ptr<scope> root)
  {
    /*for (auto const& child : root->child_scopes())
      load(child.second);
    for (auto const& child : root->unnamed_scopes())
      load(child);*/

    __debugbreak();
  }
  void evaluator::reduce_expression(expr_entity const& expr)
  {
    if (expr.data->evaluated_value)
      return;

    for (auto& par : expr.data->parameters)
      reduce_expression(par);
    expr.data->evaluated_value = evaluate(expr);
  }
  std::optional<value_entity> evaluator::evaluate(expr_entity const& expr)
  {
    switch (expr.data->type)
    {
    case expr_entity_impl::type::is_literal:
    {
      auto& x = std::get<expr_entity_literal>(expr.data->data);
      switch (dealias(x.type.data)->base->as_builtin()->base)
      {
      case builtin_type_entity::basic_kind::int32_base:
        return make_value<int32_t>(x.value);
      case builtin_type_entity::basic_kind::uint32_base:
        return make_value<uint32_t>(x.value);
      case builtin_type_entity::basic_kind::float_base:
        return make_value<float>(x.value);
      case builtin_type_entity::basic_kind::double_base:
        return make_value<double>(x.value);
      case builtin_type_entity::basic_kind::bool_base:
        return make_value<bool>(x.value);
      }
      break;
    }
    case expr_entity_impl::type::is_member:
      __debugbreak();
      break;

    case expr_entity_impl::type::is_variable:
      auto& var_expr = std::get<expr_entity_variable>(expr.data->data);
      break;

    //case expr_entity_impl::type::is_operator:
      //auto& fn = std::get<expr_entity_function>(expr.data->data);
    }


    return std::nullopt;
  }
}