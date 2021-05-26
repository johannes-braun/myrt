#include "xgl_parser2.hpp"

#include <iostream>
#include <format>

namespace myrt::xgl
{
  struct gmake {

  };

  struct mk {
    string_view_type name;
    std::optional<string_view_type> flat_by;
  };

  struct ay {
    template<size_t S>
    ay(char_type const(&charr)[S]) : ay(string_view_type(charr)) {
    }

    ay(std::optional<string_view_type> n, std::optional<string_view_type> t = std::nullopt, bool o = false) {
      name = n;
      text = t;
      opt = o;
    }

    bool opt;
    std::optional<string_view_type> name;
    std::optional<string_view_type> text;
  };

  const struct epx {} ep;

  ay operator>>(epx, ay&& a)
  {
    return a;
  }

  ay operator>>(epx, ay const& a)
  {
    return a;
  }

  auto mky(ay const& a) {
    return std::make_shared<grammar_node>(grammar_node{ .type_name = a.name, .required_text = a.text, .optional = a.opt });
  }
  struct opt {
    std::vector<std::shared_ptr<grammar_node>> ps;
  };
  struct ox {
    std::vector<std::vector<std::shared_ptr<grammar_node>>> ps;
  };

  struct se {
    mk m;
    ox ac;
  };

  /* template<size_t S>
   opt operator+(char const(&sh)[S], ay a)
   {
     opt o;
     o.ps.push_back(mky(ay{ sh }));
     o.ps.push_back(mky(a));
     return o;
   }*/
  opt operator>>(char const* sh, ay a)
  {
    opt o;
    o.ps.push_back(mky(ay{ string_view_type(sh) }));
    o.ps.push_back(mky(a));
    return o;
  }

  //opt operator>>(char const* sh, ay a)
  //{
  //  opt o;
  //  o.ps.push_back(mky(ay{ string_view_type(sh) }));
  //  o.ps.push_back(mky(a));
  //  return o;
  //}

  opt operator>>(ay sh, ay a)
  {
    opt o;
    o.ps.push_back(mky(sh));
    o.ps.push_back(mky(a));
    return o;
  }

  opt operator>>(opt&& sh, ay a)
  {
    sh.ps.push_back(mky(a));
    return sh;
  }

  ox operator|(ay&& sh, ay&& a)
  {
    ox o;
    o.ps.push_back({ mky(sh) });
    o.ps.push_back({ mky(a) });
    return o;
  }

  ox operator|(ay&& sh, ay const& a)
  {
    ox o;
    o.ps.push_back({ mky(sh) });
    o.ps.push_back({ mky(a) });
    return o;
  }

  ay operator "" _opt(char_type const* str, size_t s) {
    return ay{ string_view_type(str, s), std::nullopt, true };
  }

  ox operator|(ay const& sh, ay const& a)
  {
    ox o;
    o.ps.push_back({ mky(sh) });
    o.ps.push_back({ mky(a) });
    return o;
  }

  ox operator|(opt&& sh, ay&& a)
  {
    ox o;
    o.ps.push_back(std::move(sh.ps));
    o.ps.push_back({ mky(a) });
    return o;
  }

  ox operator|(ay const& a, opt&& sh)
  {
    ox o;
    o.ps.push_back({ mky(a) });
    o.ps.push_back(std::move(sh.ps));
    return o;
  }

  ox operator|(opt&& sh, ay const& a)
  {
    ox o;
    o.ps.push_back(std::move(sh.ps));
    o.ps.push_back({ mky(a) });
    return o;
  }

  ox operator|(opt&& sh, opt&& a)
  {
    ox o;
    o.ps.push_back(std::move(sh.ps));
    o.ps.push_back(std::move(a.ps));
    return o;
  }

  ox operator|(opt&& sh, opt const& a)
  {
    ox o;
    o.ps.push_back(std::move(sh.ps));
    o.ps.push_back(a.ps);
    return o;
  }

