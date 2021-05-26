#include "property.hpp"

namespace myrt::dyn
{
  std::shared_ptr<property_type const> simple_property(size_t blocks, char const* name, char const* load) {
    return std::make_shared<property_type>(property_type{
      .num_blocks = blocks,
      .glsl_info = {
        .type_name = name,
        .load_value = load
      }
      });
  }
}