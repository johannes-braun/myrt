#pragma once

#include <string>
#include <random>
#include <optional>
#include <glad/glad.h>
#include "texture_provider.hpp"
#include "scene.hpp"

namespace myrt
{
    class pathtracer
    {
    public:
        constexpr static auto max_uniform_distribution_value = 0x0ffffffu;
        constexpr static auto random_number_count = 4096;

        ~pathtracer();
        void sample_to_framebuffer(scene& scene, GLuint target_framebuffer, GLenum attachment);
        void sample_to_display(scene& scene, int width, int height);
        
        void set_projection(rnu::mat4 matrix);
        void set_view(rnu::mat4 matrix);
        void set_cubemap(GLuint cubemap, GLuint sampler);
        void set_lens_radius(float radius);
        void set_bokeh(GLuint bokeh);
        void set_focus(float focus);
        void reload_shaders();

        void invalidate_texture();
        void invalidate_counter();

        [[nodiscard]] int sample_count() const noexcept;

    private:
        void sample_internal(scene& scene, GLuint target_framebuffer, int width, int height);
        void initialize();
        void deinitialize();
        void repopulate_random_texture();

        bool m_is_initialized = false;
        std::optional<GLuint> m_program = std::nullopt;
        GLuint m_framebuffer = 0;
        GLuint m_vertex_array = 0;
        GLuint m_cubemap = 0;
        GLuint m_cubemap_sampler = 0;
        GLuint m_bokeh = 0;
        texture_provider_t m_texture_provider;

        int m_sample_counter = 0;
        float m_lens_radius = 100.f;
        float m_last_width = 0;
        float m_last_height = 0;
        float m_focus = 5.0f;
        scene const* m_last_scene = nullptr;
        std::shared_ptr<texture_t> m_last_sample_texture;
        std::shared_ptr<texture_t> m_current_sample_texture;
        std::shared_ptr<texture_t> m_random_texture;

        rnu::mat4 m_view_matrix;
        rnu::mat4 m_projection_matrix;

        std::mt19937 m_random_engine;
    };
}