  ox operator|(ox&& o, opt const& a)
  {
    o.ps.push_back(a.ps);
    return o;
  }

  ox operator|(ox&& o, ay const& a)
  {
    o.ps.push_back({ mky(a) });
    return o;
  }

  se operator||(mk lhs, ay o)
  {
    se s{ lhs, {{{mky(o)}}} };
    return s;
  }

  se operator||(mk lhs, opt&& o)
  {
    se s{ lhs, {{std::move(o.ps)}} };
    return s;
  }

  se operator||(mk lhs, opt const& o)
  {
    se s{ lhs, {{o.ps}} };
    return s;
  }

  se operator||(mk lhs, ox&& o)
  {
    se s{ lhs, std::move(o) };
    return s;
  }

  template<size_t S>
  ay operator&(ay l, char const(&r)[S])
  {
    l.text = r;
    return l;
  }

  ay operator&(ay l, string_view_type r)
  {
    l.text = r;
    return l;
  }

  auto& operator+=(std::unordered_map<string_view_type, node_or_resolver>& grammar, se m)
  {
    auto n = std::make_shared<grammar_node>(grammar_node{ .type_name = m.m.name });
    n->parts = std::move(m.ac.ps);
    n->flatten_by = m.m.flat_by;
    return std::get<1>(grammar[m.m.name] = n);
  }

  struct yo {
    std::function<std::optional<node>()> f;
  };

  struct su {
    mk m;
    yo ac;
  };

  su operator||(mk lhs, std::function<std::optional<node>()> o)
  {
    su s{ lhs, yo{std::move(o)} };
    return s;
  }

  auto& operator+=(std::unordered_map<string_view_type, node_or_resolver>& grammar, su m)
  {
    grammar[m.m.name] = std::move(m.ac.f);
    return grammar;
  }

  mk create(string_view_type v, std::optional<string_view_type> flat_by = std::nullopt) {
    return mk{ v, flat_by };
  }

  ay ex(string_view_type type, std::optional<string_view_type> text = std::nullopt, bool op = false)
  {
    return ay{ type, text, op };
  }

  ay tx(string_view_type text, bool op = false)
  {
    return ay{ std::nullopt, text, op };
  }

  static opt any{};

  /*

  grammar["global"] = mk("global") | ex("type") + ex("")

  */

  static const auto expect(string_view_type type, std::optional<string_view_type> text = std::nullopt) {
    return std::make_shared<grammar_node>(grammar_node{ .type_name = type, .required_text = text });
  }

  static const auto exp_type = expect("type");
  static const auto exp_ident = expect("identifier");
  static const auto exp_lit = expect("literal");
  static const auto exp_expr = expect("expression");
  static const auto exp_expr_ext = expect("expression_ext");
  static const auto exp_aop = expect("arithmetic_operator");
  static const auto exp_semicolon = expect("operator", ";");

  void print(node const& n, int indent)
  {
    std::format_to(std::ostreambuf_iterator(std::cout), "{}{} :\n", std::string(indent, ' '), n.type_name);

    struct {
      void operator()(token_iterator const& tok) const {
        std::format_to(std::ostreambuf_iterator(std::cout), "{} -> {}\n", std::string(i, ' '), tok->text);
      }

      void operator()(std::shared_ptr<node> const& node) const {
        print(*node, 3 + i);
      }
      int i;
    } expect_visitor{ .i = indent };

    for (auto const& ch : n.children)
    {
      std::visit(expect_visitor, ch);
    }
  }

