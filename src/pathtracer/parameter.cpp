#include "parameter.hpp"

namespace myrt
{
  parameter_type::parameter_type(std::string type_identifier, std::string type_name, int buffer_blocks, std::string buffer_load)
    : type_identifier(type_identifier), type_name(type_name), buffer_blocks(buffer_blocks) {
    function = type_name + " _r" + type_identifier + "(";
    for (int i = 0; i < buffer_blocks; ++i)
    {
      if (i != 0)
        function += ",";
      function += "float in_block" + std::to_string(i);
    }
    function += "){" + buffer_load + "\n;}";
  }
  parameter::parameter(std::shared_ptr<parameter_type> type)
    : type(std::move(type)) {
    _value_links.resize(this->type->buffer_blocks);
    _default_value.resize(this->type->buffer_blocks);
  }
  parameter_link parameter::link_value_block(size_t block, std::shared_ptr<parameter> other, size_t other_block) {
    parameter_link last = _value_links[block];
    _value_links[block].other = std::move(other);
    _value_links[block].block = other_block;
    return last;
  }
  parameter_link parameter::unlink_value_block(size_t block) {
    parameter_link last = _value_links[block];
    _value_links[block].other.reset();
    _value_links[block].block = 0;
    return last;
  }
  parameter_link const& parameter::get_link(size_t block) const {
    return _value_links[block];
  }
  std::vector<float> const& parameter::get_default_value() const {
    return _default_value;
  }
  void parameter::set_default_value(std::span<float const> data) {
    if (data.size() != type->buffer_blocks)
      throw std::invalid_argument("Data size not suitable for this parameter");
    _default_value.assign(data.begin(), data.end());
  }
  std::shared_ptr<parameter_type> create_parameter_type(std::string type_identifier, std::string type_name, int buffer_blocks, std::string buffer_load)
  {
    return std::make_shared<parameter_type>(std::move(type_identifier), std::move(type_name), buffer_blocks, std::move(buffer_load));
  }

  bool parameter_link::is_linked() const {
    return other != nullptr;
  }

