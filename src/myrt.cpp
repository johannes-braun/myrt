#include "pathtracer/bvh.hpp"
#include "pathtracer/utils.hpp"
#include "pathtracer/texture_provider.hpp"
#include "pathtracer/pathtracer.hpp"
#include "pathtracer/scene.hpp"
#include "cube.hpp"
#include "teapot_low.h"
#include <imgui-SFML.h>

#include <iostream>
#include <sstream>
#include <thread>
#include <filesystem>
#include <random>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Window.hpp>
#include <glad/glad.h>
#include <glm/ext.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <imgui.h>

std::experimental::generator<std::reference_wrapper<sf::Event>> co_poll(sf::Window& window)
{
    sf::Event event{};
    while (window.pollEvent(event))
        co_yield event;
}

const static std::filesystem::path res_dir = "../../../res";

std::pair<GLuint, GLuint> load_cubemap();

int main(int argc, char** argv)
{
    sf::ContextSettings settings;
    settings.majorVersion = 4;
    settings.minorVersion = 6;
    settings.attributeFlags |= sf::ContextSettings::Debug;

    sf::RenderWindow window(sf::VideoMode(1280, 720), "MyRT", sf::Style::Default, settings);
    window.setActive(false);

    std::atomic_bool close = false;
    std::thread render_thread([&] {
        window.setActive(true);
        gladLoadGL();
        printf_s(
            "\033[0;31mGL_VERSION:\033[0m  %s\n"
            "\033[0;31mGL_VENDOR:\033[0m   %s\n"
            "\033[0;31mGL_RENDERER:\033[0m %s\n",
            glGetString(GL_VERSION),
            glGetString(GL_VENDOR),
            glGetString(GL_RENDERER));

        glDebugMessageCallback([](GLenum source,
            GLenum type,
            GLuint id,
            GLenum severity,
            GLsizei length,
            const GLchar* message,
            const void* userParam) {
                std::cout.write(message, length);
                std::cout << '\n';
            }, nullptr);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

        myrt::scene scene;
        auto teapot = scene.push_geometry(teapot_low::indices,
            { (glm::vec3*)teapot_low::vertices, teapot_low::num_points },
            { (glm::vec3*)teapot_low::normals, teapot_low::num_points });
        auto cube = scene.push_geometry(cube::indices,
            { (glm::vec3*)cube::vertices, cube::num_points },
            { (glm::vec3*)cube::normals, cube::num_points });

        std::vector<myrt::geometric_object> objects;
        std::minstd_rand rng(1);
        for (int i = -2; i < 2; ++i)
            for (int j = -2; j < 2; ++j)
            {
                auto& obj = objects.emplace_back();
                obj.geometry = (cube && (i + (j % 2)) % 2 == 0) ? cube : teapot;
                obj.transformation = glm::translate(glm::mat4(1.0), 2.2f * glm::vec3(i, 0.1 * i, j))
                    * glm::rotate(glm::mat4(1.0f), glm::radians(float(rng())), normalize(glm::vec3(glm::uvec3(rng(), rng(), rng() + 1))))
                    * glm::scale(glm::mat4(1.0), glm::vec3(1, 0.1f + 2.f * (rng() / float(rng.max())), 1));
            }

        auto [cubemap, cube_sampler] = load_cubemap();
        bool cubemap_enabled = false;

        ImGui::SFML::Init(window, false);
        ImGui::GetIO().Fonts->AddFontFromFileTTF((res_dir / "alata.ttf").string().c_str(), 20);
        ImGui::SFML::UpdateFontTexture();

        auto proj = glm::perspectiveFov(glm::radians(60.f), float(window.getSize().x), float(window.getSize().y), 0.01f, 1000.f);
        auto view = glm::lookAt(glm::vec3(5, 5.5, 4.5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

        myrt::pathtracer pathtracer;

        sf::Clock delta_time;
        while (!close)
        {
            sf::Time delta = delta_time.restart();
            ImGui::SFML::Update(window, delta);

            for (auto const& obj : objects)
                obj.enqueue();

            pathtracer.set_view(view);
            pathtracer.set_projection(proj);
            pathtracer.sample_to_display(scene, window.getSize().x, window.getSize().y);

            ImGui::Begin("Settings");
            ImGui::Text("Samples: %d (%.00f sps)", pathtracer.sample_count(), 1.f / delta.asSeconds());
            if (ImGui::Button("Restart Sampling"))
                pathtracer.invalidate_counter();
            if (ImGui::Button("Eradicate (default) Cube"))
            {
                scene.erase_geometry_direct(cube);
                objects.erase(std::remove_if(objects.begin(), objects.end(), [&](const auto& obj) { return obj.geometry == cube; }), objects.end());
                cube.reset();
                pathtracer.invalidate_counter();
            }
            if (ImGui::Checkbox("Enable Cubemap", &cubemap_enabled))
            {
                if (cubemap_enabled)
                    pathtracer.set_cubemap(cubemap, cube_sampler);
                else
                    pathtracer.set_cubemap(0, 0);
            }

            ImGui::End();

            glBindVertexArray(0);
            glBindSampler(0, 0);
            ImGui::SFML::Render(window);
            window.display();
            std::this_thread::yield();
        }
        });

    while (!close)
    {
        for (auto const& event : co_poll(window))
        {
            ImGui::SFML::ProcessEvent(event);
            if (event.get().type == sf::Event::Closed)
            {
                close = true;
            }
        }
        std::this_thread::yield();
    }
    render_thread.join();
    window.close();
}

std::pair<GLuint, GLuint> load_cubemap()
{
    uint32_t cubemap;
    uint32_t cube_sampler;
    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &cubemap);
    glCreateSamplers(1, &cube_sampler);
    glSamplerParameterf(cube_sampler, GL_TEXTURE_MAX_ANISOTROPY, 16.f);
    glSamplerParameteri(cube_sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(cube_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(cube_sampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(cube_sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(cube_sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(cube_sampler, GL_TEXTURE_CUBE_MAP_SEAMLESS, true);

    int w, h, c;
    // face 1
    float* img = stbi_loadf((res_dir / "whipple_creek/posx.hdr").string().c_str(), &w, &h, &c, 3);
    const int num_mips = static_cast<int>(1 + floor(log(float(std::max(w, h)))));
    glTextureStorage3D(cubemap, num_mips, GL_RGB16F, w, h, 6);
    glTextureSubImage3D(cubemap, 0, 0, 0, 0, w, h, 1, GL_RGB, GL_FLOAT, img);
    stbi_image_free(img);

    img = stbi_loadf((res_dir / "whipple_creek/negx.hdr").string().c_str(), &w, &h, &c, 3);
    glTextureSubImage3D(cubemap, 0, 0, 0, 1, w, h, 1, GL_RGB, GL_FLOAT, img);
    stbi_image_free(img);

    img = stbi_loadf((res_dir / "whipple_creek/posy.hdr").string().c_str(), &w, &h, &c, 3);
    glTextureSubImage3D(cubemap, 0, 0, 0, 2, w, h, 1, GL_RGB, GL_FLOAT, img);
    stbi_image_free(img);

    img = stbi_loadf((res_dir / "whipple_creek/negy.hdr").string().c_str(), &w, &h, &c, 3);
    glTextureSubImage3D(cubemap, 0, 0, 0, 3, w, h, 1, GL_RGB, GL_FLOAT, img);
    stbi_image_free(img);

    img = stbi_loadf((res_dir / "whipple_creek/posz.hdr").string().c_str(), &w, &h, &c, 3);
    glTextureSubImage3D(cubemap, 0, 0, 0, 4, w, h, 1, GL_RGB, GL_FLOAT, img);
    stbi_image_free(img);

    img = stbi_loadf((res_dir / "whipple_creek/negz.hdr").string().c_str(), &w, &h, &c, 3);
    glTextureSubImage3D(cubemap, 0, 0, 0, 5, w, h, 1, GL_RGB, GL_FLOAT, img);
    stbi_image_free(img);
    glGenerateTextureMipmap(cubemap);
    uint32_t view{ 0 };
    glGenTextures(1, &view);
    glTextureView(view, GL_TEXTURE_CUBE_MAP, cubemap, GL_RGB16F, 0, num_mips, 0, 6);

    return { view, cube_sampler };
}