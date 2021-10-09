#include <myrt/sfml/utils.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <array>
#include <sstream>
#include <regex>
#include <mygl/mygl.hpp>

namespace myrt
{
    namespace detail
    {
        namespace {
            auto make_string_lengths_lists(std::span<std::string_view const> views)
            {
                std::pair<std::vector<char const*>, std::vector<std::int32_t>> pair;
                pair.first.resize(views.size());
                pair.second.resize(views.size());
                for (size_t index = 0; index < views.size(); ++index)
                {
                    pair.first[index] = views[index].data();
                    pair.second[index] = static_cast<int>(views[index].size());
                }
                return pair;
            }
        }
    }

    std::string replace(std::string str, const std::string& sub1, const std::string& sub2)
    {
      if (sub1.empty())
        return str;

      std::size_t pos = 0;
      while ((pos = str.find(sub1, pos)) != std::string::npos) {
        str.replace(pos, sub1.size(), sub2);
        pos += sub2.size();
      }

      return str;
    }

    void pretty_print_line(std::string line) {
        if (line.find("error") != std::string::npos)
            line = "\033[0;31m" + line + "\033[0m";
        else if (line.find("warning") != std::string::npos)
            line = "\033[0;33m" + line + "\033[0m";
        std::cout << line << '\n';
    }

    void pretty_print(std::string const& log) {
        std::stringstream log_stream(log);
        for (std::string line; std::getline(log_stream, line);)
        {
          const std::regex re(R"(([^(]+)[ ]*\(([0-9]+)\)[ ]*:[ ]*([^:]+):)");

          std::match_results<std::string::const_iterator> matches;
          if (std::regex_search(line, matches, re))
          {
          }
          pretty_print_line(line);
        }
    }

    std::optional<std::uint32_t> make_program(std::span<std::string_view const> vertex_shader_codes, std::span<std::string_view const> fragment_shader_codes)
    {
        auto const [vs_codes, vs_lengths] = detail::make_string_lengths_lists(vertex_shader_codes);
        auto const [fs_codes, fs_lengths] = detail::make_string_lengths_lists(fragment_shader_codes);

        std::uint32_t vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, std::int32_t(vs_codes.size()), vs_codes.data(), vs_lengths.data());
        glCompileShader(vs);
        std::uint32_t fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, std::int32_t(fs_codes.size()), fs_codes.data(), fs_lengths.data());
        glCompileShader(fs);
        std::uint32_t prog = glCreateProgram();
        glAttachShader(prog, vs);
        glAttachShader(prog, fs);
        glLinkProgram(prog);

        int log_length = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_length);
        std::string info_log(log_length, ' ');
        glGetProgramInfoLog(prog, log_length, &log_length, info_log.data());
        pretty_print(info_log);

        glDetachShader(prog, vs);
        glDetachShader(prog, fs);
        glDeleteShader(vs);
        glDeleteShader(fs);

        int link_status = 0;
        glGetProgramiv(prog, GL_LINK_STATUS, &link_status);
        if (link_status != GL_TRUE)
        {
            glDeleteProgram(prog);
            return std::nullopt;
        }
        return prog;
    }
    std::optional<std::uint32_t> make_program(std::string_view const vertex_shader_code, std::string_view const fragment_shader_code)
    {
        return make_program({ &vertex_shader_code, 1 }, {&fragment_shader_code, 1});
    }

    std::optional<std::uint32_t> make_program(std::span<std::string_view const> compute_shader_codes)
    {
      auto const [codes, lengths] = detail::make_string_lengths_lists(compute_shader_codes);

      std::uint32_t cs = glCreateShader(GL_COMPUTE_SHADER);
      glShaderSource(cs, std::int32_t(codes.size()), codes.data(), lengths.data());
      glCompileShader(cs);

      int ll = 0;
      glGetShaderiv(cs, GL_INFO_LOG_LENGTH, &ll);
      std::string info_logx(ll, ' ');
      glGetShaderInfoLog(cs, ll, &ll, info_logx.data());
      pretty_print(info_logx);

      std::uint32_t prog = glCreateProgram();
      glAttachShader(prog, cs);
      glLinkProgram(prog);

      int log_length = 0;
      glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_length);
      std::string info_log(log_length, ' ');
      glGetProgramInfoLog(prog, log_length, &log_length, info_log.data());
      pretty_print(info_log);

      glDetachShader(prog, cs);
      glDeleteShader(cs);

      int link_status = 0;
      glGetProgramiv(prog, GL_LINK_STATUS, &link_status);
      if (link_status != GL_TRUE)
      {
        glDeleteProgram(prog);
        return std::nullopt;
      }
      return prog;
    }
    std::optional<std::uint32_t> make_program(std::string_view const compute_shader_code)
    {
      return make_program({ &compute_shader_code, 1 });
    }
}