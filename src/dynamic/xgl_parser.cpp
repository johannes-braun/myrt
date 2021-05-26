#include "xgl_parser.hpp"
#include <iostream>
#include <format>

namespace myrt
{
  bool is_nswizzle(string_view_type text, size_t num_components)
  {
    return text.find_first_not_of(string_view_type("xrsygtzbuwav", 3 * num_components)) == std::string::npos;
  }

  xgl_parser::xgl_parser(xgl_tokenizer const& tokenizer)
  {
    m_types["void"] = { .name = "void", .type = xgl_builtin_type::void_type };
    m_types["int"] = { .name = "int", .type = xgl_builtin_type::int_type, .components = 1 };
    m_types["uint"] = { .name = "uint", .type = xgl_builtin_type::uint_type, .components = 1 };
    m_types["long"] = { .name = "long", .type = xgl_builtin_type::long_type, .components = 1 };
    m_types["ulong"] = { .name = "ulong", .type = xgl_builtin_type::ulong_type, .components = 1 };
    m_types["float"] = { .name = "float", .type = xgl_builtin_type::float_type, .components = 1 };
    m_types["double"] = { .name = "double", .type = xgl_builtin_type::double_type, .components = 1 };
    m_types["vec3"] = { .name = "vec3", .type = xgl_builtin_type::vec3_type, .components = 3 };
    m_types["unorm4x8"] = { .name = "unorm4x8", .type = xgl_builtin_type::unorm4x8_type, .components = 4 };

    m_end_token = tokenizer.tokens().end();
    for (m_current_token = tokenizer.tokens().begin(); m_current_token != m_end_token;)
    {
      if (expect_structure_definition("struct"))
      {
        if (expect_operator(";")) {
          continue;
        }

        /*     auto const instance_name = expect_identifier();
             if (instance_name)
             {

             }*/
        __debugbreak();
      }
      else if (expect_function_definition())
      {
        if (expect_operator(";")) { // forward declaration
          continue;
        }
        __debugbreak();
      }
      else
      {
        std::format_to(std::ostreambuf_iterator(std::cerr), "[{}:{}] Unknown identifier: \"{}\"\n",
          m_current_token->line, m_current_token->column_start, m_current_token->text);
        ++m_current_token;
      }
    }
  }
  std::optional<type_iterator> xgl_parser::expect_type()
  {
    auto const last_token = m_current_token;
    auto const identifier = expect_identifier();
    if (identifier)
    {
      if (auto const it = m_types.find(identifier.value()->text); it != m_types.end())
      {
        return it;
      }
    }
    m_current_token = last_token;
    return std::nullopt;
  }
  std::optional<type_iterator> xgl_parser::expect_complete_type()
  {
    auto const last_token = m_current_token;
    auto const type = expect_type();

    if (type && type.value()->second.defined)
      return type;

    m_current_token = last_token;
    return std::nullopt;
  }
  std::optional<token_iterator> xgl_parser::expect_identifier(std::optional<string_view_type> name)
  {
    if (m_current_token == m_end_token ||
      m_current_token->type != xgl_tokenizer::token_type::identifier ||
      (name.has_value() &&
        m_current_token->text != name.value()))
    {
      return std::nullopt;
    }

    token_iterator result = m_current_token;
    ++m_current_token;
    return result;
  }

  std::optional<token_iterator> xgl_parser::expect_operator(std::optional<string_view_type> name)
  {
    if (m_current_token == m_end_token ||
      m_current_token->type != xgl_tokenizer::token_type::op ||
      (name.has_value() &&
        m_current_token->text != name.value()))
    {
      return std::nullopt;
    }

    token_iterator result = m_current_token;
    ++m_current_token;
    return result;
  }

  std::optional<token_iterator> xgl_parser::expect_literal()
  {
    if (m_current_token == m_end_token || (
      m_current_token->type != xgl_tokenizer::token_type::double_literal &&
      m_current_token->type != xgl_tokenizer::token_type::int_literal &&
      m_current_token->type != xgl_tokenizer::token_type::uint_literal &&
      m_current_token->type != xgl_tokenizer::token_type::float_literal &&
      m_current_token->type != xgl_tokenizer::token_type::long_literal &&
      m_current_token->type != xgl_tokenizer::token_type::ulong_literal &&
      m_current_token->type != xgl_tokenizer::token_type::str_literal))
    {
      return std::nullopt;
    }

    token_iterator result = m_current_token;
    ++m_current_token;
    return result;
  }

