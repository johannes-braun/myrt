#include "xgl_tokenizer.hpp"
#include <format>
#include <iostream>

namespace myrt
{

  constexpr bool is_space(char_type c) {
    return c == ' ' || c == '\t';
  }

  constexpr bool is_space_or_newline(char_type c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
  }

  constexpr bool is_newline(char_type c) {
    return c == '\n' || c == '\r';
  }

  constexpr bool is_operator(char_type c)
  {
    switch (c) {
    case '+':
    case '-':
    case '*':
    case '/':
    case '&':
    case '|':
    case '^':
    case '(':
    case ')':
    case '[':
    case ']':
    case '=':
    case '{':
    case '}':
    case '~':
    case '!':
    case '<':
    case '>':
    case ',':
    case ';':
    case '?':
    case ':':
      return true;
    }
    return false;
  }

  constexpr char_type const* advance_spaces(char_type const* ptr)
  {
    while (*ptr != '\0' && is_space_or_newline(*ptr))
      ++ptr;
    return ptr;
  }

  constexpr char_type const* advance_token(char_type const* ptr)
  {
    if (is_operator(*ptr))
      return ptr + 1;

    while (*ptr != '\0' && !is_space_or_newline(*ptr) && !is_operator(*ptr))
      ++ptr;
    return ptr;
  }

  void xgl_tokenizer::error(token const& tok, string_view_type message)
  {
    std::format_to(std::ostreambuf_iterator(std::cerr), "[error ({}:{})] {}\n", tok.line, tok.column_start, message);
  }

  void xgl_tokenizer::close(xgl_tokenizer::token& tok)
  {
    if (tok.type != token_type::space)
    {
      if (tok.type == token_type::double_literal_extended)
        tok.type = token_type::double_literal;
      m_tokens.push_back(tok);
    }

    tok.type = token_type::space;
    tok.text = string_view_type(tok.text.end(), tok.text.end());
    tok.column_start = tok.column_end;
  }

