#include "material.hpp"
#include <fstream>
#include <string_view>
#include <iostream>

namespace myrt::dyn
{
  enum class parse_state
  {
    none,
    global_subscope,
  };

  constexpr bool is_space_or_newline(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
  }

  constexpr bool is_newline(char c) {
    return c == '\n' || c == '\r';
  }

  constexpr bool is_operator(char c)
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
    case '.':
    case ',':
    case ';':
    case '?':
    case ':':
      return true;
    }
    return false;
  }

  char const* advance_spaces(char const* ptr)
  {
    while (*ptr != '\0' && is_space_or_newline(*ptr))
      ++ptr;
    return ptr;
  }

  char const* advance_token(char const* ptr)
  {
    if (is_operator(*ptr))
      return ptr + 1;

    while (*ptr != '\0' && !is_space_or_newline(*ptr) && !is_operator(*ptr))
      ++ptr;
    return ptr;
  }

  class parse_exception : public std::exception {
    using std::exception::exception;
  };

  char const* consume_token(char const* token_begin, char const* token_end, parse_state& state)
  {
    std::string_view token(token_begin, token_end);

    if (token == "input")
    {
      if (state != parse_state::none)
        throw parse_exception("scope must be global");
      state = parse_state::global_subscope;
    }

    return token_end;
  }

  material load_from_file(std::filesystem::path file) {
    std::ifstream file_stream(file);
    std::string source((std::istreambuf_iterator<char>(file_stream)),
      std::istreambuf_iterator<char>());

    parse_state state = parse_state::none;
    char const* ptr = source.data();
    while (*ptr)
    {
      ptr = advance_spaces(ptr);
      auto const end = advance_token(ptr);
      consume_token(ptr, end, state);

      std::cout << "TOK: " << std::string_view(ptr, end) << '\n';

      ptr = end;
    }

    return material{};
  }
}