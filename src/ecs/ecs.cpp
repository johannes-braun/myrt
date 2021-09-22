#include "ecs.hpp"
#include <algorithm>

namespace myrt {
entity::operator entity_handle() const noexcept {
  return _handle;
}

entity::operator bool() const noexcept {
  return _ecs && _handle;
}

entity::entity(ecs* e, entity_handle hnd) : _ecs(e), _handle(hnd) {}

void entity_deleter::operator()(entity* e) const {
  e->_ecs->delete_entity(*e);
}

ecs::~ecs() {
  for (auto&& cp : _components) {
    const size_t size = component_base::type_size(cp.first);
    const auto deleter = component_base::get_deleter(cp.first);

    for (auto i = 0ull; i < cp.second.size(); i += size) deleter(reinterpret_cast<component_base*>(&cp.second[i]));
  }

  for (auto&& e : _entities) delete e;
}

void ecs::add_listener(listener& l) {
  _listeners.push_back(&l);
}

entity ecs::create_entity(const component_base** components, const id_t* component_ids, size_t count) {
  const auto e = new indexed_entity();
  auto hnd = static_cast<entity_handle>(e);

  for (auto i = 0u; i < count; ++i) {
    if (!component_base::is_valid(component_ids[i])) {
      delete e;
      return entity{this, null_entity};
    }
    add_component_impl(hnd, component_ids[i], components[i]);
  }

  e->first = static_cast<uint32_t>(_entities.size());
  _entities.push_back(e);

  const entity result{this, hnd};
  for (auto& l : _listeners) {
    if (![&] {
          for (const auto id : l->component_ids())
            if (![&] {
                  for (const auto& x : as_entity(hnd))
                    if (id == x.first)
                      return true;
                  return false;
                }())
              return false;
          return true;
        }())
      break;
    l->on_add(result);
  }

  return result;
}

void ecs::delete_entity(entity handle) {
  auto& e = as_entity(handle._handle);

  for (auto& l : _listeners) {
    if (![&] {
          for (const auto id : l->component_ids())
            if (![&] {
                  for (const auto& x : e)
                    if (id == x.first)
                      return true;
                  return false;
                }())
              return false;
          return true;
        }())
      break;
    l->on_remove(handle);
  }

  for (auto&& comp : e) delete_component(comp.first, comp.second);
  const auto dst_idx = index_of(handle._handle);
  delete _entities[dst_idx];
  _entities[dst_idx] = _entities[_entities.size() - 1];
  _entities[dst_idx]->first = dst_idx;
  _entities.pop_back();
}

unique_entity ecs::create_entity_unique(const component_base** components, const id_t* component_ids, size_t count) {
  return unique_entity(new entity(create_entity(components, component_ids, count)));
}

shared_entity ecs::create_entity_shared(const component_base** components, const id_t* component_ids, size_t count) {
  return shared_entity(new entity(create_entity(components, component_ids, count)), entity_deleter{});
}

component_base* ecs::get_component(entity_handle handle, id_t cid) {
  return get_component_impl(handle, _components.at(cid), cid);
}

void ecs::update(duration_type delta, system_list& list) {
  std::vector<component_base*> multi_components;
  std::vector<std::vector<std::byte>*> component_arrays;

  std::for_each(list.begin(), list.end(), [&](std::reference_wrapper<system_base>& item) {
    item.get().pre_update();
    const auto& component_types = item.get().types();
    if (component_types.size() == 1) {
      const auto size = component_base::type_size(component_types[0]);
      auto& arr = _components[component_types[0]];
      for (auto ci = 0ull; ci < arr.size(); ci += size) {
        auto* c = reinterpret_cast<component_base*>(&arr[ci]);
        item.get().update(delta, &c);
      }
    } else {
      update_multi_system(item.get(), delta, component_types, multi_components, component_arrays);
    }
    item.get().post_update();
  });
}

void ecs::update(double delta_seconds, system_list& list) {
  update(duration_type(delta_seconds), list);
}

void ecs::delete_component(id_t id, size_t index) {
  auto& component_arr = _components[id];
  const auto deleter = component_base::get_deleter(id);
  const auto size = component_base::type_size(id);
  auto source_index = component_arr.size() - size;

  const auto dst = reinterpret_cast<component_base*>(&component_arr[index]);
  const auto src = reinterpret_cast<component_base*>(&component_arr[source_index]);
  deleter(dst);

  if (index == source_index) {
    component_arr.resize(source_index);
    return;
  }

  memcpy(dst, src, size);

  auto& components = as_entity(src->entity);
  if (const auto it = std::find_if(std::execution::seq, components.begin(), components.end(),
          [&](const auto& p) { return p.first == id && p.second == source_index; });
      it != components.end()) {
    it->second = static_cast<uint32_t>(index);
  }
  component_arr.resize(source_index);
}

bool ecs::remove_component_impl(entity_handle e, id_t component_id) {
  auto& components = as_entity(e);
  if (const auto it = std::find_if(std::execution::seq, components.begin(), components.end(),
          [&](const auto& p) { return p.first == component_id; });
      it != components.end()) {
    for (auto& l : _listeners)
      for (const auto id : l->component_ids())
        if (id == component_id) {
          l->on_remove_component({this, e}, id);
          break;
        }

    const auto src_index = components.size() - 1;
    const auto dst_index = std::distance(components.begin(), it);
    delete_component(it->first, it->second);
    components[dst_index] = components[src_index];
    components.pop_back();
    return true;
  }
  return false;
}

void ecs::add_component_impl(entity_handle e, id_t component_id, const component_base* component) {
  auto ent = as_entity_ptr(e);
  const auto create = component_base::get_creator(component_id);
  ent->second.emplace_back(component_id, static_cast<uint32_t>(create(_components[component_id], e, component)));
  for (auto& l : _listeners)
    for (const auto id : l->component_ids())
      if (id == component_id) {
        l->on_remove_component({this, e}, id);
        break;
      }
}

component_base* ecs::get_component_impl(entity_handle e, std::vector<std::byte>& carr, id_t component_id) {
  auto& components = as_entity(e);
  if (const auto it = std::find_if(std::execution::seq, components.begin(), components.end(),
          [&](const auto& p) { return p.first == component_id; });
      it != components.end()) {
    return reinterpret_cast<component_base*>(&carr[it->second]);
  }
  return nullptr;
}

void ecs::update_multi_system(system_base& system, duration_type delta, const std::vector<id_t>& types,
    std::vector<component_base*>& components, std::vector<std::vector<std::byte>*>& component_arrays) {
  const auto& system_flags = system.flags();

  components.resize(std::max(types.size(), components.size()));
  component_arrays.resize(std::max(component_arrays.size(), components.size()));
  for (auto i = 0ull; i < types.size(); ++i) component_arrays[i] = &_components[types[i]];

  const auto min_index = [&]() -> ptrdiff_t {
    const auto d = std::distance(types.begin(),
        std::min_element(std::execution::seq, types.begin(), types.end(), [&](const id_t& a, const id_t& b) {
          const auto sa = component_base::type_size(a);
          const auto sb = component_base::type_size(b);
          const auto a_opt =
              (system_flags[std::distance(types.data(), &a)] & component_flag::optional) == component_flag::optional;
          const auto b_opt =
              (system_flags[std::distance(types.data(), &b)] & component_flag::optional) == component_flag::optional;

          if (a_opt)
            return false;
          if (b_opt)
            return true;
          return (_components.at(a).size() / sa < _components.at(b).size() / sb);
        }));
    if (static_cast<unsigned long long>(d) == types.size())
      return 0ll;
    return d;
  }();

  const auto size = component_base::type_size(types[min_index]);
  for (auto ci = 0ull; ci < component_arrays[min_index]->size(); ci += size) {
    components[min_index] = reinterpret_cast<component_base*>(&(*component_arrays[min_index])[ci]);
    const auto& parent_entity = components[min_index]->entity;

    if ([&]() -> bool {
          for (auto j = 0ll; j < static_cast<int64_t>(types.size()); ++j) {
            if (j == min_index)
              continue;
            components[j] = get_component_impl(parent_entity, *component_arrays[j], types[j]);
            if (!components[j] && (system_flags[j] & component_flag::optional) != component_flag::optional)
              return false;
          }
          return true;
        }())
      system.update(delta, components.data());
  }
}

indexed_entity* ecs::as_entity_ptr(entity_handle handle) {
  return static_cast<indexed_entity*>(handle);
}

uint32_t ecs::index_of(entity_handle handle) {
  return as_entity_ptr(handle)->first;
}

entity_info& ecs::as_entity(entity_handle handle) {
  return as_entity_ptr(handle)->second;
}
} // namespace myrt