#include "pathtracer.hpp"
#include "bvh.hpp"
#include <sstream>
#include "utils.hpp"
#include <glm/ext.hpp>
#include <glsp/preprocess.hpp>

namespace myrt
{
    const static std::filesystem::path res_dir = "../../../res";

    namespace detail {
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

    void pathtracer::reload_shaders() {
        const auto vertex_shader = glsp::preprocess_file(res_dir / "../src/glsl/pathtracer.vert");
        const auto fragment_shader = glsp::preprocess_file(res_dir / "../src/glsl/pathtracer.frag", {}, {
            glsp::definition("MYRT_POINT_TYPE", detail::glsl::bvh_point_type),
            glsp::definition("MYRT_INDEX_TYPE", detail::glsl::bvh_index_type),
            glsp::definition("MYRT_BVH_NODE_PADDING_BYTES", std::to_string((sizeof(aligned_node_t) - sizeof(bvh_node_t)) / sizeof(detail::default_index_type))),
            glsp::definition("MYRT_BVH_NODE_TYPE_SHIFT", std::to_string(bvh_node_t::type_shift)),
            glsp::definition("MYRT_BVH_NODE_STRUCT", detail::glsl::bvh_struct_name) });

        const auto new_program = make_program(vertex_shader.contents, fragment_shader.contents);
        if (new_program)
        {
            if (m_program) glDeleteProgram(m_program.value());
            m_program = new_program;
            invalidate_counter();
        }
    }

    void pathtracer::sample_internal(scene& scene, GLuint target_framebuffer, int width, int height)
    {
        const std::uniform_int_distribution<unsigned> distribution{ 0, max_uniform_distribution_value };

        if (!m_is_initialized)
            initialize();

        scene.bind_buffers();
        if (!m_program)
            return;

        m_texture_provider.new_frame();
        repopulate_random_texture();

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
        glUseProgram(m_program.value());

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

        if (glIsTexture(m_bokeh)) {
            glUniform1i(7, true);
            glBindTextureUnit(3, m_bokeh);
        }
        else
        {
            glUniform1i(7, false);
            glBindTextureUnit(3, 0);
        }

        glUniform1f(6, m_lens_radius);
        glUniform1f(8, m_focus);
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
    void pathtracer::set_bokeh(GLuint bokeh)
    {
        if (detail::set_if_different(m_bokeh, bokeh))
            invalidate_counter();
    }
    void pathtracer::set_focus(float focus)
    {
        if (detail::set_if_different(m_focus, focus))
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
    void pathtracer::set_lens_radius(float radius)
    {
        if (detail::set_if_different(m_lens_radius, radius))
            invalidate_counter();
    }
    void pathtracer::initialize()
    {
        m_is_initialized = true;
        reload_shaders();
        glCreateFramebuffers(1, &m_framebuffer);
        glCreateVertexArrays(1, &m_vertex_array);

        m_random_texture = m_texture_provider.get(GL_TEXTURE_1D, GL_R32F, int(random_number_count), 1);
        m_random_texture->lock();
    }
    void pathtracer::repopulate_random_texture() {
        std::vector<float> random_texture_data(random_number_count);
        std::uniform_real_distribution<float> distribution(0.f, 1.f);
        std::generate(random_texture_data.begin(), random_texture_data.end(), [&] {return distribution(m_random_engine); });
        glTextureSubImage1D(m_random_texture->id(), 0, 0, GLsizei(random_texture_data.size()), GL_RED, GL_FLOAT, random_texture_data.data());
    }
    void pathtracer::deinitialize()
    {
        if(m_program) glDeleteProgram(m_program.value());
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