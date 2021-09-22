#pragma once
#include "xgl_parser.hpp"
#include <memory>

namespace myrt::xgl
{
  class evaluator
  {
  public:
    void load(std::shared_ptr<scope_system> scope);

  private:
    void load(std::shared_ptr<scope> scope);

    void reduce_expression(expr_entity const& expr);
    std::optional<value_entity> evaluate(expr_entity const& expr);

    std::shared_ptr<scope_system> m_scope_system;
  };
}