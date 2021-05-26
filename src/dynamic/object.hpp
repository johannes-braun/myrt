#pragma once

#include <vector>
#include "property.hpp"
#include <string>
#include <map>
#include <optional>

namespace myrt::dyn
{
  class object
  {
  public:
    void emplace_property(std::string name, std::shared_ptr<property_type const> type);
    void erase_property(std::string const& name);

    void set(std::string const& property_name, void const* data, size_t data_size);
    void get(std::string const& property_name, void* dst);

    template<typename T>
    void set(std::string const& property_name, T&& data);
    template<typename T>
    std::optional<T> get(std::string const& property_name);

    bool is_dirty() const;
    void set_dirty(bool dirty);

  private:
    struct property
    {
      std::shared_ptr<property_type const> type;
      ptrdiff_t buffer_offset;
    };

    bool m_dirty = false;
    std::map<std::string, property> m_properties;
    std::vector<float> m_buffer;
  };

  template<typename T>
  inline void object::set(std::string const& property_name, T&& data)
  {
    set(property_name, &data, sizeof(data));
  }

  template<typename T>
  inline std::optional<T> object::get(std::string const& property_name)
  {
    if (auto const it = m_properties.find(property_name); it != m_properties.end())
    {
      alignas(alignof(T)) std::byte result[sizeof(T)];
      it->second.type->get(*(it->second.type), result, m_buffer.data() + it->second.buffer_offset);
      return reinterpret_cast<T const&>(*result);
    }
    return std::nullopt;
  }
}