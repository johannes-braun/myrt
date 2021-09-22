#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_set>

namespace myrt::xgl
{
  using char_type = char;
  using string_type = std::basic_string<char_type>;
  using string_view_type = std::basic_string_view<char_type>;

  class tokenizer
  {
  public:
    enum class token_type
    {
      space,
      identifier,
      int_literal,
      uint_literal,
      long_literal,
      ulong_literal,
      potential_double,
      double_literal,
      double_exp_literal,
      double_exp_signed_literal,
      double_literal_extended,
      float_literal,
      str_literal,
      str_escape,
      op,
      new_line,
      potential_comment,
      comment
    };

    struct token
    {
      string_view_type text;
      token_type type = token_type::space;
      ptrdiff_t line = 0;
      ptrdiff_t column_start = 0;
      ptrdiff_t column_end = 0;
    };

    tokenizer(string_type source);
    std::vector<token> const& tokens() const { return m_tokens; }

  private:
    void error(token const& tok, string_view_type message);
    void close(token& tok);

    string_type m_source;
    std::vector<token> m_tokens;
  };

  using token_iterator = std::vector<tokenizer::token>::const_iterator;
}