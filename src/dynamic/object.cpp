#include "object.hpp"

namespace myrt::dyn
{
  void object::emplace_property(std::string name, std::shared_ptr<property_type const> type)
  {
    m_properties[std::move(name)] = property{ .type = std::move(type), .buffer_offset = static_cast<ptrdiff_t>(m_buffer.size()) };
    m_buffer.resize(m_buffer.size() + type->num_blocks);
    m_dirty = true;
  }
  void object::erase_property(std::string const& name)
  {
    if (auto const it = m_properties.find(name); it != m_properties.end())
    {
      m_buffer.erase(m_buffer.begin() + it->second.buffer_offset, m_buffer.begin() + it->second.type->num_blocks);
      auto const base_offset = it->second.buffer_offset;
      auto const base_size = it->second.type->num_blocks;
      m_properties.erase(name);
      for (auto& prop : m_properties) {
        if (prop.second.buffer_offset >= base_offset)
        {
          prop.second.buffer_offset -= base_size;
        }
      }
      m_dirty = true;
    }
  }
  void object::set(std::string const& property_name, void const* data, size_t data_size)
  {
    if (auto const it = m_properties.find(property_name); it != m_properties.end())
    {
      it->second.type->set(*(it->second.type), data, m_buffer.data() + it->second.buffer_offset);
      m_dirty = true;
    }
  }
  void object::get(std::string const& property_name, void* dst)
  {
    if (auto const it = m_properties.find(property_name); it != m_properties.end())
      it->second.type->get(*(it->second.type), dst, m_buffer.data() + it->second.buffer_offset);
  }
  inline bool object::is_dirty() const { return m_dirty; }
  inline void object::set_dirty(bool dirty) { m_dirty = dirty; }
}