  parser::parser(xgl_tokenizer const& tokenizer)
  {
    m_defined_types["void"] = type{ .name = "void", .type = builtin_type::void_type };
    m_defined_types["bool"] = type{ .name = "bool", .type = builtin_type::bool_type };
    m_defined_types["int"] = type{ .name = "int", .type = builtin_type::int_type };
    m_defined_types["float"] = type{ .name = "float", .type = builtin_type::float_type };
    m_defined_types["vec3"] = type{ .name = "vec3", .type = builtin_type::vec3_type };
    m_defined_types["vec2"] = type{ .name = "vec2", .type = builtin_type::vec2_type };

    m_functions["cross"] = 8;
    m_functions["dot"] = 8;

    // TODO: add errifn = error if expression is not matched from here on (e.g. no closing bracket in function).
    {
      auto const type = ex("type");
      auto const id = ex("id");
      auto const op = ex("op");
      auto const arop = ex("arop");
      auto const lit = ex("lit");
      auto const var = ex("var");
      auto const fun = ex("fun");

      auto const sc_push = ex("scope_begin");
      auto const sc_pop = ex("scope_end");

      m_grammar += create("global") || (
        ep >> "func_decl" |
        ep >> "func_def" |
        ep >> "struct_decl" |
        ep >> "struct_def"
        );

      m_grammar += create("scope_begin") || any;
      m_grammar += create("scope_end") || any;

      m_grammar += create("func_hdr") ||
        ep >> type >> id >> "(" >> "func_params"_opt >> ")";

      // One or more function params
      m_grammar += create("func_params", "func_params") || (
        ep >> "func_param" >> "," >> "func_params" |
        ep >> "func_param"
        );

      m_grammar += create("func_param") || (
        ep >> "expr_var_decl" |
        ep >> type
        );

      m_grammar += create("func_decl") ||
        ep >> sc_push >> "func_hdr" >> sc_pop >> ";";

      m_grammar += create("func_def") ||
        ep >> sc_push >> "func_hdr" >> "{" >> "statement_list"_opt >> sc_pop >> "}";

      m_grammar += create("struct_decl") ||
        ep >> "struct" >> id >> ";";

      m_grammar += create("struct_def") ||
        ep >> "struct" >> id >> "{" >> sc_push >> "member_list"_opt >> sc_pop >> "}" >> ";";

      m_node_success_direct["struct_def"] = [this](node const& n) {

        auto const type_name = std::get<0>(std::get<1>(n.children[1])->children[0])->text;

        auto const found_definition = m_defined_types.find(type_name);

        if (found_definition != m_defined_types.end() && found_definition->second.defined)
        {
          std::format_to(std::ostreambuf_iterator(std::cerr), "Type already defined: {}.\n", type_name);
          return false;
        }

        xgl::type new_type;
        new_type.name = type_name;
        new_type.type = builtin_type::struct_type;
        new_type.defined = true;
        if (std::shared_ptr<node> const& cur = std::get<1>(n.children[3]); cur->type_name == "member_list")
        {
          for (auto const& def : cur->children)
          {
            auto const& var_def = std::get<1>(std::get<1>(def)->children[0]);

            token_iterator type_tok;
            std::optional<token_iterator> name_tok;
            type_tok = std::get<0>(std::get<1>(var_def->children[0])->children[0]);
            if (var_def->children.size() > 1)
              name_tok = std::get<0>(std::get<1>(var_def->children[1])->children[0]);

            variable& member = new_type.fields.emplace_back();
            if (name_tok)
              member.name = name_tok.value();
            member.type = m_defined_types.find(type_tok->text);
          }
        }

        m_defined_types[type_name] = std::move(new_type);

        std::format_to(std::ostreambuf_iterator(std::cout), "Register type: {}.\n", type_name);
        return true;
      };

      m_node_success_direct["func_hdr"] = [this](node const& n) {
        auto const tok = std::get<0>(std::get<1>(n.children[1])->children[0]);

        auto const ib = m_functions.emplace(tok->text, 232);

        m_on_cancel.back().push_back([ib, this] {
          if (ib.second)
            m_functions.erase(ib.first);
          });
        return true;
      };

      m_node_success_direct["struct_decl"] = [this](node const& n) {

        auto const type_name = std::get<0>(std::get<1>(n.children[1])->children[0])->text;

        xgl::type new_type;
        new_type.name = type_name;
        new_type.type = builtin_type::struct_type;
        new_type.defined = false;

        m_defined_types[type_name] = std::move(new_type);

        std::format_to(std::ostreambuf_iterator(std::cout), "Register (fwd) type: {}.\n", type_name);
        return true;
      };

      m_var_stack.push_back({}); // global vars
      m_node_success_direct["scope_begin"] = [this](node const& n) mutable {
        push_scope();
        m_on_cancel.back().push_back([this] {
          pop_scope();
          });
        return true;
      };
      m_node_success_post["scope_end"] = [this](node const& n) mutable {
        return pop_scope();
      };

      m_node_success_direct["expr_var_decl"] = [&](node const& n) {
        auto const& var_def = std::get<1>(n.children[0]);

        token_iterator type_tok;
        std::optional<token_iterator> name_tok;

        for (auto const& ch : std::get<1>(n.children[0])->children)
        {
          if (ch.index() == 1)
          {
            auto const& g = std::get<1>(ch);
            if (g->type_name == "type_base")
            {
              type_tok = std::get<0>(g->children[0]);
              break;
            }
          }
        }

        if (n.children.size() > 1)
          name_tok = std::get<0>(std::get<1>(n.children[1])->children[0]);

        if (!name_tok)
          return true;

        auto& cur = m_var_stack.back();
        if (auto const it = std::find_if(cur.begin(), cur.end(), [&](auto const& p) { return p.name.value()->text == name_tok.value()->text; }); it != cur.end())
        {
          if (name_tok.value()->line != it->name.value()->line && name_tok.value()->column_start != it->name.value()->column_start)
          {
            std::format_to(std::ostreambuf_iterator(std::cerr), "Variable already defined in the current scope: {} at {}:{} (see definition at {}:{})\n", name_tok.value()->text,
              name_tok.value()->line, name_tok.value()->column_start, it->name.value()->line, it->name.value()->column_start);
          }
        }

        auto const& type = m_defined_types.find(type_tok->text);
        variable var;
        var.name = name_tok;
        var.type = type;

        auto ib0 = cur.insert(cur.begin(), var);
        auto it1 = m_current_vars.emplace(name_tok.value()->text, var);

        m_on_cancel.back().push_back([ib0, it1, this] {
          m_var_stack.back().erase(ib0);
          m_current_vars.erase(it1);
          });

        return true;
      };
      m_grammar += create("member_list", "member_list") || (
        ep >> "member_def" >> "member_list" |
        ep >> "member_def"
        );

      m_grammar += create("member_def") ||
        ep >> "expr_var_decl" >> ";" |
        ep >> "expr_var_def" >> ";";

      m_grammar += create("statement_list", "statement_list") ||
        ep >> "statement" >> "statement_list"_opt |
        ep >> "{" >> sc_push >> "statement_list"_opt >> sc_pop >> "}";

      m_grammar += create("statement_or_block") ||
        ep >> "{" >> sc_push >> "statement_list"_opt >> sc_pop >> "}" |
        ex("statement");

      m_grammar += create("statement", "statement_or_block") ||
        ep >> "expr" >> ";" |
        ep >> "while" >> sc_push >> "(" >> "val_expr" >> ")" >> "statement_or_block" >> sc_pop |
        ep >> "do" >> sc_push >> "statement_or_block" >> "while" >> "(" >> "val_expr" >> ")" >> sc_pop >> ";" |
        ep >> "if" >> sc_push >> "(" >> "val_expr" >> ")" >> "statement_or_block" >> "else"_opt >> sc_pop |
        ep >> "return" >> "val_expr" >> ";" |
        ep >> ";";

      m_grammar += create("else", "statement_or_block") ||
        ep >> (id & "else") >> "statement_or_block";

      m_grammar += create("expr_var_decl") ||
        ep >> type >> id |
        ep >> type;

      m_grammar += create("expr_var_def", "expr_var_decl") ||
        ep >> "expr_var_decl" >> "=" >> "val_expr";

      m_grammar += create("expr") ||
        ep >> "val_expr" |
        ep >> "val_expr_base" >> "?" >> "expr" >> ":" >> "expr" |
        ep >> "expr_var_def" |
        ep >> "expr_var_decl";

      m_grammar += create("expr_list", "expr_list") ||
        ep >> "val_expr" >> "," >> "expr_list" |
        ep >> "val_expr";

      m_grammar += create("cast") || 
        ep >> type >> "(" >> "val_expr" >> ")";

      m_grammar += create("construct") || 
        ep >> type >> "(" >> "expr_list" >> ")" |
        ep >> type >> "{" >> "expr_list" >> "}";

      m_grammar += create("val_expr") || (
        ep >> "val_expr_base" >> arop >> "val_expr" |
        ep >> "val_expr_base" >> "?" >> "val_expr" >> ":" >> "val_expr" |

        // TODO: id must be member or swizzle
        (ex("val_dot_expr_list") >> arop >> ex("val_expr")) |
        (ex("val_dot_expr_list") >> (op & "?") >> ex("val_expr") >> (op & ":") >> ex("val_expr")) |

        (ex("val_dot_expr_list")) |
        ex("val_expr_base")
        );

      m_grammar += create("val_dot_expr_list", "val_dot_expr_list_cont") ||
        // TODO: id must be member or swizzle
        ep >> "val_expr_base" >> "val_dot_expr_list_cont";

      m_grammar += create("val_dot_expr_list_cont", "val_dot_expr_list_cont") ||
        // TODO: id must be member or swizzle
        ep >> "." >> id >> "val_dot_expr_list_cont"_opt;

      m_grammar += create("val_expr_base") ||
        ep >> "(" >> "val_expr" >> ")" |
        ep >> "+" >> "val_expr" |
        ep >> "-" >> "val_expr" |
        ep >> "construct" |
        ep >> "cast" |
        ep >> fun >> "(" >> "expr_list" >> ")" |
        ep >> lit |
        ep >> var;

      m_grammar += create("type_base") || [this] { return expect_type(); };
      m_grammar += create("id") || [this] { return expect_identifier(); };
      m_grammar += create("op") || [this] { return expect_operator(); };
      m_grammar += create("builtin_lit") || [this] { return expect_literal(); };
      m_grammar += create("arop") || [this] { return expect_arithmetic_operator(); };
      m_grammar += create("var") || [this] { return expect_variable(); };
      m_grammar += create("fun") || [this] { return expect_function_name(); };

      m_grammar += create("lit") || (
        ep >> "builtin_lit" |
        ep >> "true" |
        ep >> "false"
        );

      auto type_attrs = m_grammar += create("type_attrs") ||
        ep >> "type_attr" >> "type_attrs"_opt;

      m_grammar += create("type") ||
        ep >> "type_attrs"_opt >> "type_base" >> "type_attrs"_opt;

      m_grammar += create("type_attr") || (
        ep >> "const" |
        ep >> "inout" |
        ep >> "in" |
        ep >> "out"
        );

    }

    m_current_token = tokenizer.tokens().begin();
    m_end_token = tokenizer.tokens().end();

    while (m_current_token != m_end_token)
    {
      auto const g = expect_grammar(*std::get<1>(m_grammar["global"]));

      if (g)
        print(g.value(), 0);
      __debugbreak();
    }

    m_current_token = tokenizer.tokens().begin();
    m_end_token = tokenizer.tokens().end();

    while (m_current_token != m_end_token)
    {
      auto const g = expect_grammar(*std::get<1>(m_grammar["global_scope"]));

      if (g)
        print(g.value(), 0);
      __debugbreak();
    }
  }

