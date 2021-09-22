#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace myrt {
enum class id_t : uint64_t {};
struct component_base;
using entity_handle = void*;
constexpr const entity_handle null_entity = nullptr;
using component_creator_fun = ptrdiff_t (*)(
    std::vector<std::byte>& memory, entity_handle entity, const component_base* base_component);
using component_deleter_fun = void (*)(component_base* base_component);

struct component_base {
  entity_handle entity = null_entity;

  static auto get_creator(id_t id) {
    return std::get<component_creator_fun>(types()[static_cast<size_t>(id)]);
  }
  static auto get_deleter(id_t id) {
    return std::get<component_deleter_fun>(types()[static_cast<size_t>(id)]);
  }
  static size_t type_size(id_t id) {
    return std::get<size_t>(types()[static_cast<size_t>(id)]);
  }
  static bool is_valid(id_t id) {
    return static_cast<size_t>(id) < types().size();
  }

  template <typename T> T& as() {
    return static_cast<std::decay_t<T>&>(*this);
  }
  template <typename T> const T& as() const {
    return static_cast<const std::decay_t<T>&>(*this);
  }
  template <typename T> T* as_ptr() {
    return static_cast<std::decay_t<T>*>(this);
  }
  template <typename T> const T* as_ptr() const {
    return static_cast<const std::decay_t<T>*>(this);
  }

protected:
  static id_t register_type(component_creator_fun create, component_deleter_fun deleter, size_t size) {
    const id_t id{types().size()};
    types().emplace_back(create, deleter, size);
    return id;
  }

private:
  static auto types() -> std::vector<std::tuple<component_creator_fun, component_deleter_fun, size_t>>& {
    static std::vector<std::tuple<component_creator_fun, component_deleter_fun, size_t>> t;
    return t;
  }
};

template <typename C>
ptrdiff_t create(std::vector<std::byte>& memory, entity_handle entity, const component_base* base_component) {
  const ptrdiff_t ptr = memory.size();
  memory.resize(memory.size() + C::size);
  C* component = new (&memory[ptr]) C(*static_cast<const C*>(base_component));
  component->entity = entity;
  return ptr;
}

template <typename C> void destroy(component_base* base_component) {
  C* component = static_cast<C*>(base_component);
  component->~C();
}

template <typename T> struct component : component_base {
  using type = T;

  static const size_t size;
  static const id_t id;
  static const component_creator_fun creator;
  static const component_deleter_fun deleter;
};

template <typename T> const size_t component<T>::size = sizeof(T);
template <typename T> const id_t component<T>::id = register_type(create<T>, destroy<T>, size);
template <typename T> const component_creator_fun component<T>::creator = create<T>;
template <typename T> const component_deleter_fun component<T>::deleter = destroy<T>;

template <typename T> struct simple_component : component<simple_component<T>>, T { using T::T; };

} // namespace myrt