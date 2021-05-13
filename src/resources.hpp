#pragma once
#include <filesystem>

namespace myrt {
  const inline static std::filesystem::path resources_dir = 
    std::filesystem::exists("./res") ? "./res" : "../../../res";
}