  static int indent = 0;
  std::optional<node> parser::expect_grammar(grammar_node const& gnode)
  {
    //std::format_to(std::ostreambuf_iterator(std::cout), "GET {}.\n", gnode.type_name);
    struct {
      parser* self;

      std::optional<node> operator()(std::function<std::optional<node>()> const& fun) const {
        return fun();
      }

      std::optional<node> operator()(std::shared_ptr<grammar_node> const& node) const {
        return self->expect_grammar(*node);
      }
    } expect_visitor{ .self = this };

    node n;
    n.type_name = *gnode.type_name;
    for (auto& child : gnode.parts)
    {
      begin_expect();
      if (child.empty())
      {
        std::format_to(std::ostreambuf_iterator(std::cout), "{} <- success (empty)\n", std::string(indent, ' '));
        return n;
      }

      bool success = true;
      for (auto& part : child)
      {
        auto name = part->type_name;
        auto text = part->required_text;

        std::optional<node> opt;

        decltype(m_grammar.begin()) gnit;
        if (name)
        {
          gnit = m_grammar.find(*name);
          if (gnit == m_grammar.end())
          {
            if (!text)
            {
              text = name;
              name = std::nullopt;
            }
            else {
              std::format_to(std::ostreambuf_iterator(std::cerr), "Grammar entry for {} not found. Using as raw text\n", *name);

              success = false;
              break;
            }
          }
        }


        if (name) {
          auto const& node = gnit->second;

          /*if (text)
            std::format_to(std::ostreambuf_iterator(std::cout), "{} > {} [{}:{}]: {}@{}\n", std::string(indent, ' '), m_current_token->text, m_current_token->line, m_current_token->column_start, *name, *text);
          else
            std::format_to(std::ostreambuf_iterator(std::cout), "{} > {} [{}:{}]: {}\n", std::string(indent, ' '), m_current_token->text, m_current_token->line, m_current_token->column_start, *name);*/

          indent += 3;
          opt = std::visit(expect_visitor, node);
          indent -= 3;
          if (text)
            opt = expect_text(text.value(), opt);
        }
        else if (text)
        {
          if (m_current_token->text == text)
          {
            opt = node{
              .children = { m_current_token }
            };
            opt->type_name = name ? *name : *text;
            opt->was_optional = part->optional;
            ++m_current_token;
          }
        }
        else
        {
          std::format_to(std::ostreambuf_iterator(std::cerr), "name and text are nullopt!\n");
          success = false;
          break;
        }

        if (opt && (name || text))
        {
          if (auto const callback_it = m_node_success_direct.find(name ? *name : *text); callback_it != m_node_success_direct.end())
          {
            if (!callback_it->second(*opt))
            {
              opt = std::nullopt;
            }
          }
        }

        /*   if(name && text)
             std::format_to(std::ostreambuf_iterator(std::cout), "{} > {}: {}@{} ??? {} (optional: {}).\n", std::string(indent, ' '),
               m_previous_tokens.top()->text, *name, *text, opt != std::nullopt, part->optional);
           else
             std::format_to(std::ostreambuf_iterator(std::cout), "{} > {}: {} ??? {} (optional: {}).\n", std::string(indent, ' '), m_previous_tokens.top()->text, name ? *name : *text, opt != std::nullopt, part->optional);*/

        if (!opt && !part->optional)
        {
          success = false;
          break;
        }
        else if (!opt && part->optional)
        {
          //std::format_to(std::ostreambuf_iterator(std::cout), "Skipping optional {}.\n", name ? *name : *text);
        }
        else
        {
          opt->type_name = name ? *name : *text;
          opt->was_optional = part->optional;
          n.children.push_back(std::make_shared<xgl::node>(std::move(opt.value())));
        }
      }
      if (success)
      {
        for (auto it = n.children.begin(); it != n.children.end();)
        {
          auto& opt = *it;
          if (opt.index() == 1)
          {
            auto& val = std::get<1>(opt);
            if (auto const callback_it = m_node_success_post.find(val->type_name); callback_it != m_node_success_post.end())
            {
              auto const good = callback_it->second(*val);
              if (!good && val->was_optional)
              {
                //std::format_to(std::ostreambuf_iterator(std::cout), "Skipping optional {}.\n", val->type_name);
                it = n.children.erase(it);
                continue;
              }
              else if (!good)
              {
                success = false;
                n.children.clear();
                cancel_expect();
                //std::format_to(std::ostreambuf_iterator(std::cout), "{} <- failed\n", std::string(indent, ' '));
                continue;
              }
            }
          }
          ++it;
        }
        //std::format_to(std::ostreambuf_iterator(std::cerr), "GOT -- {}.\n", n.type_name);

        if (gnode.flatten_by)
        {
          auto const ftext = gnode.flatten_by;

          auto it = n.children.begin();
          for (; it != n.children.end();)
          {
            if (it->index() == 1)
            {
              auto const child = std::get<1>(*it);
              if (child->type_name == gnode.flatten_by)
              {
                it = n.children.erase(it);
                it = n.children.insert(it, child->children.begin(), child->children.end());
              }
              else
              {
                ++it;
              }
            }
            else
            {
              ++it;
            }
          }
        }

        m_on_cancel.push_back({});
        pop_expect();
        //std::format_to(std::ostreambuf_iterator(std::cout), "{} <- success\n", std::string(indent, ' '));
        return n;
      }
      n.children.clear();
      cancel_expect();
    }

    //std::format_to(std::ostreambuf_iterator(std::cout), "{} <- failed\n", std::string(indent, ' '));
    return std::nullopt;
  }