  bool xgl_parser::expect_function_definition()
  {
    auto const last_token = m_current_token;
    auto const return_type = expect_type();
    if (return_type)
    {
      auto const name = expect_identifier();
      if (name) {
        auto const op = expect_operator("(");
        if (op)
        {
          xgl_function fun{
            .name = name.value()->text,
            .return_type = return_type.value(),
            .defined = false
          };

          while (!expect_operator(")"))
          {
            auto const type = expect_type();
            if (!type)
            {
              std::format_to(std::ostreambuf_iterator(std::cerr), "[{}:{}] Expected type in parameter list, but got \"{}\"\n",
                m_current_token->line, m_current_token->column_start, m_current_token->text);
              continue;
            }

            auto const param_name = expect_identifier();

            auto& param = fun.parameters.emplace_back();
            param.type = type.value();
            if (param_name)
              param.name = param_name.value()->text;

            if (!expect_operator(","))
            {
              if (!expect_operator(")")) {
                std::format_to(std::ostreambuf_iterator(std::cerr), "[{}:{}] Expected comma \",\" or closing bracket \")\" in parameter list, but got \"{}\"\n",
                  m_current_token->line, m_current_token->column_start, m_current_token->text);
              }
              break;
            }
          }

          fun.defined = !expect_operator(";");

          if (fun.defined) {
            if (auto const f = m_functions.find(fun.name); f != m_functions.end())
            {
              if (f->second.defined)
              {
                std::format_to(std::ostreambuf_iterator(std::cerr), "[{}:{}] Function already defined: \"{}\"\n",
                  m_current_token->line, m_current_token->column_start, f->second.name);
                m_current_token = last_token;
                return false;
              }
            }

            auto const body = expect_statement(true);

            if (!body)
            {
              std::format_to(std::ostreambuf_iterator(std::cerr), "[{}:{}] Failed to parse function body for definition of \"{}\".\n",
                m_current_token->line, m_current_token->column_start, fun.name);
              m_current_token = last_token;
              return false;
            }
            fun.statements = std::move(body);
          }

          m_functions[fun.name] = std::move(fun);
          return true;
        }
      }
    }
    m_current_token = last_token;
    return false;
  }
  bool xgl_parser::expect_structure_definition(string_view_type identifier)
  {
    auto const last_token = m_current_token;

    auto const struct_identifier = expect_identifier(identifier);
    if (struct_identifier)
    {
      auto const struct_name = expect_identifier();
      auto const semicolon = expect_operator(";"); // forward declaration
      if (struct_name && semicolon)
      {
        xgl_type type{
          .name = struct_name.value()->text,
          .type = xgl_builtin_type::struct_type,
          .defined = false
        };
        m_types[type.name] = std::move(type);
        return true;
      }

      auto const bracket = expect_operator("{");
      if (bracket)
      {
        xgl_type type;
        type.type = xgl_builtin_type::struct_type;
        if (struct_name)
          type.name = struct_name.value()->text;

        type.defined = true;

        while (true)
        {
          if (expect_operator("}"))
          {
            break;
          }

          auto const field_type = expect_complete_type();
          if (!field_type)
          {
            std::format_to(std::ostreambuf_iterator(std::cerr), "[{}:{}] Expected complete type in struct member declaration, but got \"{}\"\n",
              m_current_token->line, m_current_token->column_start, m_current_token->text);
            m_current_token = last_token;
            return false;
          }

          auto const field_name = expect_identifier();
          if (!field_name)
          {
            // unnamed member
            if (!expect_operator(";"))
            {
              std::format_to(std::ostreambuf_iterator(std::cerr), "[{}:{}] Unexpected token after unnamed struct field: \"{}\"\n",
                m_current_token->line, m_current_token->column_start, m_current_token->text);
              m_current_token = last_token;
              return false;
            }

            auto& field = type.fields.emplace_back();
            field.type = field_type.value();
            field.name = std::nullopt;
            continue;
          }

          auto& field = type.fields.emplace_back();
          field.type = field_type.value();
          field.name = field_name.value()->text;

          if (!expect_operator(";"))
          {
            std::format_to(std::ostreambuf_iterator(std::cerr), "[{}:{}] Unexpected token after struct field: \"{}\"\n",
              m_current_token->line, m_current_token->column_start, m_current_token->text);
            m_current_token = last_token;
            return false;
          }
        }

        if (auto const ty = m_types.find(type.name); ty != m_types.end())
        {
          if (ty->second.defined)
          {
            std::format_to(std::ostreambuf_iterator(std::cerr), "[{}:{}] Type already defined: \"{}\"\n",
              m_current_token->line, m_current_token->column_start, type.name);
            m_current_token = last_token;
            return false;
          }
        }

        m_types[type.name] = std::move(type);
        return true;
      }
    }

    m_current_token = last_token;
    return false;
  }

