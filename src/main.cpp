#include <rnu/camera.hpp>

#include "sfml.hpp"
#include "gl.hpp"

sf::RenderWindow* window;

void render_function(std::stop_token stop_token, sf::RenderWindow* window);

int main(int argc, char** argv) {
  sf::ContextSettings settings;
  settings.majorVersion = 4;
  settings.minorVersion = 6;
  settings.attributeFlags |= sf::ContextSettings::Debug;

  sf::RenderWindow window(sf::VideoMode(1600, 1200), "MyRT", sf::Style::Default, settings);
  window.setActive(false);

  std::jthread render_thread(&render_function, &window);

  for (auto event : myrt::sfml::poll_event(window)) {

  }
  render_thread.request_stop();
}

void render_function(std::stop_token stop_token, sf::RenderWindow* window)
{
  rnu::cameraf camera(rnu::vec3{ 0.0f, 0.0f, -15.f });

  for (auto frame : myrt::gl::next_frame(*window)) {
    if (stop_token.stop_requested())
      return;

    auto mat = camera.matrix();
  }
}