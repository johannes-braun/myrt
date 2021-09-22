#pragma once

#include "xgl_tokenizer.hpp"
#include <optional>
#include <memory>
#include <unordered_map>
#include <stack>
#include <variant>
#include <functional>
#include <experimental/generator>

namespace myrt::xgl 
{
  struct grammar_node {
    string_view_type identifier;
    bool optional = false;
    bool show_in_ast = true;
    std::vector<std::vector<grammar_node>> parts;
  };

  struct resolved_node {
    using token_or_node = std::variant<token_iterator, resolved_node>;

    string_view_type identifier;
    bool was_optional = false;
    std::vector<token_or_node> children;

    const resolved_node& child_node(size_t n) const;
    const std::optional<resolved_node> child_node(string_view_type n) const;
    std::experimental::generator<resolved_node const*> child_nodes(string_view_type n) const;
    const token_iterator& child_token(size_t n) const;
    const token_or_node& child(size_t n) const;

    std::optional<token_iterator> child_node_token(string_view_type n) const;
    std::optional<token_iterator> child_node_token(size_t n) const;
  };

  using resolver = std::function<std::optional<resolved_node>(token_iterator& current, token_iterator end)>;

  struct expr_props
  {
    string_view_type actual_identifier;

    bool is_optional : 1 = false;
    bool show_in_ast : 1 = true;
  };

  constexpr expr_props determine_props(string_view_type str)
  {
    if (str.length() < 2)
      return expr_props{ .actual_identifier = str };

    expr_props props;
    constexpr string_view_type special_chars("?#");
    for (auto it = str.rbegin(); it != str.rend(); ++it)
    {
      if (it != std::prev(str.rend()) && *it == '?')
      {
        props.is_optional = true;
      }
      else if (it != std::prev(str.rend()) && *it == '#')
      {
        props.show_in_ast = false;
      }
      else
      {
        auto const len = std::distance(it, str.rend());
        props.actual_identifier = str.substr(0, len);
        it = std::prev(str.rend());
        break;
      }
    }
    return props;
  }

  constexpr std::vector<grammar_node> operator""_expr(char_type const* str, size_t len)
  {
    if (len == 0)
    {
      return { };
    }

    std::vector<grammar_node> nodes;
    string_view_type base(str, len);
    while (!base.empty())
    {
      auto const off = base.find_first_of(' ');
      if (off == string_view_type::npos)
      {
        auto const props = determine_props(base);

        nodes.push_back(grammar_node{ 
          .identifier = props.actual_identifier,
          .optional = props.is_optional,
          .show_in_ast = props.show_in_ast
          });
        break;
      }
      else
      {
        auto const props = determine_props(base.substr(0, off));

        base = base.substr(off + 1);

        nodes.push_back(grammar_node{
          .identifier = props.actual_identifier,
          .optional = props.is_optional,
          .show_in_ast = props.show_in_ast
          });
      }
    }
    return nodes;
  }

  inline std::vector<std::vector<grammar_node>> operator "" _any_char(char_type const* str, size_t s) {
    std::vector<std::vector<grammar_node>> result;
    for (size_t i = 0; i < s; ++i)
    {
      string_view_type id(str + i, 1);
      result.push_back({ grammar_node{.identifier = id } });
    }
    return result;
  }

  template<typename T>
  concept grammar_node_vector = std::is_same_v<std::decay_t<T>, std::vector<grammar_node>>;
  template<typename T>
  concept grammar_node_vector_vector = std::is_same_v<std::decay_t<T>, std::vector<std::vector<grammar_node>>>;

  template<grammar_node_vector Lhs, grammar_node_vector Rhs>
  constexpr std::vector<std::vector<grammar_node>> operator|(Lhs&& lhs, Rhs&& rhs)
  {
    return std::vector<std::vector<grammar_node>>{ std::forward<Lhs>(lhs), std::forward<Rhs>(rhs) };
  }

  template<grammar_node_vector_vector Lhs, grammar_node_vector Rhs>
  constexpr std::vector<std::vector<grammar_node>> operator|(Lhs lhs, Rhs&& rhs)
  {
    lhs.push_back(std::forward<Rhs>(rhs));
    return lhs;
  }

  template<grammar_node_vector Lhs, grammar_node_vector_vector Rhs>
  constexpr std::vector<std::vector<grammar_node>> operator|(Lhs&& lhs, Rhs rhs)
  {
    rhs.insert(rhs.begin(), std::forward<Rhs>(lhs));
    return lhs;
  }

  template<grammar_node_vector_vector Lhs, grammar_node_vector_vector Rhs>
  constexpr std::vector<std::vector<grammar_node>> operator|(Lhs&& lhs, Rhs rhs)
  {
    lhs.insert(lhs.end(), std::make_move_iterator(rhs.begin()), std::make_move_iterator(rhs.end()));
    return lhs;
  }

  template<grammar_node_vector_vector Rhs>
  constexpr grammar_node operator&&(string_view_type lhs, Rhs&& rhs)
  {
    grammar_node node;
    node.identifier = lhs;
    node.optional = false;
    node.parts = std::forward<Rhs>(rhs);
    return node;
  }

  template<grammar_node_vector Rhs>
  constexpr grammar_node operator&&(string_view_type lhs, Rhs&& rhs)
  {
    grammar_node node;
    node.identifier = lhs;
    node.optional = false;
    node.parts = { std::forward<Rhs>(rhs) };
    return node;
  }

  inline std::pair<string_view_type, resolver> operator&&(string_view_type lhs, resolver&& rhs)
  {
    return std::pair(lhs, rhs);
  }

  constexpr struct flatten_node_t* flatten = nullptr;

  constexpr std::pair<bool, grammar_node> operator||(struct flatten_node_t*, grammar_node&& node)
  {
    return std::pair(true, std::move(node));
  }

  class grammar {
  public:
    grammar& operator+=(grammar_node&& node);
    grammar& operator+=(std::pair<bool, grammar_node>&& node);
    grammar& operator+=(std::pair<string_view_type, resolver>&& node);

    void set_root(string_view_type root);

    std::vector<resolved_node> resolve(token_iterator begin, token_iterator end);

  private:
    std::optional<resolved_node> resolve_impl(grammar_node const& node, token_iterator& current, token_iterator begin, token_iterator end, token_iterator& farthest);
    void try_increment(token_iterator begin, token_iterator current, token_iterator& farthest);

    void increment_skip_comments(token_iterator& current, token_iterator end);

    using node_or_resolver = std::variant<resolver, grammar_node>;

    struct stack_trace_entry
    {
      grammar_node const* node;
      std::vector<grammar_node> const* towards_section = nullptr;
      grammar_node const* towards = nullptr;
      token_iterator token;
      bool complete = false;
    };

    std::stack<stack_trace_entry> m_resolve_stack;
    std::stack<stack_trace_entry> m_best_resolved_stack;
    
    std::optional<string_view_type> m_root;
    std::unordered_set<string_view_type> m_flatten_node;
    std::unordered_map<string_view_type, node_or_resolver> m_grammar;
  };
}