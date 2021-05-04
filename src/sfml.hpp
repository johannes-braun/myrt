#pragma once
#include <experimental/generator>
#include <memory>
#include <thread>
#include <imgui-SFML.h>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window.hpp>

namespace myrt::sfml {
  std::experimental::generator<std::reference_wrapper<sf::Event>> poll_event(sf::Window& window)
  {
    bool do_close = false;
    sf::Event event{};
    while (!do_close) {
      auto const begin = std::chrono::steady_clock::now();
      if (window.pollEvent(event)) {
        ImGui::SFML::ProcessEvent(event);
          if (event.type == sf::Event::Closed)
            do_close = true;
          co_yield event;
      }
      std::this_thread::yield();
      std::this_thread::sleep_until(begin + std::chrono::milliseconds(10));
    }
  }
}