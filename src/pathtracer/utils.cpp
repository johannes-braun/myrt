#include "utils.hpp"
#include <vector>
#include <string>
#include <iostream>

namespace myrt
{
    namespace detail
    {
        namespace {
            auto make_string_lengths_lists(std::span<std::string_view const> views)
            {
                std::pair<std::vector<char const*>, std::vector<GLint>> pair;
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

    GLuint make_program(std::span<std::string_view const> vertex_shader_codes, std::span<std::string_view const> fragment_shader_codes)
    {
        auto const [vs_codes, vs_lengths] = detail::make_string_lengths_lists(vertex_shader_codes);
        auto const [fs_codes, fs_lengths] = detail::make_string_lengths_lists(fragment_shader_codes);

        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, GLsizei(vs_codes.size()), vs_codes.data(), vs_lengths.data());
        glCompileShader(vs);
        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, GLsizei(fs_codes.size()), fs_codes.data(), fs_lengths.data());
        glCompileShader(fs);
        GLuint prog = glCreateProgram();
        glAttachShader(prog, vs);
        glAttachShader(prog, fs);
        glLinkProgram(prog);

        int status = 0;
        glGetProgramiv(prog, GL_LINK_STATUS, &status);
        if (!status)
        {
            glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &status);
            std::string info_log(status, ' ');
            glGetProgramInfoLog(prog, status, &status, info_log.data());
            std::cout << info_log << '\n';
        }

        glDetachShader(prog, vs);
        glDetachShader(prog, fs);
        glDeleteShader(vs);
        glDeleteShader(fs);

        return prog;
    }
    GLuint make_program(std::string_view const vertex_shader_code, std::string_view const fragment_shader_code)
    {
        return make_program({ &vertex_shader_code, 1 }, {&fragment_shader_code, 1});
    }
}