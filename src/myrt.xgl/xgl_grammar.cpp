#include "xgl_grammar.hpp"
#include <format>
#include <iostream>
#include <stack>
#include <sstream>

namespace myrt::xgl
{
  resolved_node const& resolved_node::child_node(size_t n) const {
    return std::get<resolved_node>(children[n]);
  }
  const std::optional<resolved_node> resolved_node::child_node(string_view_type n) const {
    auto const it = std::find_if(children.begin(), children.end(), [&](token_or_node const& nd) {
      return nd.index() == 1 && std::get<resolved_node>(nd).identifier == n;
      });
    if (it == children.end())
      return std::nullopt;
    return std::get<resolved_node>(*it);
  }
  std::experimental::generator<resolved_node const*> resolved_node::child_nodes(string_view_type n) const
  {
    for (auto const& child : children)
    {
      if (child.index() == 1)
      {
        auto const& rnode = std::get<resolved_node>(child);
        if (rnode.identifier == n)
          co_yield &rnode;
      }
    }
  }
  token_iterator const& resolved_node::child_token(size_t n) const {
    return std::get<token_iterator>(children[n]);
  }
  resolved_node::token_or_node const& resolved_node::child(size_t n) const {
    return children[n];
  }
  std::optional<token_iterator> resolved_node::child_node_token(string_view_type n) const
  {
    auto const node = child_node(n);
    if (!node || node->children[0].index() != 0)
      return std::nullopt;
    return std::get<0>(node->children[0]);
  }
  std::optional<token_iterator> resolved_node::child_node_token(size_t n) const
  {
    auto const& node = child_node(n);
    if (node.children[0].index() != 0)
      return std::nullopt;
    return std::get<0>(node.children[0]);
  }
  grammar& grammar::operator+=(grammar_node&& node)
  {
    m_grammar[node.identifier] = std::move(node);
    return *this;
  }
  grammar& grammar::operator+=(std::pair<bool, grammar_node>&& node)
  {
    m_flatten_node.emplace(node.second.identifier);
    m_grammar[node.second.identifier] = std::move(node.second);
    return *this;
  }

  grammar& grammar::operator+=(std::pair<string_view_type, resolver>&& node)
  {
    m_flatten_node.emplace(node.first);
    m_grammar[node.first] = std::move(node.second);
    return *this;
  }

  void grammar::set_root(string_view_type root)
  {
    m_root = root;
  }

  std::vector<resolved_node> grammar::resolve(token_iterator begin, token_iterator end)
  {
    if (!m_root)
      return {};

    auto const root_node = std::get<grammar_node>(m_grammar[*m_root]);

    std::vector<resolved_node> result;
    token_iterator farthest = begin;
    token_iterator current = begin;
    while (current != end)
    {
      auto node = resolve_impl(root_node, current, begin, end, farthest);

      if (node)
      {
        result.push_back(*node);
      }
      else
      {
        std::format_to(std::ostreambuf_iterator(std::cout),
          "\033[1;91mGRAMMAR ERROR:\033[0m Failed to resolve token \"{}\" at [{}:{}].\n"
          "\033[1;91mSTACKTRACE:\033[0m\n",
          farthest->text,
          farthest->line + 1,
          farthest->column_start - 1
        );

        while (!m_best_resolved_stack.empty())
        {
          auto const top = std::move(m_best_resolved_stack.top());
          m_best_resolved_stack.pop();

          if (top.node && top.towards)
          {
            ptrdiff_t offset = std::distance(top.node->parts.data(), top.towards_section);
            ptrdiff_t part_offset = std::distance(top.node->parts[offset].data(), top.towards);

            std::stringstream expr;
            for (auto const& part : top.node->parts)
            {
              if (&part == top.towards_section)
                expr << "    \033[1;34m>>\033[21;39m  ";
              else
                expr << "        ";

              for (auto const& p : part)
              {
                if (&p == top.towards)
                {
                  std::format_to(std::ostreambuf_iterator(expr),
                    "\033[1;34m{}\033[21;39m{}",
                    p.identifier,
                    p.optional ? "\033[1;36m<opt>\033[21;39m" : ""
                  );
                }
                else
                {
                  expr << p.identifier;
                  if (p.optional)
                    expr << "\033[1;33m<opt>\033[21;39m";
                }

                if (&p != &top.node->parts[offset].back())
                  expr << " ";
              }
              if (&part != &top.node->parts.back())
                expr << "\n";
            }

            std::format_to(std::ostreambuf_iterator(std::cout),
              "  \033[1;30mBEG EXPECT\033[0m {} \033[1;30m\n    EXPRSET\n\033[0m{}\033[1;30m\n    \033[1;30mENDEXPR\033[0m\033[1;30m\n    REQUIRE\033[0m [{}] \"\033[1;34m{}\033[0m\" \033[1;30m\n    TOKEN  \033[0m \033[1;32m{}\033[1;30m [{}:{}]\n\n",
              m_best_resolved_stack.empty() ? *m_root : m_best_resolved_stack.top().towards->identifier,
              expr.str(),
              part_offset,
              top.towards->identifier,
              top.token->text,
              top.token->line + 1,
              top.token->column_start - 1
            );
          }
          else
          {
            std::format_to(std::ostreambuf_iterator(std::cout),
              "  \033[1;30mBEG EXPECT\033[0m {} \033[1;30m\n    TOKEN  \033[0m \033[1;32m{}\033[1;30m [{}:{}]\n\n",
              m_best_resolved_stack.empty() ? *m_root : m_best_resolved_stack.top().towards->identifier,
              top.token->text,
              top.token->line + 1,
              top.token->column_start - 1
            );
          }
        }

        break;
      }
    }
    return result;
  }

