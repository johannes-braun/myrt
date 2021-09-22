#pragma once

#include "entity.hpp"

namespace myrt {
class listener {
public:
  listener() = default;
  listener(const listener& other) = default;
  listener(listener&& other) = default;
  listener& operator=(const listener& other) = default;
  listener& operator=(listener&& other) = default;

  virtual ~listener() = default;
  virtual void on_add(entity e) {}
  virtual void on_remove(entity e) {}
  virtual void on_add_component(entity e, id_t id) {}
  virtual void on_remove_component(entity e, id_t id) {}

  const std::vector<id_t>& component_ids() const noexcept {
    return _component_ids;
  }

protected:
  void add_component_id(id_t id) {
    _component_ids.push_back(id);
  }

private:
  std::vector<id_t> _component_ids;
};
} // namespace myrt