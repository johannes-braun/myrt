#include "pathtracer.hpp"
#include "bvh.hpp"
#include <sstream>
#include "utils.hpp"
#include <glm/ext.hpp>

namespace myrt
{
    namespace detail {
        constexpr std::string_view pathtracer_fragment_shader_base = {
#include "pathtracer.frag"
        };

        template<typename T, typename K>
        [[nodiscard]] bool set_if_different(T& t, K&& new_value)
        {
            if (t != new_value)
            {
                t = static_cast<T>(std::forward<K>(new_value));
                return true;
            }
            return false;
        }

        [[nodiscard]] GLuint get_framebuffer_texture(GLuint fbo, GLenum attachment)
        {
            GLint texture{};
            glGetNamedFramebufferAttachmentParameteriv(fbo, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &texture);
            return static_cast<GLuint>(texture);
        }
        [[nodiscard]] std::pair<GLint, GLint> get_framebuffer_size(GLuint fbo, GLenum attachment)
        {
            const auto texture = get_framebuffer_texture(fbo, attachment);
            std::pair<GLint, GLint> size;
            glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_WIDTH, &size.first);
            glGetTextureLevelParameteriv(texture, 0, GL_TEXTURE_HEIGHT, &size.second);
            return size;
        }
    }

    constexpr std::string_view pathtracer_vertex_shader = {
#include "pathtracer.vert"
    };

    const std::string pathtracer_fragment_shader = (std::stringstream() <<
        "#version 460 core\n" <<
        glsl::bvh_definitions_code("bvh_traverse", "visit_triangle") <<
        glsl::bvh_definitions_code("bvh_traverse_global", "visit_object_aabb") <<
        glsl::intersect_triangle_code() <<
        detail::pathtracer_fragment_shader_base <<
        glsl::bvh_code("bvh_traverse", "visit_triangle", "bvh_nodes", "bvh_indices") <<
        glsl::bvh_code("bvh_traverse_global", "visit_object_aabb", "global_bvh_nodes", "global_bvh_indices")).str();

    pathtracer::~pathtracer()
    {
        if (m_is_initialized)
            deinitialize();
    }

    void pathtracer::sample_to_display(scene& scene, int width, int height)
    {
        sample_internal(scene, 0, width, height);
    }
    void pathtracer::sample_to_framebuffer(scene& scene, GLuint target_framebuffer, GLenum attachment)
    {
        const auto [width, height] = detail::get_framebuffer_size(target_framebuffer, attachment);
        sample_internal(scene, target_framebuffer, width, height);
    }

    void pathtracer::sample_internal(scene& scene, GLuint target_framebuffer, int width, int height)
    {
        const std::uniform_int_distribution<unsigned> distribution{ 0, max_uniform_distribution_value };
        m_texture_provider.new_frame();

        if (!m_is_initialized)
            initialize();

        if (detail::set_if_different(m_last_scene, &scene))
            invalidate_counter();
        if (detail::set_if_different(m_last_width, width) ||
            detail::set_if_different(m_last_height, height))
        {
            invalidate_texture();
            invalidate_counter();
        }

        glViewport(0, 0, width, height);
        if (!m_current_sample_texture)
            m_current_sample_texture = m_texture_provider.get(GL_TEXTURE_2D, GL_RGBA32F, width, height, 1);
        if (!m_last_sample_texture)
            m_last_sample_texture = m_texture_provider.get(GL_TEXTURE_2D, GL_RGBA32F, width, height, 1);
        glNamedFramebufferTexture(m_framebuffer, GL_COLOR_ATTACHMENT0, m_current_sample_texture->id(), 0);

        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
        glUseProgram(m_program);
        scene.bind_buffers();
        if (glIsTexture(m_cubemap) && glIsSampler(m_cubemap_sampler)) {
            glUniform1i(5, true);
            glBindSampler(0, m_cubemap_sampler);
            glBindTextureUnit(0, m_cubemap);
        }
        else
        {
            glUniform1i(5, false);
            glBindSampler(0, 0);
            glBindTextureUnit(0, 0);
        }
        glBindTextureUnit(1, m_random_texture->id());
        glBindTextureUnit(2, m_last_sample_texture->id());
        glBindVertexArray(m_vertex_array);
        glUniformMatrix4fv(0, 1, false, value_ptr(inverse(m_projection_matrix)));
        glUniformMatrix4fv(1, 1, false, value_ptr(inverse(m_view_matrix)));
        glUniform2f(2, GLfloat(width), GLfloat(height));
        glUniform1ui(3, distribution(m_random_engine));
        glUniform1i(4, m_sample_counter++);

        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        std::swap(m_last_sample_texture, m_current_sample_texture);

        glBlitNamedFramebuffer(m_framebuffer, 0, 0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    void pathtracer::set_projection(glm::mat4 matrix)
    {
        if (detail::set_if_different(m_projection_matrix, matrix))
            invalidate_counter();
    }
    void pathtracer::set_view(glm::mat4 matrix)
    {
        if (detail::set_if_different(m_view_matrix, matrix))
            invalidate_counter();
    }
    void pathtracer::set_cubemap(GLuint cubemap, GLuint sampler)
    {
        bool cubemap_changed = detail::set_if_different(m_cubemap, cubemap);
        bool sampler_changed = detail::set_if_different(m_cubemap_sampler, sampler);
        if (cubemap_changed || sampler_changed)
            invalidate_counter();
    }
    void pathtracer::initialize()
    {
        m_is_initialized = true;
        m_program = make_program(pathtracer_vertex_shader, pathtracer_fragment_shader);
        glCreateFramebuffers(1, &m_framebuffer);
        glCreateVertexArrays(1, &m_vertex_array);

        std::vector<float> random_texture_data(random_number_count);
        std::uniform_real_distribution<float> distribution(0.f, 1.f);
        std::generate(random_texture_data.begin(), random_texture_data.end(), [&] {return distribution(m_random_engine); });

        m_random_texture = m_texture_provider.get(GL_TEXTURE_1D, GL_R32F, int(random_texture_data.size()), 1);
        m_random_texture->lock();
        glTextureSubImage1D(m_random_texture->id(), 0, 0, GLsizei(random_texture_data.size()), GL_RED, GL_FLOAT, random_texture_data.data());
    }

    void pathtracer::deinitialize()
    {
        glDeleteProgram(m_program);
        glDeleteFramebuffers(1, &m_framebuffer);
        glDeleteVertexArrays(1, &m_vertex_array);
        m_random_texture->unlock();
        m_current_sample_texture.reset();
        m_last_sample_texture.reset();
        m_is_initialized = false;
    }

    void pathtracer::invalidate_texture()
    {
        invalidate_counter();
        m_current_sample_texture.reset();
        m_last_sample_texture.reset();
    }
    void pathtracer::invalidate_counter()
    {
        m_sample_counter = 0;
    }
    int pathtracer::sample_count() const noexcept
    {
        return m_sample_counter;
    }
}