  std::optional<resolved_node> grammar::resolve_impl(grammar_node const& node, token_iterator& current, token_iterator begin, token_iterator end, token_iterator& farthest)
  {
    class expect {
    public:
      expect(grammar& grammar, token_iterator& current, token_iterator begin, token_iterator end, token_iterator& farthest) :
        m_self(&grammar), m_current_token(&current), m_farthest_token(&farthest), m_begin_token(begin), m_end_token(end) {}

      std::optional<resolved_node> operator()(std::function<std::optional<resolved_node>(token_iterator& current, token_iterator end)> const& fun) const {
        m_self->m_resolve_stack.push(stack_trace_entry{ nullptr, nullptr, nullptr, *m_current_token, false });
        auto const result = fun(*m_current_token, m_end_token);
        m_self->m_resolve_stack.top().complete = result != std::nullopt;
        m_self->try_increment(m_begin_token, *m_current_token, *m_farthest_token);
        m_self->m_resolve_stack.pop();
        return result;
      }

      std::optional<resolved_node> operator()(grammar_node const& node) const {
        m_self->m_resolve_stack.push(stack_trace_entry{ &node, nullptr, nullptr, *m_current_token, false });
        auto const result = m_self->resolve_impl(node, *m_current_token, m_begin_token, m_end_token, *m_farthest_token);
        m_self->m_resolve_stack.top().complete = result != std::nullopt;
        m_self->try_increment(m_begin_token, *m_current_token, *m_farthest_token);

        m_self->m_resolve_stack.pop();
        return result;
      }

    private:
      grammar* m_self;
      token_iterator* m_current_token;
      token_iterator* m_farthest_token;
      token_iterator m_begin_token;
      token_iterator m_end_token;
    };

    token_iterator current_token = current;

    resolved_node resolved;
    resolved.identifier = node.identifier;
    for (auto& sub_parts : node.parts)
    {
      if (sub_parts.empty())
      {
        return resolved;
      }

      bool success = true;
      for (auto& part : sub_parts)
      {
        auto name = part.identifier;
        std::optional<string_view_type> raw_text;
        std::optional<resolved_node> child;

        if (!m_resolve_stack.empty())
        {
          m_resolve_stack.top().towards_section = &sub_parts;
          m_resolve_stack.top().towards = &part;
        }

        auto const grammar_iterator = m_grammar.find(name);
        if (grammar_iterator == m_grammar.end())
        {
          raw_text = name;
        }

        if (!raw_text)
        {
          child = std::visit(expect(*this, current_token, begin, end, farthest), grammar_iterator->second);
        }
        else
        {
          child = std::visit(expect(*this, current_token, begin, end, farthest), node_or_resolver{
            [&](token_iterator& current, token_iterator end)->std::optional<resolved_node> {
            if (current != end && current->text == *raw_text)
            {
              resolved_node node
              {
                .identifier = name,
                .was_optional = part.optional,
                .children = { current }
              };
              increment_skip_comments(current, end);
              return node;
            }
            return std::nullopt;
            } });
        }

        if (!child && !part.optional)
        {
          success = false;
          break;
        }
        else if (!child && part.optional)
        {
        }
        else if (part.show_in_ast)
        {
          child->identifier = name;
          child->was_optional = part.optional;
          resolved.children.push_back(std::move(child.value()));
        }
      }
      if (success)
      {
        for (auto it = resolved.children.begin(); it != resolved.children.end();)
        {
          if (it->index() == 1)
          {
            auto const child = std::get<1>(*it);
            if (m_flatten_node.contains(child.identifier))
            {
              it = resolved.children.erase(it);
              it = resolved.children.insert(it, child.children.begin(), child.children.end());
              continue;
            }
          }
          ++it;
        }

        current = current_token;
        return resolved;
      }
      resolved.children.clear();
      current_token = current;
    }

    return std::nullopt;
  }
  void grammar::increment_skip_comments(token_iterator& current, token_iterator end)
  {
    if (current == end)
      return;

    do {
      ++current;
    } while (current != end && current->type == tokenizer::token_type::comment);
  }
  void grammar::try_increment(token_iterator begin, token_iterator current, token_iterator& farthest)
  {
    auto const dist = std::distance(begin, current);
    auto const dist_last = std::distance(begin, farthest);
    if (dist >= dist_last)
    {
      farthest = current;
      m_best_resolved_stack = m_resolve_stack;
    }
  }
}