  std::optional<node> parser::expect_type()
  {
    auto identifier = expect_identifier();
    if (!identifier)
      return std::nullopt;

    if (auto const it = m_defined_types.find(std::get<token_iterator>(identifier.value().children[0])->text); it == m_defined_types.end())
      return std::nullopt;
    else
    {
      identifier->type_name = "type";
      return identifier;
    }
  }
  std::optional<node> parser::expect_identifier()
  {
    if (m_current_token == m_end_token || m_current_token->type != xgl_tokenizer::token_type::identifier)
    {
      return std::nullopt;
    }

    auto tok = m_current_token;
    m_current_token++;

    node n;
    n.children.push_back(tok);
    n.type_name = "identifier";
    return n;
  }
  std::optional<node> parser::expect_literal()
  {
    if (m_current_token == m_end_token || (
      m_current_token->type != xgl_tokenizer::token_type::double_literal &&
      m_current_token->type != xgl_tokenizer::token_type::float_literal &&
      m_current_token->type != xgl_tokenizer::token_type::int_literal &&
      m_current_token->type != xgl_tokenizer::token_type::uint_literal &&
      m_current_token->type != xgl_tokenizer::token_type::long_literal &&
      m_current_token->type != xgl_tokenizer::token_type::ulong_literal &&
      m_current_token->type != xgl_tokenizer::token_type::str_literal))
    {
      return std::nullopt;
    }

    auto tok = m_current_token;
    m_current_token++;

    node n;
    n.children.push_back(tok);
    n.type_name = "literal";
    return n;
  }
  std::optional<node> parser::expect_operator()
  {
    if (m_current_token == m_end_token || m_current_token->type != xgl_tokenizer::token_type::op)
    {
      return std::nullopt;
    }

    auto tok = m_current_token;
    m_current_token++;

    node n;
    n.children.push_back(tok);
    n.type_name = "operator";
    return n;
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

  std::optional<node> parser::expect_arithmetic_operator()
  {
    auto const op1 = expect_operator();
    if (!op1 || std::get<token_iterator>(op1.value().children[0])->text.find_first_not_of("+-*/%<>=!^&|") != string_view_type::npos)
    {
      return std::nullopt;
    }

    auto const tok_before_2 = m_current_token;
    auto const op2 = expect_operator();
    if (!op2)
    {
      return op1;
    }

    if (std::get<token_iterator>(op2.value().children[0])->text.find_first_not_of("+-*/%<>=!^&|") != string_view_type::npos)
    {
      --m_current_token;
      return op1;
    }


    std::string combined;
    combined.reserve(3);
    combined.append(std::get<token_iterator>(op1.value().children[0])->text);
    combined.append(std::get<token_iterator>(op2.value().children[0])->text);

    auto const op3 = expect_operator();
    if (op3 && std::get<token_iterator>(op3.value().children[0])->text.find_first_not_of("+-*/%<>=!^&|") != string_view_type::npos)
      --m_current_token;
    else if (op3)
      combined.append(std::get<token_iterator>(op3.value().children[0])->text);

    node n{
        .type_name = "arithmetic_operator"
    };

    switch (packed_op(combined.c_str()))
    {
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
    case packed_op("&&"):
    case packed_op("||"):
      n.children.insert(n.children.end(), op1.value().children.begin(), op1.value().children.end());
      n.children.insert(n.children.end(), op2.value().children.begin(), op2.value().children.end());
      return n;

    case packed_op("<<="):
    case packed_op(">>="):
      n.children.insert(n.children.end(), op1.value().children.begin(), op1.value().children.end());
      n.children.insert(n.children.end(), op2.value().children.begin(), op2.value().children.end());
      n.children.insert(n.children.end(), op3.value().children.begin(), op3.value().children.end());
      return n;
    }

    m_current_token = tok_before_2;
    return op1;
  }
  std::optional<node> parser::expect_text(string_view_type text, std::optional<node> node)
  {
    if (!node)
      return std::nullopt;

    if (node->children.size() != 1 || node->children[0].index() != 0)
    {
      throw std::exception("Nop");
    }

    if (std::get<0>(node->children[0])->text == text)
      return node;
    return std::nullopt;
  }
  std::optional<node> parser::expect_variable()
  {
    auto const ident = expect_identifier();
    if (!ident)
    {
      --m_current_token;
      return std::nullopt;
    }

    bool found = false;
    auto stack_it = m_var_stack.rbegin();
    while (!found && stack_it != m_var_stack.rend())
    {
      auto const it = std::find_if(stack_it->begin(), stack_it->end(), [&](variable const& var) {
        return var.name.value()->text == std::get<0>(ident->children[0])->text;
        });
      if (it != stack_it->end())
        return ident;

      ++stack_it;
    }

    --m_current_token;
    return std::nullopt;
  }
  std::optional<node> parser::expect_function_name()
  {
    auto const ident = expect_identifier();
    if (!ident)
    {
      --m_current_token;
      return std::nullopt;
    }

    auto const it = m_functions.find(std::get<0>(ident->children[0])->text);
    if (it == m_functions.end())
    {
      --m_current_token;
      return std::nullopt;
    }
    return ident;
  }
  void parser::push_scope()
  {
    m_var_stack.push_back({});
  };
  bool parser::pop_scope()
  {
    std::format_to(std::ostreambuf_iterator(std::cout), "------ POP ------\n");
    for (auto const& var : m_var_stack.back())
    {
      auto it = std::find_if(m_current_vars.begin(), m_current_vars.end(), [&](std::pair<string_view_type, variable> const& p) {
        return p.second.name == var.name && p.second.type == var.type;
        });

      if (it != m_current_vars.end())
      {
        std::format_to(std::ostreambuf_iterator(std::cout), "RM {} {}\n", var.type->first, var.name.value()->text);
        m_current_vars.erase(it);
      }
    }
    m_var_stack.pop_back();
    if (m_var_stack.empty())
    {
      std::format_to(std::ostreambuf_iterator(std::cerr), "Incorrect scoping.");
      return false;
    }
    return true;
  }
  void parser::begin_expect()
  {
    m_previous_tokens.push(m_current_token);
    m_on_cancel.emplace_back();
  }
  std::nullopt_t parser::cancel_expect() {

    if (m_previous_tokens.empty())
      throw std::exception("Meh");

    for (auto& on_cancel : m_on_cancel.back())
    {
      on_cancel();
    }

    m_current_token = m_previous_tokens.top();
    pop_expect();
    return std::nullopt;
  }
  void parser::pop_expect()
  {
    m_on_cancel.pop_back();
    auto top = m_previous_tokens.top();
    m_previous_tokens.pop();
  }
}