  xgl_tokenizer::xgl_tokenizer(string_type source)
    : m_source(std::move(source))
  {
    token current_token;

    auto const* ptr = m_source.data();
    current_token.text = string_view_type(ptr, ptr + 1);
    bool cr_lf_filter = false;
    while (ptr && *ptr)
    {
      if (cr_lf_filter && *ptr != '\n')
        cr_lf_filter = false;

      if (current_token.type == token_type::potential_comment && *ptr != '/' && *ptr != '*')
      {
        current_token.type = token_type::op;
        close(current_token);
      }
      if (current_token.type == token_type::potential_double && !std::isdigit(*ptr))
      {
        current_token.type = token_type::op;
        close(current_token);
      }

      if (std::isdigit(*ptr) && current_token.type != token_type::uint_literal && current_token.type != token_type::long_literal && current_token.type != token_type::ulong_literal)
      {
        if (current_token.type == token_type::space)
        {
          current_token.type = token_type::int_literal;
          current_token.text = string_view_type(ptr, ptr + 1);
        }
        else if (current_token.type == token_type::potential_double)
        {
          current_token.type = token_type::double_literal;
          current_token.text = string_view_type(current_token.text.data(), ptr + 1);
        }
        else if (current_token.type == token_type::double_exp_literal || current_token.type == token_type::double_exp_signed_literal)
        {
          current_token.type = token_type::double_literal_extended;
          current_token.text = string_view_type(current_token.text.data(), ptr + 1);
        }
        else
        {
          current_token.text = string_view_type(current_token.text.data(), ptr + 1);
        }
      }
      else if ((*ptr == '+' || *ptr == '-') && current_token.type == token_type::double_exp_literal)
      {
        current_token.type = token_type::double_exp_signed_literal;
        current_token.text = string_view_type(current_token.text.data(), ptr + 1);
      }
      else if (*ptr == '.')
      {
        if (current_token.type == token_type::space)
        {
          current_token.type = token_type::potential_double;
          current_token.text = string_view_type(ptr, ptr + 1);
        }
        else if (current_token.type == token_type::int_literal)
        {
          current_token.type = token_type::double_literal;
          current_token.text = string_view_type(current_token.text.data(), ptr + 1);
        }
        else if (current_token.type == token_type::str_literal)
        {
          current_token.text = string_view_type(current_token.text.data(), ptr + 1);
        }
        else
        {
          close(current_token);
          current_token.type = token_type::op;
          current_token.text = string_view_type(ptr, ptr + 1);
          close(current_token);
        }
      }
      else if (std::tolower(*ptr) == 'u' && (current_token.type == token_type::int_literal))
      {
        current_token.type = token_type::uint_literal;
        current_token.text = string_view_type(current_token.text.data(), ptr + 1);
      }
      else if (std::tolower(*ptr) == 'l' && (current_token.type == token_type::int_literal || current_token.type == token_type::uint_literal))
      {
        if (current_token.type == token_type::uint_literal)
        {
          current_token.type = token_type::ulong_literal;
        }
        else if (current_token.type == token_type::int_literal)
        {
          current_token.type = token_type::long_literal;
        }
        current_token.text = string_view_type(current_token.text.data(), ptr + 1);
      }
      else if (std::tolower(*ptr) == 'e' && (current_token.type == token_type::int_literal || current_token.type == token_type::double_literal))
      {
        current_token.type = token_type::double_exp_literal;
        current_token.text = string_view_type(current_token.text.data(), ptr + 1);
      }
      else if (std::tolower(*ptr) == 'f' && (current_token.type == token_type::double_literal || current_token.type == token_type::double_literal_extended))
      {
        current_token.type = token_type::float_literal;
        current_token.text = string_view_type(current_token.text.data(), ptr + 1);
        close(current_token);
      }
      else if (is_operator(*ptr) && current_token.type != token_type::str_literal) {

        if (current_token.type == token_type::potential_comment)
        {
          if (*ptr == '/')
          {
            while (*(ptr + 1) && !is_newline(*(ptr + 1)))
            {
              ++current_token.column_end;
              ++ptr;
            }

            current_token.type = token_type::space;
            close(current_token);
            ++current_token.column_end;
            ++ptr;
            continue;
          }
          else if (*ptr == '*')
          {
            cr_lf_filter = false;
            bool end_comment = false;
            do {
              if (cr_lf_filter && *ptr != '\n')
                cr_lf_filter = false;

              if (is_newline(*ptr))
              {
                if (*ptr != '\n' || !cr_lf_filter)
                {
                  if (*ptr == '\r')
                    cr_lf_filter = true;

                  ++current_token.line;
                  current_token.column_start = 0;
                  current_token.column_end = 0;
                }
              }

              ++current_token.column_end;
              ++ptr;
              auto const is_star = *ptr == '*';
              end_comment = is_star && *(ptr + 1) == '/';
            } while (!end_comment && *ptr);

            if (!*ptr)
            {
              error(current_token, "Closing symbols for multiline comment not found.");
            }

            current_token.type = token_type::space;
            close(current_token);
            current_token.column_end += 2;
            ptr += 2;
            continue;
          }
        }
        else if (*ptr == '/')
        {
          close(current_token);
          current_token.text = string_view_type(ptr, ptr + 1);
          current_token.type = token_type::potential_comment;
        }
        else
        {
          close(current_token);
          current_token.text = string_view_type(ptr, ptr + 1);
          current_token.type = token_type::op;
          close(current_token);
        }
      }
      else if (is_space(*ptr) && current_token.type != token_type::str_literal)
      {
        close(current_token);
      }
      else if (is_newline(*ptr))
      {
        if (current_token.type == token_type::str_literal)
        {
          error(current_token, "Closing \" for string literal not found.");
        }

        if (*ptr != '\n' || !cr_lf_filter)
        {
          if (*ptr == '\r')
            cr_lf_filter = true;

          close(current_token);
         /* current_token.type = token_type::new_line;
          close(current_token);*/
          ++current_token.line;
          current_token.column_start = 0;
          current_token.column_end = 0;
        }
      }
      else if (*ptr == '"')
      {
        if (current_token.type != token_type::str_literal)
        {
          close(current_token);
          current_token.type = token_type::str_literal;
          current_token.text = string_view_type(ptr + 1, ptr + 1);
        }
        else {
          close(current_token);
        }
      }
      else if (*ptr == '\\' && current_token.type != token_type::str_literal)
      {
        close(current_token);
        current_token.type = token_type::op;
        current_token.text = string_view_type(ptr, ptr + 1);
        close(current_token);
      }
      else
      {
        if (current_token.type == token_type::potential_double)
        {
          current_token.type = token_type::op;
          close(current_token);
          current_token.type = token_type::identifier;
          current_token.text = string_view_type(ptr, ptr + 1);
        }
        else if (current_token.type == token_type::space)
        {
          close(current_token);
          current_token.type = token_type::identifier;
          current_token.text = string_view_type(ptr, ptr + 1);
        }
        else if (current_token.type == token_type::int_literal ||
          current_token.type == token_type::uint_literal ||
          current_token.type == token_type::long_literal ||
          current_token.type == token_type::ulong_literal ||
          current_token.type == token_type::double_exp_literal ||
          current_token.type == token_type::double_literal_extended)
        {
          error(current_token, "Invalid character found after numeric literal");
          return;
        }
        else
          current_token.text = string_view_type(current_token.text.data(), ptr + 1);
      }
      ++current_token.column_end;
      ++ptr;
    }

    if (current_token.type == token_type::str_literal)
    {
      error(current_token, "Closing \" for string literal not found.");
    }
    close(current_token);
  }
}