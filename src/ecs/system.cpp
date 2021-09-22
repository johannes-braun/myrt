#include "system.hpp"

namespace myrt {
void system_base::add_component_type(id_t id, component_flags flags) {
  _component_types.push_back(id);
  _component_flags.push_back(flags);
}

void system_base::update(duration_type delta, component_base** components) const {}

const std::vector<id_t>& system_base::types() const {
  return _component_types;
}

const std::vector<component_flags>& system_base::flags() const {
  return _component_flags;
}

void system_list::add(system_base& system) {
  _systems.push_back(std::ref(system));
}

bool system_list::remove(system_base& system) {
  if (const auto it = std::find_if(
          std::execution::par, _systems.begin(), _systems.end(), [&](auto& ref) { return &ref.get() == &system; });
      it != _systems.end()) {
    _systems.erase(it);
    return true;
  }
  return false;
}

system_base& system_list::operator[](uint32_t index) {
  return _systems[index];
}

const system_base& system_list::operator[](uint32_t index) const {
  return _systems[index];
}

system_base& system_list::at(uint32_t index) {
  return _systems[index];
}

const system_base& system_list::at(uint32_t index) const {
  return _systems[index];
}

size_t system_list::size() const noexcept {
  return _systems.size();
}
} // namespace myrt