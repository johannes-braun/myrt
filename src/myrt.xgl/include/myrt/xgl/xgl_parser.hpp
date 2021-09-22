#pragma once
#include "xgl_tokenizer.hpp"
#include "xgl_entities.hpp"
#include "xgl_grammar.hpp"
#include <optional>
#include <source_location>
#include <stdexcept>
#include <format>
#include <iostream>
#include <span>

namespace myrt::xgl
{
  enum class decorator
  {
    const_dec,
    in_dec,
    out_dec,
    inout_dec
  };

  std::optional<decorator> get_decorator(string_view_type str);

  struct scope_system;
  struct scope;

  class critical_parser_error : std::runtime_error {
    using std::runtime_error::runtime_error;
  };

  class parser
  {
  public:
    parser();

    std::shared_ptr<scope_system> parse(tokenizer const& tokenizer);

  private:
    using parse_node_begin = std::function<bool(resolved_node const&)>;
    using parse_node_end = std::function<void(resolved_node const&)>;

    struct parse_callback
    {
      std::optional<parse_node_begin> on_begin;
      std::optional<parse_node_end> on_end;
    };

    template<typename ... FmtArgs>
    void critical_error(std::string_view msg, FmtArgs&&... args, std::source_location location = std::source_location::current()) {
      std::string fmt_msg = std::format(msg, std::forward<FmtArgs>(args)...);
      std::format_to(std::ostreambuf_iterator(std::cerr), "critical error [{} at {}:{}]: ", location.function_name(), location.line(), location.column());
      std::cerr << fmt_msg << '\n';
      throw critical_parser_error(std::move(fmt_msg));
    }

    template<typename ... FmtArgs>
    void error(std::string_view msg, FmtArgs&&... args, std::source_location location = std::source_location::current()) {
      std::format_to(std::ostreambuf_iterator(std::cerr), "error [{} at {}:{}]: ", location.function_name(), location.line(), location.column());
      std::format_to(std::ostreambuf_iterator(std::cerr), msg, std::forward<FmtArgs>(args)...);
    }

    template<typename ... FmtArgs>
    void warning(std::string_view msg, FmtArgs&&... args, std::source_location location = std::source_location::current()) {
      std::format_to(std::ostreambuf_iterator(std::cout), "warning [{} at {}:{}]: ", location.function_name(), location.line(), location.column());
      std::format_to(std::ostreambuf_iterator(std::cout), msg, std::forward<FmtArgs>(args)...);
    }

    void build_predefined_functions();
    void build_grammar();
    void parse_node(resolved_node const& node);

    bool parse_function(resolved_node const& node);

    std::optional<resolved_node> expect_identifier(token_iterator& current, token_iterator end);
    std::optional<resolved_node> expect_literal(std::optional<tokenizer::token_type> type, token_iterator& current, token_iterator end);
    std::optional<resolved_node> expect_literal(token_iterator& current, token_iterator end);
    std::optional<resolved_node> expect_operator(token_iterator& current, token_iterator end);

    string_type get_full_arithmetic_operator(resolved_node const& arithmetic_operator_node);

    std::optional<type_entity> resolve_type(resolved_node const& basic_type);
    expr_entity make_expression_(resolved_node const& node);
    expr_entity resolve_expression(resolved_node const& node);
    expr_entity resolve_expression_(resolved_node const& node);

    enum class type_compare
    {
      equal,
      implicitly_convertible,
      explicitly_convertible,
      different
    };

    grammar m_grammar;
    std::unordered_map<string_type, int> m_precedence;
    std::unordered_map<int, associativity> m_associativity;
    std::unordered_map<string_view_type, parse_callback> m_parsers;

    std::shared_ptr<scope_system> m_scope_system;
  };
}