  std::optional<xgl_statement> xgl_parser::expect_statement(bool require_block)
  {
    auto const last_token = m_current_token;
    auto const block_open = expect_operator("{");

    if (require_block && !block_open)
    {
      std::format_to(std::ostreambuf_iterator(std::cerr), "[{}:{}] Expected statement block, but got \"{}\"\n",
        m_current_token->line, m_current_token->column_start, m_current_token->text);
      m_current_token = last_token;
      return std::nullopt;
    }

    if (block_open && expect_operator("}"))
    {
      xgl_statement block_statement;
      block_statement.type = xgl_statement_type::block;
      block_statement.value = std::vector<xgl_statement>();
      return block_statement;
    }

    if (block_open)
    {
      xgl_statement block_statement;
      block_statement.type = xgl_statement_type::block;
      block_statement.value = std::vector<xgl_statement>();

      while (true)
      {
        auto const statement = expect_statement();

        if (statement)
        {
          std::get<std::vector<xgl_statement>>(block_statement.value).push_back(std::move(statement.value()));
        }
        else if (!statement && expect_operator("}"))
        {
          return block_statement;
        }
        else
        {
          m_current_token = last_token;
          return std::nullopt;
        }
      }
    }

    auto const type = expect_type();
    if (!type)
    {
      auto const expr = expect_expression();
      if (expr)
      {
        auto const semicolon = expect_operator(";");
        if (semicolon)
        {
          xgl_statement e;
          e.value = expr.value();
          e.type = xgl_statement_type::expression;
          return e;
        }
      }

      m_current_token = last_token;
      return std::nullopt;
    }

    auto const var_name = expect_identifier();
    if (!var_name)
    {
      m_current_token = last_token;
      return std::nullopt;
    }

    xgl_statement::declaration decl;
    decl.variable.name = var_name.value()->text;
    decl.variable.type = type.value();

    auto const equals = expect_operator("=");
    if (equals)
    {
      // handle assignment

      auto const expr = expect_expression();
      if (!expr)
      {
        std::format_to(std::ostreambuf_iterator(std::cerr), "[{}:{}] Expected expression after assignment operator, but got \"{}\"\n",
          m_current_token->line, m_current_token->column_start, m_current_token->text);
        m_current_token = last_token;
        return std::nullopt;
      }

      decl.value = std::move(expr);
    }

    auto const semicolon = expect_operator(";");
    if (!semicolon)
    {
      std::format_to(std::ostreambuf_iterator(std::cerr), "[{}:{}] Expected \";\" at the end of a statement, but got \"{}\"\n",
        m_current_token->line, m_current_token->column_start, m_current_token->text);
      m_current_token = last_token;
      return std::nullopt;
    }

    xgl_statement statement{
      .type = xgl_statement_type::declaration,
      .value = decl
    };

    //m_current_token = last_token;
    return statement;
  }
  std::optional<xgl_expression> xgl_parser::expect_expression()
  {
    auto const last_token = m_current_token;

    auto expr_base = expect_single_expression();

    if (!expr_base)
    {
      auto const op = expect_nplace_operator();

      if (op)
      {
        auto const rhs = expect_expression();
        if (!rhs)
        {
          std::format_to(std::ostreambuf_iterator(std::cerr), "[{}:{}] Expected expression after operator, but got \"{}\"\n",
            m_current_token->line, m_current_token->column_start, m_current_token->text);
          m_current_token = last_token;
          return std::nullopt;
        }

        xgl_expression::op_call op_call{ .operator_components = op.value() };
        op_call.parameters.push_back(std::move(rhs.value()));

        xgl_expression func{
          .type = xgl_expression_type::op,
          .value = op_call
        };
        return func;
      }

      m_current_token = last_token;
      return std::nullopt;
    }

    while (expect_operator("."))
    {
      auto const ident = expect_identifier();
      if (!ident)
      {
        std::format_to(std::ostreambuf_iterator(std::cerr), "[{}:{}] Expected field identifier after \".\", but got \"{}\"\n",
          m_current_token->line, m_current_token->column_start, m_current_token->text);
        m_current_token = last_token;
        return std::nullopt;
      }

      xgl_expression::field_access access{
        .source = std::make_shared<xgl_expression>(std::move(expr_base.value())),
        .field = ident.value(),
        .swizzle = is_nswizzle(ident.value()->text, 4)// todo...
      };
      expr_base = xgl_expression{
        .type = xgl_expression_type::access_field,
        .value = std::move(access)
      };
    }

    auto const op = expect_nplace_operator();
    if (op)
    {
      auto const rhs = expect_expression();

      xgl_expression::op_call op_call{ .operator_components = op.value() };

      op_call.parameters.push_back(std::move(expr_base.value()));
      if (rhs)
        op_call.parameters.push_back(std::move(rhs.value()));

      expr_base = xgl_expression{
        .type = xgl_expression_type::op,
        .value = op_call
      };
    }

    return expr_base;
  }

