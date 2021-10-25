#pragma once

#include <vector>
#include "texture_provider.hpp"

namespace myrt {
class texture_array {
public:


private:
  std::vector<std::shared_ptr<texture_t>> m_textures;
};
} // namespace myrt