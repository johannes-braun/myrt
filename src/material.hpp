#pragma once

#include "types.hpp"
#include <myrt/sfml/utils.hpp>
#include <format>
#include <fstream>
#include <cassert>

namespace myrt {
class material_registry {
public:
  struct type_info {
    std::uint32_t id;
    std::shared_ptr<object_type const> type;
    std::string glsl;
  };

  std::shared_ptr<material_registry::type_info const> const& push_type(
      std::shared_ptr<object_type const> type, std::string glsl) {
    glsl = replace(glsl, "open", type->name() + "_open_");
    glsl = replace(glsl, "evaluate", type->name() + "_evaluate_");
    glsl = replace(glsl, "bounce", type->name() + "_bounce_");
    glsl = replace(glsl, "this", type->name() + "_instance_");

    auto str = std::format("{0} {0}_instance_;", type->name());
    auto suffix =
        std::format("void {0}_load_mat_(uint buf) {{ {0}_instance_ = {0}_load_(buf); {0}_open_(); }}", type->name());

    glsl = str + glsl + suffix;

    type_info info;
    info.id = m_type_id++;
    info.type = std::move(type);
    info.glsl = std::move(glsl);

    return m_material_type_infos.emplace_back(std::make_shared<type_info>(std::move(info)));
  }

  std::string generate_code() const {
    std::stringstream stream;
    std::stringstream open;
    std::stringstream eval;
    std::stringstream bounce;

    constexpr auto case_str = "case {}: {} {}_{}_({}); break;";

    open << "uint cmattype = ~0; void open(uint type, uint buf){cmattype=type;switch(type){";
    eval << "void evaluate(vec3 p, vec2 uv, vec3 n, vec3 tl, vec3 tv, out vec3 ref, out float pdf){ ref=vec3(0,0,0);pdf=1;if(cmattype==~0)return;switch(cmattype){";
    bounce << "vec3 bounce(vec2 rnd, vec3 tv, vec3 n){switch(cmattype){";

    for (auto const& info : m_material_type_infos) {
      stream << info->glsl;

      std::format_to(std::ostreambuf_iterator(open), case_str, info->id, "", info->type->name(), "load_mat", "buf");
      std::format_to(std::ostreambuf_iterator(eval), case_str, info->id, "", info->type->name(), "evaluate",
          "p, uv, n, tl, tv, ref, pdf");
      std::format_to(
          std::ostreambuf_iterator(bounce), case_str, info->id, "return", info->type->name(), "bounce", "rnd, tv, n");
    }
    open << "}}";
    eval << "}}";
    bounce << "}return vec3(0);}";

    return stream.str() + open.str() + eval.str() + bounce.str();
  }

private:
  std::uint32_t m_type_id = 0;
  std::vector<std::shared_ptr<material_registry::type_info const>> m_material_type_infos;
};

class material_buffer {
public:
  using material_type_t = std::shared_ptr<material_registry::type_info const>;
  struct material_t {
    size_t index;
    size_t offset;
    material_type_t type_info;
  };

  std::shared_ptr<material_t> const& push(material_type_t type) {
    auto& material = m_material_instances.emplace_back(std::make_shared<material_t>());
    material->type_info = std::move(type);
    material->offset = m_buffer.size();
    m_buffer.resize(m_buffer.size() + material->type_info->type->blocks());

    for (size_t i = 0; i < m_material_instances.size(); ++i) m_material_instances[i]->index = i;

    return material;
  }

  template<typename T> void set(material_t const& material, std::string_view parameter, T&& value) {
    auto& ps = material.type_info->type->parameters();
    auto iter = std::find_if(
        begin(ps), end(ps), [&](auto const& p) { return p.first == parameter; });
    if (iter != end(ps)) {
      assert(sizeof(value) / sizeof(float) == iter->second.blocks());
      std::visit(
          [&](auto const& x) { write(x, m_buffer.data() + material.offset + iter->second.relative_offset, &value); },
          iter->second.type);
    }
  }

  std::vector<float> const& buffer() const {
    return m_buffer;
  }
  std::vector<std::shared_ptr<material_t>> material_instances() {
    return m_material_instances;
  }

private:
  std::vector<float> m_buffer;
  std::vector<std::shared_ptr<material_t>> m_material_instances;
};

} // namespace myrt