  std::optional<xgl_expression> xgl_parser::expect_single_expression()
  {
    auto const last_token = m_current_token;
    auto const literal = expect_literal();
    if (literal)
    {
      xgl_expression result{
        .type = xgl_expression_type::literal,
        .value = literal.value()
      };

      return result;
    }

    auto const fun = expect_function_call();

    if (fun)
    {
      xgl_expression::function_call call;
      xgl_expression result{
        .type = xgl_expression_type::function_call
      };
      call.function = fun.value();

      auto op = expect_operator(")");
      while (true)
      {
        if (op)
        {
          result.value = call;
          return result;
        }

        auto const xpr = expect_expression();
        if (!xpr)
        {
          std::format_to(std::ostreambuf_iterator(std::cerr), "[{}:{}] Expected expression in function call, but got \"{}\"\n",
            m_current_token->line, m_current_token->column_start, m_current_token->text);
          m_current_token = last_token;
          return std::nullopt;
        }
        call.parameters.push_back(std::move(xpr.value()));

        auto const comma = expect_operator(",");
        op = expect_operator(")");
        if (!comma && !op)
        {
          std::format_to(std::ostreambuf_iterator(std::cerr), "[{}:{}] Expected \",\" or \")\" in function call, but got \"{}\"\n",
            m_current_token->line, m_current_token->column_start, m_current_token->text);
          m_current_token = last_token;
          return std::nullopt;
        }
      }
    }

    auto const variable = expect_identifier();
    if (variable)
    {
      xgl_expression result{
        .type = xgl_expression_type::variable,
        .value = variable.value()
      };
      return result;
    }

    m_current_token = last_token;
    return std::nullopt;
  }

  std::optional<function_iterator> xgl_parser::expect_function_call()
  {
    auto const last_token = m_current_token;
    auto const it = m_functions.find(m_current_token->text);

    if (it == m_functions.end())
    {
      m_current_token = last_token;
      return std::nullopt;
    }

    m_current_token++;
    auto const open_bracket = expect_operator("(");
    if (!open_bracket)
    {
      m_current_token = last_token;
      return std::nullopt;
    }

    return it;
  }

  constexpr uint64_t packed_op(char const* buf) {
    uint64_t hash = 0;
    while (*buf)
    {
      auto ch = buf++;
      hash += 0xcbf29ce484222325 ^ uint64_t(*ch) * 0x100000001b3;
    }
    return hash;
  }

  std::optional<std::vector<token_iterator>> xgl_parser::expect_nplace_operator()
  {
    auto const last_token = m_current_token;
    auto const op1 = expect_operator();
    if (!op1 || op1.value()->text.find_first_not_of("+-*/%<>=!^&|") != string_view_type::npos)
    {
      m_current_token = last_token;
      return std::nullopt;
    }

    auto const op2 = expect_operator();
    if (!op2)
      return std::vector<token_iterator>{ op1.value() };

    auto const op3 = expect_operator();

    std::string combined;
    combined.reserve(3);
    combined.append(op1.value()->text);
    combined.append(op2.value()->text);
    if (op3)
      combined.append(op3.value()->text);

    switch (packed_op(combined.c_str()))
    {
    case packed_op("++"):
    case packed_op("--"):
    case packed_op("+="):
    case packed_op("-="):
    case packed_op("*="):
    case packed_op("/="):
    case packed_op("%="):
    case packed_op("&="):
    case packed_op("|="):
    case packed_op("^="):
    case packed_op(">="):
    case packed_op("<="):
    case packed_op("!="):
    case packed_op("=="):
      return std::vector<token_iterator>{ op1.value(), op2.value() };

    case packed_op("<<="):
    case packed_op(">>="):
      return std::vector<token_iterator>{ op1.value(), op2.value(), op3.value() };
    }

    m_current_token = last_token;
    return std::nullopt;
  }
}