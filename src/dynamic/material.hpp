#pragma once

#include <filesystem>
#include "object.hpp"

namespace myrt::dyn
{
  class material : public object
  {
  public:
    
  private:
    const char* m_sample;
    const char* m_distribution;
  };

  material load_from_file(std::filesystem::path file);
}