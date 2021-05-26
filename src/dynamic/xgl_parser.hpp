#pragma once

#include "xgl_tokenizer.hpp"
#include <optional>
#include <variant>
#include <string>
#include <unordered_map>
#include <memory>

namespace myrt
{
  enum class xgl_builtin_type
  {
    void_type,
    int_type,
    uint_type,
    long_type,
    ulong_type,
    float_type,
    double_type,
    vec3_type,
    unorm4x8_type,
    struct_type,
    input_type
  };

  class xgl_function;
  class xgl_parameter;
  class xgl_type;
  class xgl_statement;

  using token_iterator = std::vector<xgl_tokenizer::token>::const_iterator;
  using type_iterator = std::unordered_map<string_view_type, xgl_type>::const_iterator;
  using function_iterator = std::unordered_map<string_view_type, xgl_function>::const_iterator;

  class xgl_parameter
  {
  public:
    std::optional<string_view_type> name = std::nullopt;
    type_iterator type;
  };

  class xgl_type
  {
  public:
    struct field : xgl_parameter { };

    string_view_type name;
    xgl_builtin_type type;
    size_t components = 0;
    std::vector<field> fields;
    bool defined = true; // false if only forward declared
  };

  enum class xgl_expression_type {
    op,
    function_call,
    variable,
    access_field,
    literal
  };

  class xgl_expression
  {
  public:
    xgl_expression_type type;

    struct function_call
    {
      function_iterator function;
      std::vector<xgl_expression> parameters;
    };

    struct op_call
    {
      std::vector<token_iterator> operator_components;
      std::vector<xgl_expression> parameters;
    };

    struct field_access
    {
      std::shared_ptr<xgl_expression> source;
      token_iterator field;
      bool swizzle = false;
    };

    std::variant<function_call, token_iterator, field_access, op_call> value;
  };

  enum class xgl_statement_type {
    declaration,
    expression,
    block
  };

  class xgl_statement
  {
  public:
    xgl_statement_type type;
    struct declaration {
      xgl_parameter variable;
      std::optional<xgl_expression> value;
    };
    
    std::variant<declaration, std::vector<xgl_statement>, xgl_expression> value;
  };

  class xgl_function
  {
  public:
    string_view_type name;
    type_iterator return_type;
    std::vector<xgl_parameter> parameters;
    bool defined = true; // false if only forward declared
    std::optional<xgl_statement> statements;
  };

  class xgl_parser
  {
  public:
    xgl_parser(xgl_tokenizer const& tokenizer);

  private:
    std::optional<type_iterator> expect_type();
    std::optional<type_iterator> expect_complete_type();
    std::optional<token_iterator> expect_identifier(std::optional<string_view_type> name = std::nullopt);
    std::optional<token_iterator> expect_operator(std::optional<string_view_type> name = std::nullopt);
    std::optional<std::vector<token_iterator>> expect_nplace_operator();
    std::optional<token_iterator> expect_literal();

    bool expect_function_definition();
    bool expect_structure_definition(string_view_type identifier);
    std::optional<xgl_statement> expect_statement(bool require_block = false);
    std::optional<xgl_expression> expect_single_expression();
    std::optional<xgl_expression> expect_expression();
    std::optional<function_iterator> expect_function_call();

    std::unordered_map<string_view_type, xgl_type> m_types;
    std::unordered_map<string_view_type, xgl_function> m_functions;
    token_iterator m_current_token;
    token_iterator m_end_token;
  };
}