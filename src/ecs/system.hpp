#pragma once

#include "component.hpp"
#include "flags.hpp"
#include <algorithm>
#include <execution>

namespace myrt {
enum class component_flag : uint32_t {
  optional = 1 << 0,
};
using component_flags = flags<component_flag>;

class system_base {
public:
  using duration_type = std::chrono::duration<double>;

  virtual ~system_base() = default;
  system_base() = default;
  system_base(const system_base& other) = default;
  system_base(system_base&& other) = default;
  system_base& operator=(const system_base& other) = default;
  system_base& operator=(system_base&& other) = default;

  virtual void pre_update() {}
  virtual void update(duration_type delta, component_base** components) const;
  virtual void post_update() {}

  const std::vector<id_t>& types() const;
  const std::vector<component_flags>& flags() const;

protected:
  template <typename T, typename = std::enable_if_t<std::is_convertible_v<T, component<T>>>>
  void add_component_type(component_flags flags = {}) {
    add_component_type(T::id, flags);
  }
  void add_component_type(id_t id, component_flags flags = {});

private:
  std::vector<id_t> _component_types;
  std::vector<component_flags> _component_flags;
};

using system = system_base;

class system_list {
public:
  void add(system_base& system);
  bool remove(system_base& system);

  system_base& operator[](uint32_t index);
  const system_base& operator[](uint32_t index) const;
  system_base& at(uint32_t index);
  const system_base& at(uint32_t index) const;
  size_t size() const noexcept;

  auto begin() {
    return _systems.begin();
  }
  auto end() {
    return _systems.end();
  }

private:
  std::vector<std::reference_wrapper<system_base>> _systems;
};

} // namespace myrt