#pragma once

#include "component.hpp"
#include <memory>
#include <vector>

namespace myrt {
using entity_info = std::vector<std::pair<id_t, uint32_t>>;
using indexed_entity = std::pair<uint32_t, entity_info>;

namespace traits {
  template <typename T> struct is_component : std::is_convertible<std::decay_t<T>&, component_base&> {};

  template <typename T> struct is_component<component<T>> : std::true_type {};

  template <typename T>
  concept component_type = is_component<T>::value;

  //template <typename T> using enable_if_component_t = std::enable_if_t<is_component<T>::value>;
} // namespace traits

class ecs;
class listener;
class entity {
  friend class ecs;
  friend struct entity_deleter;
  friend class listener;

public:
  entity() = default;

  template <traits::component_type... Component> void add(const Component&... component);
  template <traits::component_type Component, traits::component_type... Components> bool remove();
  template <traits::component_type Component> std::decay_t<Component>* get();
  template <traits::component_type Component> const std::decay_t<Component>* get() const;

  operator entity_handle() const noexcept;
  operator bool() const noexcept;

private:
  entity(ecs* e, entity_handle hnd);

  ecs* _ecs = nullptr;
  entity_handle _handle = nullptr;
};

struct entity_deleter {
  void operator()(entity* e) const;
};
using unique_entity = std::unique_ptr<entity, entity_deleter>;
using shared_entity = std::shared_ptr<entity>;
} // namespace myrt
