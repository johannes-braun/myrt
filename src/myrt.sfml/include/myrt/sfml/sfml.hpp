#pragma once
#include <experimental/generator>
#include <imgui-SFML.h>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window.hpp>

namespace myrt::sfml {
  std::experimental::generator<std::reference_wrapper<sf::Event>> poll_event(sf::Window& window);
}