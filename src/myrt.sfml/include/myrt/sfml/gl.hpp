#pragma once
#include <SFML/Graphics/RenderWindow.hpp>
#include <imgui-SFML.h>
#include <imgui.h>
#include <mygl/mygl.hpp>
#include <experimental/generator>
#include <chrono>
#include <optional>
#include <iostream>
#include <filesystem>

namespace myrt::gl {
  const inline static std::filesystem::path resources_dir =
    std::filesystem::exists("./res") ? "./res" : "C:/Users/johan/Documents/Projekte/myrt/res/";

  using delta_time_t = std::chrono::duration<double>;

  struct frame_t {
    delta_time_t delta_time;
  };

  void start(sf::RenderWindow& window);

  std::experimental::generator<frame_t> next_frame(sf::RenderWindow& window);
}