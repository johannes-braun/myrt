#pragma once
#include "myrt/sfml/gl.hpp"
#include <thread>

namespace myrt::gl {
  void start(sf::RenderWindow& window) {
    window.setActive(true);
    mygl::load();
    printf_s(
      "\033[0;31mGL_VERSION:\033[0m  %s\n"
      "\033[0;31mGL_VENDOR:\033[0m   %s\n"
      "\033[0;31mGL_RENDERER:\033[0m %s\n",
      glGetString(GL_VERSION),
      glGetString(GL_VENDOR),
      glGetString(GL_RENDERER));

    glDebugMessageCallback([](GLenum source,
      GLenum type,
      std::uint32_t id,
      GLenum severity,
      std::int32_t length,
      const char* message,
      const void* userParam) {
        std::cout.write(message, length);
        std::cout << '\n';
      }, nullptr);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, false);

    ImGui::SFML::Init(window, false);
    ImGui::GetIO().Fonts->AddFontFromFileTTF((resources_dir / "alata.ttf").string().c_str(), 20);
    ImGui::SFML::UpdateFontTexture();
  }

  std::experimental::generator<frame_t> next_frame(sf::RenderWindow& window) {
    frame_t current_frame;
    auto last_frame_time = std::chrono::steady_clock::now();
    while (true) {
      auto const current_time = std::chrono::steady_clock::now();
      current_frame.delta_time = std::chrono::duration_cast<delta_time_t>(
        current_time - last_frame_time
        );
      last_frame_time = std::chrono::steady_clock::now();
      ImGui::SFML::Update(window, sf::seconds(static_cast<float>(current_frame.delta_time.count())));

      co_yield current_frame;

      glBindVertexArray(0);
      glBindSampler(0, 0);
      ImGui::SFML::Render(window);
      window.display();
      std::this_thread::yield();
    }
  }
}