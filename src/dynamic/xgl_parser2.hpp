#pragma once

#include "xgl_tokenizer.hpp"
#include <vector>
#include <variant>
#include <memory>
#include <optional>
#include <functional>
#include <stack>
#include <unordered_map>
#include <unordered_set>

namespace myrt::xgl
{
  class type;

  using token_iterator = std::vector<xgl_tokenizer::token>::const_iterator;
  using type_iterator = std::unordered_map<string_view_type, type>::const_iterator;

  enum class builtin_type
  {
    void_type,
    int_type,
    bool_type,
    uint_type,
    long_type,
    ulong_type,
    float_type,
    double_type,
    vec2_type,
    vec3_type,
    unorm4x8_type,
    struct_type,
    input_type
  };

  class variable
  {
  public:
    std::optional<token_iterator> name = std::nullopt;
    type_iterator type;
  };

  class type
  {
  public:
    string_view_type name;
    builtin_type type;
    std::vector<variable> fields;
    bool defined = true; // false if only forward declared
  };

  struct grammar_node {
    std::optional<string_view_type> type_name;
    std::optional<string_view_type> required_text;
    std::vector<std::vector<std::shared_ptr<grammar_node>>> parts;
    bool optional = false;
    std::optional<string_view_type> flatten_by;
  };

  struct node
  {
    string_view_type type_name;
    bool was_optional = false;
    using token_or_node = std::variant<token_iterator, std::shared_ptr<node>>;
    std::vector<token_or_node> children;
  };

  using node_or_resolver = std::variant<std::function<std::optional<node>()>, std::shared_ptr<grammar_node>>;

  class parser {
  public:
    parser(xgl_tokenizer const& tokenizer);

  private:
    std::optional<node> expect_grammar(grammar_node const& gnode);

    std::optional<node> expect_type();
    std::optional<node> expect_identifier();
    std::optional<node> expect_literal();
    std::optional<node> expect_operator();
    std::optional<node> expect_arithmetic_operator();
    std::optional<node> expect_text(string_view_type text, std::optional<node> node);
    std::optional<node> expect_variable();
    std::optional<node> expect_function_name();

    void push_scope();
    bool pop_scope();

    void begin_expect();
    std::nullopt_t cancel_expect();
    void pop_expect();

    std::stack<token_iterator> m_previous_tokens;
    token_iterator m_current_token;
    token_iterator m_end_token;

    std::unordered_map<string_view_type, type> m_defined_types;
    std::unordered_map<string_view_type, node_or_resolver> m_grammar;

    std::vector<std::list<variable>> m_var_stack;
    std::unordered_multimap<string_view_type, variable> m_current_vars;

    std::unordered_map<string_view_type, int> m_functions;

    std::unordered_map<string_view_type, std::function<bool(node const& node)>> m_node_success_post;
    std::unordered_map<string_view_type, std::function<bool(node const& node)>> m_node_success_direct;
    std::vector<std::vector<std::function<void()>>> m_on_cancel;
  };
}