  size_t parameter_link::hash() const {
    return std::hash<std::shared_ptr<parameter>>{}(other) * 37 +
      std::hash<size_t>{}(block);
  }
  void parameter_buffer_description::append(global_info& global, std::shared_ptr<parameter> const& param)
  {
    if (parameters.emplace(param).second) {
      for (size_t i = 0; i < param->type->buffer_blocks; ++i)
      {
        parameter_link self{ param, i };
        auto const& link = param->get_link(i);
        if (link.is_linked())
        {
          append(global, link.other);
        }
        else
        {
          auto const hash = self.hash();
          if (auto const it = global.buffer_offsets.find(hash); global.buffer_offsets.end() == it)
          {
            buffer_offsets[hash] = base_offset + static_cast<int>(blocks_required);
            global.buffer_offsets[hash] = base_offset + static_cast<int>(blocks_required);
            ++blocks_required;
          }
          else
          {
            buffer_offsets[hash] = it->second;
          }
        }
      }
    }
  }
  void parameter_buffer_description::apply_defaults(std::span<float> buffer)
  {
    if (buffer.size() < base_offset + blocks_required)
      throw std::invalid_argument("Buffer does not have the correct size.");
    for (auto const& p : parameters)
      set_value(buffer.data(), p, p->get_default_value().data(), buffer.size());
  }
  void parameter_buffer_description::set_value(float* buffer, std::shared_ptr<parameter> const& param, float const* data, size_t buflen) const {
    param->set_default_value(std::span<float const>(data, param->type->buffer_blocks));
    for (size_t i = 0; buffer && i < param->type->buffer_blocks; ++i)
    {
      parameter_link self{ param, i };
      auto const& link = param->get_link(i);
      if (!link.is_linked())
      {
        auto const hash = self.hash();
        auto index_it = buffer_offsets.find(hash);
        if (index_it != buffer_offsets.end())
        {
          printf("Updating param index %d\n", base_offset + index_it->second);
          buffer[base_offset + index_it->second] = data[i];
        }
      }
    }
  }
  void parameter_buffer_description::set_value(float* buffer, std::shared_ptr<parameter> const& param, int data) const {
    float cast = float(data);
    set_value(buffer, param, &cast);
  }
  void parameter_buffer_description::set_value(float* buffer, std::shared_ptr<parameter> const& param, unsigned data) const {
    float cast = float(data);
    set_value(buffer, param, &cast);
  }
  void parameter_buffer_description::set_value(float* buffer, std::shared_ptr<parameter> const& param, float data) const {
    set_value(buffer, param, &data);
  }
  void parameter_buffer_description::set_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::vec2 data) const {
    set_value(buffer, param, data.data());
  }
  void parameter_buffer_description::set_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::vec3 data) const {
    set_value(buffer, param, data.data());
  }
  void parameter_buffer_description::set_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::vec4 data) const {
    set_value(buffer, param, data.data());
  }
  void parameter_buffer_description::set_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::vec4ui8 data) const {
    auto const packed_unorm = std::bit_cast<float>(data);
    set_value(buffer, param, &packed_unorm);
  }
  void parameter_buffer_description::set_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::vec2ui data) const
  {
    auto const packed_unorm = std::bit_cast<rnu::vec2>(data);
    set_value(buffer, param, packed_unorm.data());
  }
  void parameter_buffer_description::set_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::mat2 data) const {
    set_value(buffer, param, data.data());
  }
  void parameter_buffer_description::set_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::mat4 data) const {
    set_value(buffer, param, data.data());
  }
  void parameter_buffer_description::get_link_value(float* buffer, parameter_link const& self, float* data) const {
    auto const& link = self.other->get_link(self.block);
    if (!link.is_linked())
    {
      auto const hash = self.hash();
      auto index_it = buffer_offsets.find(hash);
      if (index_it != buffer_offsets.end())
        data[0] = buffer[index_it->second];
    }
    else
    {
      get_link_value(buffer, link, data);
    }
  }
  void parameter_buffer_description::get_value(float* buffer, std::shared_ptr<parameter> const& param, float* data) const {
    if (!buffer)
    {
      std::copy(param->get_default_value().begin(), param->get_default_value().end(), data);
      return;
    }
    for (size_t i = 0; i < param->type->buffer_blocks; ++i)
    {
      parameter_link self{ param, i };
      get_link_value(buffer, self, data + i);
    }
  }
  void parameter_buffer_description::get_value(float* buffer, std::shared_ptr<parameter> const& param, int& data) const {
    float cast = float(data);
    get_value(buffer, param, &cast);
    data = int(cast);
  }
  void parameter_buffer_description::get_value(float* buffer, std::shared_ptr<parameter> const& param, unsigned& data) const {
    float cast = float(data);
    get_value(buffer, param, &cast);
    data = unsigned(cast);
  }
  void parameter_buffer_description::get_value(float* buffer, std::shared_ptr<parameter> const& param, float& data) const {
    get_value(buffer, param, &data);
  }
  void parameter_buffer_description::get_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::vec2& data) const {
    get_value(buffer, param, data.data());
  }
  void parameter_buffer_description::get_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::vec3& data) const {
    get_value(buffer, param, data.data());
  }
  void parameter_buffer_description::get_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::vec4& data) const {
    get_value(buffer, param, data.data());
  }
  void parameter_buffer_description::get_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::mat2& data) const {
    get_value(buffer, param, data.data());
  }
  void parameter_buffer_description::get_value(float* buffer, std::shared_ptr<parameter> const& param, rnu::mat4& data) const {
    get_value(buffer, param, data.data());
  }

  std::string resolve_parameter(
    std::shared_ptr<parameter> param,
    parameter_scope& scope,
    global_info& globals)
  {
    if (globals.used_functions.emplace(globals.hash(param->type)).second)
      globals.auxilliary << param->type->function;

    auto param_hash = globals.hash(param);
    std::string param_name = "par" + myrt::detail::to_hex_string(param_hash);
    if (scope.used_objects.emplace(param_hash).second) {
      std::vector<std::string> block_names(param->type->buffer_blocks);

      for (size_t i = 0; i < param->type->buffer_blocks; ++i)
      {
        parameter_link self{ param, i };
        auto const& link = param->get_link(i);
        size_t const own_hash = self.hash();

        if (!link.is_linked())
        {
          // unlinked. use static block offset
          globals.buffer_offsets[own_hash] = globals.current_offset;
          block_names[i] = "_bk" + std::to_string(globals.current_offset);
          scope.inline_code << "float _bk" << globals.current_offset << "=_G" << scope.hash << "(" << globals.current_offset << ");";
          globals.current_offset++;
        }
        else
        {
          // resolve link.
          auto const other_hash = link.hash();
          if (auto const it = globals.buffer_offsets.find(other_hash); it != globals.buffer_offsets.end())
          {
            block_names[i] = "_bk" + std::to_string(it->second);
          }
          else
          {
            // link is not yet resolved. 
            auto const param_name = resolve_parameter(link.other, scope, globals);

            auto const next_try = globals.buffer_offsets.find(other_hash);
            block_names[i] = "_bk" + std::to_string(next_try->second);
          }
        }
      }

      scope.inline_code << param->type->type_name << " " << param_name << "=_r" << param->type->type_identifier << "(";

      for (int i = 0; i < block_names.size(); ++i)
      {
        if (i != 0)
          scope.inline_code << ",";
        scope.inline_code << block_names[i];
      }

      scope.inline_code << ");";
    }
    return param_name;
  }
}