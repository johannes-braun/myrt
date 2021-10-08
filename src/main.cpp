#include <rnu/camera.hpp>
#include "pathtracer/scene.hpp"
#include "pathtracer/pathtracer.hpp"
#include <myrt/sfml/gl.hpp>
#include <myrt/sfml/sfml.hpp>
#include "obj.hpp"
#include <stb_image.h>
#include "pathtracer/parameterized.hpp"

#include "pathtracer/sequential_pathtracer.hpp"
#include "pathtracer/post_process.hpp"
#include "pathtracer/forward_renderer.hpp"

#include "thread_pool.hpp"

#include "ecs/ecs.hpp"
#include <format>

#include <nlohmann/json.hpp>

namespace stbi {
template <typename T> struct deleter {
  void operator()(T* data) const {
    stbi_image_free(data);
  }
};

struct image_info {
  int width = 0;
  int height = 0;
  int channels = 0;
  bool valid = false;

  image_info(std::filesystem::path const& file) {
    valid = 0 == stbi_info(file.string().c_str(), &width, &height, &channels);
  }

protected:
  image_info() = default;
};

template <typename T> using image_data = std::unique_ptr<T[], deleter<T>>;

struct float_image : image_info {
  float_image(std::filesystem::path const& file, int desired_channels = 0) {
    data = image_data<float>{stbi_loadf(file.string().c_str(), &width, &height, &channels, desired_channels)};
    channels = desired_channels;
  }

  image_data<float> data;
};

struct ldr_image : image_info {
  ldr_image(std::filesystem::path const& file, int desired_channels = 0) {
    data = image_data<stbi_uc>{stbi_load(file.string().c_str(), &width, &height, &channels, desired_channels)};
    channels = desired_channels;
  }

  image_data<stbi_uc> data;
};
} // namespace stbi

const static std::filesystem::path res_dir = "../../../res";

void render_function(std::stop_token stop_token, sf::RenderWindow* window);
std::vector<myrt::geometric_object> load_object_file(
    myrt::scene& scene, std::filesystem::path const& path, float import_scale = 6.0f);
std::pair<std::uint32_t, std::uint32_t> load_cubemap(std::filesystem::path folder);
std::uint32_t load_bokeh();
float focus = 10.0f;

#include "pathtracer/material.hpp"

namespace myrt {

parameter_scope new_scope(parameter_scope const& current) {
  parameter_scope scope;
  scope.used_objects = current.used_objects;
  return scope;
}

} // namespace myrt

#include <fstream>

namespace myrt {}

#define glsl(...) #__VA_ARGS__

auto const sop = glsl(

    material<my_type> my_class{
      float f;

      my_class(my_type material) {

      }

      void sample() {}

      void 
    };

);

auto pbr_material_glsl = std::string("#include <pbr.glsl>\n") +
                         glsl(
                             struct material_state_t {
                               pbr_matinfo_t mat;
                               uvec2 albedo_texture;
                               bool is_incoming;
                               float ior1;
                               float ior2;
                             };
                             material_state_t material_state;

                             vec3 material_normal(vec3 n) { return material_state.is_incoming ? n : -n; }

                             void material_load(material m) {
                               material_state.albedo_texture = m.albedo_texture;
                               material_state.mat.albedo_rgba_unorm = color_make(m.albedo);
                               material_state.mat.ior = m.ior;
                               material_state.mat.roughness = m.roughness;
                               material_state.mat.metallic = m.metallic;
                               material_state.mat.transmission = m.transmission;
                             }

                             void material_sample(vec3 point, vec2 uv, vec3 normal, vec3 towards_light,
                                 vec3 towards_viewer, out vec3 reflectance, out float pdf) {
                               sampler2D albedo_texture = sampler2D(material_state.albedo_texture);
                               if (material_state.albedo_texture != uvec2(0))
                                 material_state.mat.albedo_rgba_unorm =
                                     color_make(pow(textureLod(albedo_texture, vec2(uv.x, 1 - uv.y), 0), vec4(2.2)));
                               material_state.is_incoming = dot(normal, -towards_viewer) < 0;
                               material_state.ior1 = material_state.is_incoming ? 1.0 : material_state.mat.ior;
                               material_state.ior2 = material_state.is_incoming ? material_state.mat.ior : 1.0;

                               brdf_result_t ev;
                               pbr_eval(material_state.mat, towards_viewer, towards_light, material_normal(normal),
                                   material_state.ior1, material_state.ior2, ev);
                               reflectance = ev.reflectance;
                               pdf = ev.pdf;
                             } vec3 material_continue_ray(vec2 random, vec3 towards_viewer, vec3 normal) {
                               material_state.is_incoming = dot(normal, -towards_viewer) < 0;
                               material_state.ior1 = material_state.is_incoming ? 1.0 : material_state.mat.ior;
                               material_state.ior2 = material_state.is_incoming ? material_state.mat.ior : 1.0;
                               return pbr_resample(random, material_state.mat, towards_viewer, material_normal(normal),
                                   material_state.ior1, material_state.ior2);
                             });

#include <myrt/xgl/xgl_tokenizer.hpp>
#include <myrt/xgl/xgl_parser.hpp>
#include <myrt/xgl/xgl_evaluator.hpp>
#include <myrt/xgl/xgl_scope_system.hpp>

#ifdef CHECK_XGL_STUFF
void write_expr(std::ostream& o, myrt::xgl::expr_entity const& e) {
  namespace xgl = myrt::xgl;
  struct visitor_t {
    // void operator()(xgl::expr_entity_operator const& e) {
    //  e.op;
    //}

    void operator()(xgl::expr_entity_function const& e) {
      os << e.func.data->name << "(";
      bool comma = false;
      for (auto& par : ex.data->parameters) {
        if (comma)
          os << ",";
        comma = true;
        write_expr(os, par);
      }
      os << ")";
    }
    void operator()(xgl::expr_entity_literal const& e) {
      os << e.value;
    }
    void operator()(xgl::expr_entity_construct const& e) {}
    void operator()(xgl::expr_entity_variable const& e) {
      os << e.var.data->name;
    }
    void operator()(xgl::expr_entity_member const& e) {
      // os << e.member_name;
    }

    std::ostream& os;
    myrt::xgl::expr_entity const& ex;
  } visitor{.os = o, .ex = e};
  std::visit(visitor, e.data->data);
}
#endif // CHECK_XGL_STUFF

int main(int argc, char** argv) {
#ifdef CHECK_XGL_STUFF
  std::ifstream file_stream("C:\\Users\\johan\\Documents\\Projekte\\myrt\\src\\myrt.xgl\\test.xgl", std::ios::binary);
  std::string source((std::istreambuf_iterator<char>(file_stream)), std::istreambuf_iterator<char>());
  file_stream.close();
  myrt::xgl::tokenizer tok(source);
  myrt::xgl::parser parser;
  auto root_scope = parser.parse(source);

  std::ostringstream os;
  auto intro = root_scope->find_function({"pbr", "material"});
  auto& bdy = intro->data->available_overloads[0]->body;
  for (auto& st : bdy->statements()) {
    namespace xgl = myrt::xgl;
    struct visitor_t {
      void operator()(xgl::simple_statement_entity const& e) {
        if (e.expression)
          write_expr(oss, *e.expression);
      }
      void operator()(xgl::variable_declaration_entity const& e) {
        oss << e.variable.data->type.data->base->as_builtin()->name << " " << e.variable.data->name;

        if (e.variable.data->value) {
          oss << " = ";
          write_expr(oss, *e.variable.data->value);
        }
      }
      void operator()(xgl::if_statement_entity const& e) {}
      void operator()(xgl::for_statement_entity const& e) {}
      void operator()(xgl::while_statement_entity const& e) {}
      void operator()(xgl::do_while_statement_entity const& e) {}
      void operator()(xgl::block_statement_entity const& e) {
        __debugbreak();
      }
      void operator()(xgl::return_statement_entity const& e) {}

      std::ostringstream& oss;
    } visitor{.oss = os};

    std::visit(visitor, st.data->data);
  }

  auto str = os.str();

  myrt::xgl::evaluator eval;
  eval.load(root_scope);
#endif // CHECK_XGL_STUFF

  sf::ContextSettings settings;
  settings.majorVersion = 4;
  settings.minorVersion = 6;
  settings.attributeFlags |= sf::ContextSettings::Debug;
  settings.depthBits = 24;
  settings.antialiasingLevel = 4;

  sf::RenderWindow window(sf::VideoMode(1600, 1200), "MyRT", sf::Style::Default, settings);
  window.setActive(false);

  std::jthread render_thread(&render_function, &window);

  for (auto event : myrt::sfml::poll_event(window)) {
    switch (event.get().type) {
      case sf::Event::MouseWheelScrolled:
        if (!ImGui::GetIO().WantCaptureMouse)
          focus += 0.2f * event.get().mouseWheelScroll.delta;
        break;
    }
  }
  render_thread.request_stop();
}

struct object_component : myrt::component<object_component> {
  std::variant<myrt::geometric_object, myrt::sdf_object, std::vector<myrt::geometric_object>> object;
};

struct transform_component : myrt::component<transform_component> {
  rnu::vec3 position;
  rnu::vec3 scale{1};
  rnu::quat orientation;

  constexpr rnu::mat4 matrix() const {
    return rnu::translation(position) * rnu::mat4(orientation.matrix()) * rnu::scale(scale);
  }
};

struct object_ui_component : myrt::component<object_ui_component> {};

struct make_ui_t {
  bool operator()(std::vector<myrt::geometric_object>& obj) const {
    auto const begin = ImGui::Begin("Geometric Objects");
    if (begin) {
      ImGui::PushID(&obj);
      for (auto& o : obj) {
        ImGui::PushID(o.name.c_str());
        ImGui::LabelText("Name", "%s", o.name.c_str());
        ImGui::Checkbox("Show", &o.show);
        ImGui::PopID();
      }
    }
    return begin;
  }
  bool operator()(myrt::geometric_object& obj) const {
    auto const begin = ImGui::Begin("Geometric Objects");
    if (begin) {
      ImGui::PushID(obj.name.c_str());
      ImGui::LabelText("Name", "%s", obj.name.c_str());
      ImGui::Checkbox("Show", &obj.show);
    }
    return begin;
  }

  bool operator()(myrt::sdf_object& obj) const {
    auto const begin = ImGui::Begin("SDF Objects");
    if (begin) {
      ImGui::PushID(obj.name.c_str());
      ImGui::LabelText("Name", "%s", obj.name.c_str());
      ImGui::Checkbox("Show", &obj.show);
    }
    return begin;
  }
};

struct object_ui_system : public myrt::typed_system<object_component, object_ui_component, transform_component> {
public:
  void update(
      duration_type delta, object_component* o, object_ui_component* ui, transform_component* t) const override {
    if (std::visit(make_ui_t{}, o->object)) {
      ImGui::BeginGroup();
      ImGui::Text("Transform");
      ImGui::DragFloat3("Position", t->position.data(), 0.01f);
      ImGui::DragFloat3("Scale", t->scale.data(), 0.01f);
      ImGui::DragFloat4("Orientation", t->orientation.data(), 0.01f);
      t->orientation = normalize(t->orientation);
      ImGui::EndGroup();

      ImGui::Separator();
      ImGui::PopID();
      ImGui::End();
    }
  }
};

struct draw_t {
  transform_component* t;

  void operator()(std::vector<myrt::geometric_object>& obj) const {
    auto pm = t->matrix();
    for (auto& o : obj) {
      o.enqueue(pm);
    }
  }

  template <typename T> void operator()(T&& obj) const {
    obj.enqueue(t->matrix());
  }
};

struct object_system : public myrt::typed_system<object_component, transform_component> {
public:
  void update(duration_type delta, object_component* o, transform_component* t) const override {
    std::visit(draw_t{.t = t}, o->object);
  }
};

struct camera_component : myrt::component<camera_component> {
  rnu::cameraf camera;
  rnu::mat4 projection;
  std::uint32_t width;
  std::uint32_t height;
};

struct renderer_component : myrt::component<renderer_component> {
  myrt::scene* scene;
  myrt::texture_provider_t* provider;
  std::variant<myrt::sequential_pathtracer*, myrt::forward_renderer*> renderer;
  std::shared_ptr<myrt::texture_t> render_target;
};

struct render_system : public myrt::typed_system<camera_component, renderer_component> {
public:
  void update(duration_type delta, camera_component* o, renderer_component* r) const override {
    if (!r->scene)
      return;

    r->render_target = std::visit(
        [&](auto* object) {
          object->set_view_matrix(o->camera.matrix(false));
          object->set_projection_matrix(o->projection);
          return object->run(*r->provider, *r->scene, o->width, o->height);
        },
        r->renderer);
  }
};

struct renderer_ui_component : myrt::component<renderer_ui_component> {};

struct make_renderer_ui {
  double dt;

  void operator()(myrt::sequential_pathtracer* pt) {
    if (ImGui::Begin("Pathtracer")) {
      ImGui::Text("Samples: %d (%.00f sps)", pt->sample_count(), 1.f / dt);
      if (ImGui::Button("Reload Shaders"))
        pt->invalidate_shaders();
      if (ImGui::Button("Reload Trace Shader"))
        pt->invalidate_shader(myrt::shader_flags::trace);
      if (ImGui::Button("Reload Color Shader"))
        pt->invalidate_shader(myrt::shader_flags::color);

      ImGui::Separator();

      if (auto x = pt->get_dof_enabled(); ImGui::Checkbox("DOF", &x))
        pt->set_dof_enabled(x);

      if (auto x = pt->get_lens_size(); ImGui::DragFloat2("Lens Size", x.data(), 0.001f, 0.0f, 10000.f))
        pt->set_lens_size(x);

      if (auto x = pt->get_focus(); ImGui::DragFloat("Focus", &x, 0.001f, 0.0f, 10000.f))
        pt->set_focus(x);

      if (auto x = pt->get_sdf_marching_steps(); ImGui::DragFloat("SDF Steps", &x, 0.01f, 0.0f, 10000.f))
        pt->set_sdf_marching_steps(x);

      if (auto x = pt->get_sdf_marching_epsilon();
          ImGui::DragFloat("SDF Epsilon", &x, 0.0000001f, 0.0f, 10000.f, "%.8f"))
        pt->set_sdf_marching_epsilon(x);

      if (auto x = pt->get_num_bounces(); ImGui::DragInt("Bounces", &x, 0.01f, 0, 20))
        pt->set_num_bounces(x);

      ImGui::End();
    }
  }
  void operator()(myrt::forward_renderer* fwd) {
    if (ImGui::Begin("Renderer")) {
      ImGui::Text("Framerate: %.00f fps",1.f / dt);
      if (ImGui::Button("Reload RESOLVE")) {
        fwd->reload_resolve_shader();
      }
      if (ImGui::Button("Reload RENDER")) {
        fwd->reload_render_shader();
      }
      ImGui::End();
    }
  }
};

struct renderer_ui_system : public myrt::typed_system<renderer_component, renderer_ui_component> {
public:
  virtual void update(duration_type delta, renderer_component* r, renderer_ui_component* ui) const override {
    std::visit(make_renderer_ui{.dt = delta.count()}, r->renderer);
  }
};

struct parameter_type;
struct parameter;

struct constant_type {
  std::string_view id;
  std::string_view glsl_type;
  std::string_view glsl_code;
  size_t blocks;

  void (*write)(float* dst, void const* src);
  void (*read)(void* dst, float const* src);
};

enum class parameter_type_id : std::int64_t {
  type_float,
  type_int,
  type_uint,
  type_vec2,
  type_vec3,
  type_vec4,
  type_vec4unorm8,
  type_ivec2,
  type_ivec3,
  type_ivec4,
  type_uvec2,
  type_uvec3,
  type_uvec4,
  type_mat2,
  type_mat3,
  type_mat4,
};

namespace internal_types {
constexpr static std::array registered_types{
    constant_type{"float", "float", "return in0", 1, [](auto* d, auto const* s) { std::memcpy(d, s, sizeof(float)); },
        [](auto* d, auto const* s) { std::memcpy(d, s, sizeof(float)); }},
    constant_type{"int", "int", "return floatBitsToInt(in0)", 1,
        [](auto* d, auto const* s) { std::memcpy(d, s, sizeof(std::int32_t)); },
        [](auto* d, auto const* s) { std::memcpy(d, s, sizeof(std::int32_t)); }},
    constant_type{"uint", "uint", "return floatBitsToUint(in0)", 1,
        [](auto* d, auto const* s) { std::memcpy(d, s, sizeof(std::uint32_t)); },
        [](auto* d, auto const* s) { std::memcpy(d, s, sizeof(std::uint32_t)); }},
    constant_type{"vec2", "vec2", "return vec2(in0, in1)", 2,
        [](auto* d, auto const* s) { std::memcpy(d, s, 2 * sizeof(float)); },
        [](auto* d, auto const* s) { std::memcpy(d, s, 2 * sizeof(float)); }},
    constant_type{"vec3", "vec3", "return vec3(in0, in1, in2)", 3,
        [](auto* d, auto const* s) { std::memcpy(d, s, 3 * sizeof(float)); },
        [](auto* d, auto const* s) { std::memcpy(d, s, 3 * sizeof(float)); }},
    constant_type{"vec4", "vec4", "return vec4(in0, in1, in2, in3)", 4,
        [](auto* d, auto const* s) { std::memcpy(d, s, 4 * sizeof(float)); },
        [](auto* d, auto const* s) { std::memcpy(d, s, 4 * sizeof(float)); }},
    constant_type{"vec4unorm8", "vec4", "return unpackUnorm4x8(floatBitsToUint(in0))", 1,
        [](auto* d, auto const* s) { std::memcpy(d, s, sizeof(std::uint32_t)); },
        [](auto* d, auto const* s) { std::memcpy(d, s, sizeof(std::uint32_t)); }},
    constant_type{"ivec2", "ivec2", "return ivec2(floatBitsToInt(in0), floatBitsToInt(in1))", 2,
        [](auto* d, auto const* s) { std::memcpy(d, s, 2 * sizeof(std::int32_t)); },
        [](auto* d, auto const* s) { std::memcpy(d, s, 2 * sizeof(std::int32_t)); }},
    constant_type{"ivec3", "ivec3", "return ivec3(floatBitsToInt(in0), floatBitsToInt(in1), floatBitsToInt(in2))", 3,
        [](auto* d, auto const* s) { std::memcpy(d, s, 3 * sizeof(std::int32_t)); },
        [](auto* d, auto const* s) { std::memcpy(d, s, 3 * sizeof(std::int32_t)); }},
    constant_type{"ivec4", "ivec4",
        "return ivec4(floatBitsToInt(in0), floatBitsToInt(in1), floatBitsToInt(in2), floatBitsToInt(in3))", 4,
        [](auto* d, auto const* s) { std::memcpy(d, s, 4 * sizeof(std::int32_t)); },
        [](auto* d, auto const* s) { std::memcpy(d, s, 4 * sizeof(std::int32_t)); }},
    constant_type{"uvec2", "uvec2", "return uvec2(floatBitsToUint(in0), floatBitsToUint(in1))", 2,
        [](auto* d, auto const* s) { std::memcpy(d, s, 2 * sizeof(std::uint32_t)); },
        [](auto* d, auto const* s) { std::memcpy(d, s, 2 * sizeof(std::uint32_t)); }},
    constant_type{"uvec3", "uvec3", "return uvec3(floatBitsToUint(in0), floatBitsToUint(in1), floatBitsToUint(in2))", 3,
        [](auto* d, auto const* s) { std::memcpy(d, s, 3 * sizeof(std::uint32_t)); },
        [](auto* d, auto const* s) { std::memcpy(d, s, 3 * sizeof(std::uint32_t)); }},
    constant_type{"uvec4", "uvec4",
        "return uvec4(floatBitsToUint(in0), floatBitsToUint(in1), floatBitsToUint(in2), floatBitsToUint(in3))", 4,
        [](auto* d, auto const* s) { std::memcpy(d, s, 4 * sizeof(std::uint32_t)); },
        [](auto* d, auto const* s) { std::memcpy(d, s, 4 * sizeof(std::uint32_t)); }},
    constant_type{"mat2", "mat2", "return mat2(in0, in1, in2, in3)", 4,
        [](auto* d, auto const* s) { std::memcpy(d, s, 4 * sizeof(float)); },
        [](auto* d, auto const* s) { std::memcpy(d, s, 4 * sizeof(float)); }},
    constant_type{"mat3", "mat3", "return mat3(in0, in1, in2, in3, in4, in5, in6, in7, in8, in9)", 9,
        [](auto* d, auto const* s) { std::memcpy(d, s, 9 * sizeof(float)); },
        [](auto* d, auto const* s) { std::memcpy(d, s, 9 * sizeof(float)); }},
    constant_type{"mat4", "mat4",
        "return mat4(in0, in1, in2, in3, in4, in5, in6, in7, in8, in9, in10, in11, in12, in13, in14, in15)", 16,
        [](auto* d, auto const* s) { std::memcpy(d, s, 16 * sizeof(float)); },
        [](auto* d, auto const* s) { std::memcpy(d, s, 16 * sizeof(float)); }}};

constexpr std::string_view internal_load_identifier = "ldv"; // LoaD Value

namespace generate_loaders {
  template <size_t S> struct c_str { char string[S]; };

  constexpr size_t log10(size_t v) {
    size_t l = 1;
    while (v /= 10) l++;
    return l;
  }
  constexpr double powr(double x, int exp) {
    int sign = 1;
    if (exp < 0) {
      sign = -1;
      exp = -exp;
    }
    if (exp == 0)
      return x < 0 ? -1.0 : 1.0;
    double ret = x;
    while (--exp) ret *= x;
    return sign > 0 ? ret : 1.0 / ret;
  }

  template <size_t... I> constexpr size_t par_size(std::index_sequence<I...>) {
    return ((std::size(",float in") - 1 + log10(I)) + ... + -1);
  }
  static_assert(par_size(std::make_index_sequence<4>{}) == std::size("float in0,float in1,float in2,float in3") - 1);

  constexpr size_t type_str_len(size_t index, constant_type const& type) {
    return type.glsl_code.length() + type.glsl_type.length() + 6 + internal_load_identifier.length() + log10(index);
  }

  constexpr void append(char** ptr_ptr, char c) {
    char* ptr = *ptr_ptr;
    *ptr = c;
    *ptr_ptr += 1;
  }

  constexpr void append(char** ptr_ptr, std::string_view str) {
    for (size_t i = 0; i < str.size(); ++i) append(ptr_ptr, str[i]);
  }

  constexpr void append_size_t(char** ptr_ptr, size_t s) {
    auto const l10 = log10(s) - 1;
    auto div = size_t(powr(10, l10));
    do {
      append(ptr_ptr, '0' + (s / div) % 10);
    } while (div /= 10);
  }

  constexpr void insert(char*& ptr, size_t index, constant_type const& type) {
    append(&ptr, type.glsl_type);
    append(&ptr, ' ');
    append(&ptr, internal_load_identifier);
    append_size_t(&ptr, index);
    append(&ptr, '(');
    for (size_t pindex = 0; pindex < type.blocks; ++pindex) {
      if (pindex != 0)
        append(&ptr, ',');
      append(&ptr, "float in");
      append_size_t(&ptr, pindex);
    }
    append(&ptr, ')');
    append(&ptr, '{');
    append(&ptr, type.glsl_code);
    append(&ptr, ";}");
  }

  template <size_t... I> constexpr auto build_loaders_impl(std::index_sequence<I...>) {
    c_str<((par_size(std::make_index_sequence<registered_types[I].blocks>{}) + type_str_len(I, registered_types[I])) +
           ... + 1)>
        result{};

    char* ptr = result.string;

    (insert(ptr, I, registered_types[I]), ...);

    return result;
  }

  constexpr auto build_loaders() {
    return build_loaders_impl(std::make_index_sequence<std::size(registered_types)>{});
  }

  constexpr auto result = build_loaders();
} // namespace generate_loaders

constexpr std::string_view loader_glsl = {generate_loaders::result.string,
    std::find((const char*)generate_loaders::result.string,
        generate_loaders::result.string + std::size(generate_loaders::result.string), '\0')};

constexpr std::optional<parameter_type_id> get_id(std::string_view identifier) {
  auto const iter = std::find_if(
      begin(registered_types), end(registered_types), [&](constant_type const& type) { return type.id == identifier; });
  if (iter == end(registered_types))
    return std::nullopt;
  return parameter_type_id{std::distance(begin(registered_types), iter)};
}
constexpr constant_type const* get_type(parameter_type_id id) {
  if (size_t(id) >= registered_types.size())
    return nullptr;
  return &registered_types[size_t(id)];
}
constexpr parameter_type_id get_id(constant_type const* type) {
  return parameter_type_id{std::distance(data(registered_types), type)};
}
constexpr constant_type const* get_type(std::string_view identifier) {
  auto const id = get_id(identifier);
  if (!id)
    return nullptr;
  return get_type(*id);
}
}; // namespace internal_types

struct object_host {
  std::vector<float> m_buffer;
};

struct object_type {
public:
  friend std::string generate_object_loader(object_type const& obj);

  object_type(std::string name) : m_name(std::move(name)) {}

  std::string const& name() const noexcept {
    return m_name;
  }
  size_t blocks() const {
    return m_size;
  }

  void add_parameter(std::string name, parameter_type_id type_id) {
    auto& back = m_parameters.emplace_back(
        std::move(name), parameter_info{static_cast<std::int64_t>(m_size), internal_types::get_type(type_id)});
    m_size += back.second.blocks();
  }
  void add_parameter(std::string name, std::variant<constant_type const*, std::shared_ptr<object_type const>> type) {
    auto& back = m_parameters.emplace_back(std::move(name), parameter_info{static_cast<std::int64_t>(m_size), type});
    m_size += back.second.blocks();
  }

private:
  struct parameter_info {
    std::int64_t relative_offset;
    std::variant<constant_type const*, std::shared_ptr<object_type const>> type;

    size_t blocks() const {
      const struct {
        size_t operator()(constant_type const* type) const {
          return type->blocks;
        }
        size_t operator()(std::shared_ptr<object_type const> type) const {
          return type->blocks();
        }
      } block_count_visitor;

      return std::visit(block_count_visitor, type);
    }
  };

  std::string m_name;
  std::size_t m_size = 0;
  std::vector<std::pair<std::string, parameter_info>> m_parameters;
};

struct type_registry {
public:
  template <typename T, typename... Args> std::shared_ptr<object_type const> const& create(Args&&... args) {
    return m_object_types.emplace_back(std::make_shared<T const>(*this, std::forward<Args>(args)...));
  }

  void push_back(std::shared_ptr<object_type const> type) {
    m_object_types.push_back(std::move(type));
  }

  std::variant<constant_type const*, std::shared_ptr<object_type const>> find(std::string_view name) {
    auto const iter =
        std::find_if(begin(m_object_types), end(m_object_types), [&](auto const& ptr) { return ptr->name() == name; });
    if (iter == end(m_object_types))
      return internal_types::get_type(name);
    return *iter;
  }

  std::vector<std::shared_ptr<object_type const>> const& object_types() const {
    return m_object_types;
  }

private:
  std::vector<std::shared_ptr<object_type const>> m_object_types;
};

struct color_object_type : object_type {
public:
  color_object_type(type_registry& registry) : object_type("color_t") {
    add_parameter("value", parameter_type_id::type_vec4unorm8);
  }
};

struct material_object_type : object_type {
public:
  material_object_type(type_registry& registry) : object_type("material") {
    add_parameter("albedo", registry.find("color_t"));
    add_parameter("ior", parameter_type_id::type_float);
    add_parameter("roughness", parameter_type_id::type_float);
    add_parameter("metallic", parameter_type_id::type_float);
    add_parameter("transmission", parameter_type_id::type_float);
    add_parameter("albedo_texture", parameter_type_id::type_uvec2);
  }
};

#include <sstream>

std::string generate_object_loader(object_type const& obj) {
  std::stringstream stream;
  std::stringstream generator;

  generator << obj.name() << " ld" << obj.name() << "(uint buffer_offset){";
  generator << obj.name() << " obj;";

  int num_float = 0;
  stream << "struct " << obj.name() << "{";
  for (auto const& par : obj.m_parameters) {
    if (std::holds_alternative<std::shared_ptr<object_type const>>(par.second.type)) {
      auto const param_type = std::get<std::shared_ptr<object_type const>>(par.second.type);
      stream << param_type->name() << ' ' << par.first << ';';

      generator << "obj." << par.first << "=ld" << param_type->name() << "(buffer_offset+" << num_float << ");";
      num_float += param_type->blocks();

    } else if (std::holds_alternative<constant_type const*>(par.second.type)) {
      auto const param_type = std::get<constant_type const*>(par.second.type);
      stream << param_type->glsl_type << ' ' << par.first << ';';

      for (size_t block = 0; block < param_type->blocks; ++block) {
        generator << "float f" << num_float + block << "=load_float(buffer_offset+"
                  << (block + par.second.relative_offset) << ");";
      }
      generator << "obj." << par.first << "=" << internal_types::internal_load_identifier
                << size_t(internal_types::get_id(param_type)) << "(";
      for (size_t block = 0; block < param_type->blocks; ++block) {
        if (block != 0)
          generator << ",";
        generator << "f" << num_float + block;
      }
      generator << ");";
      num_float += param_type->blocks;
    }
  }
  stream << "};";
  generator << "return obj;}";
  return stream.str() + generator.str();
}

std::string generate_object_loaders(type_registry& registry) {
  std::ostringstream stream;
  stream << internal_types::loader_glsl;
  for (auto const& type : registry.object_types()) {
    stream << generate_object_loader(*type);
  }
  return stream.str();
}

auto const type_json = R"(
{
  "name": "my_type",
  "members": [
    {"my_param": "float"},
    {"my_purr": "int"},
    {"my_material": "material"},
    {"my_color": "color_t"}
  ]
}
)";

void render_function(std::stop_token stop_token, sf::RenderWindow* window) {
  myrt::thread_pool loading_pool;
  myrt::gl::start(*window);
  std::format_to(std::ostreambuf_iterator(std::cout), "{} starting...", "Rendering");

  type_registry registry;
  registry.create<color_object_type>();
  registry.create<material_object_type>();

  nlohmann::json j = nlohmann::json::parse(type_json);
  auto dynamic_type = std::make_shared<object_type>(j["name"]);
  for (auto const& x : j["members"]) {
    for (auto const& [key, value] : x.get<nlohmann::json::object_t>()) {
      dynamic_type->add_parameter(key, registry.find(value));
    }
  }
  registry.push_back(std::move(dynamic_type));

  auto const genc = generate_object_loaders(registry);

  myrt::scene scene;
  myrt::sequential_pathtracer pathtracer_renderer;
  myrt::forward_renderer forward_renderer;

  pathtracer_renderer.set_dof_enabled(true);
  pathtracer_renderer.set_lens_size({0.2, 0.2});
  pathtracer_renderer.set_focus(focus);
  pathtracer_renderer.set_bokeh_mask(load_bokeh());

  myrt::ecs ecs;
  std::vector<myrt::entity> entities;

  object_system object_system;
  object_ui_system geo_ui_system;
  render_system render_system;
  renderer_ui_system render_ui_system;

  myrt::system_list systems;
  systems.add(object_system);
  systems.add(geo_ui_system);
  systems.add(render_system);
  systems.add(render_ui_system);

  myrt::texture_provider_t global_texture_provider;
  auto const render_entity =
      ecs.create_entity_unique(camera_component{}, renderer_component{}, renderer_ui_component{});
  auto* rc = render_entity->get<renderer_component>();
  rc->provider = &global_texture_provider;
  rc->renderer = &forward_renderer;
  rc->scene = &scene;

  render_entity->get<camera_component>()->camera = {rnu::vec3{0.0f, 0.0f, 15.f}};

  auto const import_scale = 4;

  entities.push_back(ecs.create_entity(object_component{.object = load_object_file(scene, "fischl/fischl.obj", import_scale)},
      transform_component{}, object_ui_component{}));

  myrt::async_resource<std::pair<std::uint32_t, std::uint32_t>> cube_resource(loading_pool, [] {
    sf::Context context;
    return load_cubemap("gamrig");
  });

  {
    myrt::sdf_object abstract_art_sdf;

    myrt::sdf::vertical_capsule capsule(3.f, 0.5f);
    myrt::sdf::sphere sphere(1.f);
    sphere.transform(myrt::sdf::translate(rnu::vec3(0.5, 0, 0.5)).transform(myrt::sdf::mirror_axis{0}));

    myrt::sdf::smooth_union unite(0.3f);
    unite.apply(sphere, capsule);

    myrt::sdf::sphere sphere2;
    sphere2.link_parameter(sphere2[sphere.radius], sphere[sphere.radius]);
    sphere2.transform(myrt::sdf::translate(rnu::vec3(0.5, 3, 0.5)).transform(myrt::sdf::mirror_axis{0}));
    auto root = myrt::sdf::smooth_union{}.apply(sphere2, unite);
    root.link_parameter(root[unite.factor], unite[unite.factor]);

    abstract_art_sdf.name = "Abstract Art";
    abstract_art_sdf.sdf = scene.push_sdf(myrt::sdf_info_t{.root = unite.get_pointer()});
    abstract_art_sdf.set(capsule.get_parameter(capsule.material), 1);
    abstract_art_sdf.set(sphere.get_parameter(sphere.material), 2);

    entities.push_back(ecs.create_entity(object_component{.object = std::move(abstract_art_sdf)},
        transform_component{.scale = {1}}, object_ui_component{}));
  }

  bool cubemap_enabled = false;
  bool bokeh_enabled = false;
  bool rr_enabled = false;
  float lens_radius = 100.f;
  int bounces_per_iteration = 8;

  int march_steps = 400;
  int march_eps_exp = 6;
  float march_eps_fac = 0.75f;

  bool outlined = true;

  float pg_sphere_radius = 3;
  float pg_smoothness1 = 0.1f;
  rnu::vec2 torus_size(0.3, 1.2);

  std::uint32_t fbo;
  glCreateFramebuffers(1, &fbo);

  float raffnes = 0.04;
  float tm = 0.04;

  myrt::denoise denoise;
  myrt::fxaa fxaa;
  myrt::tonemap tonemap;
  myrt::bloom bloom;

  for (auto frame : myrt::gl::next_frame(*window)) {
    if (stop_token.stop_requested())
      break;
    global_texture_provider.new_frame();
    rnu::vec2i size(window->getSize().x, window->getSize().y);

    {
      render_entity->get<camera_component>()->width = size.x;
      render_entity->get<camera_component>()->height = size.y;
      render_entity->get<camera_component>()->projection = rnu::cameraf::projection(
          rnu::radians(70), float(window->getSize().x) / float(window->getSize().y), 0.01f, 1000.f, true);

      if (window->hasFocus() && !ImGui::GetIO().WantCaptureKeyboard) {
        render_entity->get<camera_component>()->camera.axis(
            float(frame.delta_time.count()) * (1.f + 5 * sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)),
            sf::Keyboard::isKeyPressed(sf::Keyboard::W), sf::Keyboard::isKeyPressed(sf::Keyboard::S),
            sf::Keyboard::isKeyPressed(sf::Keyboard::A), sf::Keyboard::isKeyPressed(sf::Keyboard::D),
            sf::Keyboard::isKeyPressed(sf::Keyboard::E), sf::Keyboard::isKeyPressed(sf::Keyboard::Q));
      }
      if (window->hasFocus() && !ImGui::GetIO().WantCaptureMouse) {
        render_entity->get<camera_component>()->camera.mouse(float(sf::Mouse::getPosition(*window).x),
            float(sf::Mouse::getPosition(*window).y), sf::Mouse::isButtonPressed(sf::Mouse::Left));
      }
    }

    if (outlined)
      render_entity->get<renderer_component>()->renderer = &forward_renderer;
    else
      render_entity->get<renderer_component>()->renderer = &pathtracer_renderer;

    ecs.update(frame.delta_time, systems);

    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) {
      float mouseX = sf::Mouse::getPosition(*window).x / (size.x * 0.5f) - 1.0f;
      float mouseY = sf::Mouse::getPosition(*window).y / (size.y * 0.5f) - 1.0f;

      auto const K = render_entity->get<camera_component>()->camera.matrix(false);
      auto const ivp = inverse(render_entity->get<camera_component>()->projection * K);
      rnu::vec2 tsize = size;

      auto vd = ivp * rnu::vec4(mouseX, -mouseY, 1.f, 1.f);

      myrt::ray_t ray{.origin = render_entity->get<camera_component>()->camera.position(),
          .direction = normalize(rnu::vec3(vd)),
          .length = std::numeric_limits<float>::max()};
      auto picked = scene.pick(ray);
      if (picked) {
        scene.set_parameter(scene.materials()[picked->drawable->material_index], "metallic", 1.0f);
        scene.set_parameter(scene.materials()[picked->drawable->material_index], "roughness", 0.22f);
        scene.set_parameter(
            scene.materials()[picked->drawable->material_index], "albedo", rnu::vec4(1.f, 0.4f, 0.1f, 1));
        scene.set_parameter(scene.materials()[picked->drawable->material_index], "albedo_texture", rnu::vec2ui(0));
        printf("Picked object: #%zd at distance %.5f\n", picked->index, picked->t);
      }
    }

    static float scaling1 = 2.f;
    static float scaling2 = 8.f;
    static float fac1 = 0.3f;
    static float fac2 = 0.15f;
    static float step = 1.25f;

    if(ImGui::Begin("PP")) {
      ImGui::DragFloat("S1", &scaling1, 0.01f, 0.5f, 100.f);
      ImGui::DragFloat("S2", &scaling2, 0.01f, 0.5f, 100.f);
      ImGui::DragFloat("F1", &fac1, 0.01f, 0.f, 1.f);
      ImGui::DragFloat("F2", &fac2, 0.01f, 0.f, 1.f);
      ImGui::DragFloat("Step", &step, 0.01f, 0.1f, 100.f);

      ImGui::End();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    auto render_target = render_entity->get<renderer_component>()->render_target;
    auto result = render_target;
    bloom.texture_scaling = scaling1;
    bloom.overlay_factor = fac1;
    bloom.blur_step = step;
    result = bloom.process(global_texture_provider, result->id());
    result = tonemap.process(global_texture_provider, result->id());
    result = fxaa.process(global_texture_provider, result->id());
    //result = denoise.process(global_texture_provider, result->id());
    result = fxaa.process(global_texture_provider, result->id());
    glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, result->id(), 0);
    glBlitNamedFramebuffer(fbo, 0, 0, 0, size.x, size.y, 0, 0, size.x, size.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    ImGui::Begin("Settings");

    if (ImGui::CollapsingHeader("Denoise")) {
      ImGui::DragFloat("Exponent", &denoise.exponent, 0.001, -10.f, 10.f);
      ImGui::DragFloat("Strength", &denoise.strength, 0.001, -10.f, 10.f);
    }
    if (ImGui::Checkbox("outlined", &outlined)) {
      pathtracer_renderer.invalidate_counter();
    }
    ImGui::Checkbox("CUBEMPAP", &cubemap_enabled);

    if (cubemap_enabled)
      cube_resource.current([&](auto const& cur) { pathtracer_renderer.set_cubemap(cur.first, cur.second); });
    else
      pathtracer_renderer.set_cubemap(0, 0);
    ImGui::End();
  }
}

enum class residence { none, resident };

template <residence Residence> auto ldr_texture(std::filesystem::path const& path, GLenum format = GL_RGB565) {
  std::uint32_t texture{};
  glCreateTextures(GL_TEXTURE_2D, 1, &texture);
  glTextureParameterf(texture, GL_TEXTURE_MAX_ANISOTROPY, 16.f);
  glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTextureParameteri(texture, GL_TEXTURE_WRAP_R, GL_REPEAT);
  glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);

  stbi::ldr_image image(path, 3);
  glTextureStorage2D(texture, 1, GL_RGB8, image.width, image.height);
  glTextureSubImage2D(texture, 0, 0, 0, image.width, image.height, GL_RGB, GL_UNSIGNED_BYTE, image.data.get());
  glGenerateTextureMipmap(texture);

  if constexpr (Residence == residence::none) {
    return texture;
  } else {
    auto handle = glGetTextureHandleARB(texture);
    glMakeTextureHandleResidentARB(handle);
    return std::make_pair(texture, handle);
  }
}

std::vector<myrt::geometric_object> load_object_file(
    myrt::scene& scene, std::filesystem::path const& path, float import_scale) {
  std::map<std::string, std::shared_ptr<myrt::parameter_type>> pars;
  nlohmann::json j = nlohmann::json::parse(R"(
{
  "parameters": {
    "albedo": "vec4",
    "ior": "float",
    "roughness": "float",
    "metallic": "float",
    "transmission": "float",
    "albedo_texture": "uvec2"
  },
  "source": "${sourceDir}/file.glsl"
}
)");
  for (auto const& [key, value] : j["parameters"].get<nlohmann::json::object_t>()) {
    pars[key] = myrt::type_registry[value];
  }

  myrt::basic_material_type pbr_material_type(pars, pbr_material_glsl);

  std::vector<myrt::geometric_object> objects;
  const auto load_obj = [&](auto path) {
    objects.clear();
    auto par = path.parent_path();
    auto o = myrt::obj::load_obj(path);
    for (const auto& obj : o) {
      auto tri = myrt::obj::triangulate(obj);

      for (const auto m : tri) {
        if (m.indices.empty())
          continue;

        auto mesh = scene.push_geometry(m.indices, {(rnu::vec3*)m.positions[0].data(), m.positions.size()},
            {(rnu::vec3*)m.normals[0].data(), m.positions.size()},
            {(rnu::vec2*)m.texcoords[0].data(), m.positions.size()});

        auto mat = m.material;

        myrt::basic_material pbrmat0(pbr_material_type);
        pbrmat0.set("albedo", rnu::vec4(mat->diffuse[0], mat->diffuse[1], mat->diffuse[2], 1));
        pbrmat0.set("ior", mat->ior);
        pbrmat0.set("roughness", std::max(0.054f, std::powf(1.f / mat->specularity, 1 / 3.1415926535897f)));
        pbrmat0.set("transmission", 1.0f - mat->dissolve);
        pbrmat0.set("albedo_texture", rnu::vec2ui(0));

        if (!mat->map_diffuse.empty()) {
          auto [texture, handle] = ldr_texture<residence::resident>(par / mat->map_diffuse);

          auto ui = rnu::vec2ui((handle)&0xffffffff, (handle >> 32) & 0xffffffff);
          pbrmat0.set("albedo_texture", ui);
        }

        auto& obj = objects.emplace_back();
        obj.name = m.name;
        obj.geometry = mesh;
        obj.material = scene.push_material(pbrmat0);
        obj.transformation = rnu::scale(rnu::vec3(import_scale, import_scale, import_scale));
      }
    }
  };
  char input_file_buf[256]{};
  auto const pstr = path.string();
  std::strncpy(input_file_buf, pstr.c_str(), pstr.length());
  load_obj(res_dir / input_file_buf);
  return objects;
}

std::uint32_t load_bokeh() {
  return ldr_texture<residence::none>(res_dir / "bokeh_gear.jpg");
}

std::pair<std::uint32_t, std::uint32_t> load_cubemap(std::filesystem::path folder) {
  uint32_t cubemap;
  uint32_t cube_sampler;
  glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &cubemap);
  glCreateSamplers(1, &cube_sampler);
  glSamplerParameterf(cube_sampler, GL_TEXTURE_MAX_ANISOTROPY, 16.f);
  glSamplerParameteri(cube_sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glSamplerParameteri(cube_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glSamplerParameteri(cube_sampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glSamplerParameteri(cube_sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glSamplerParameteri(cube_sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glSamplerParameteri(cube_sampler, GL_TEXTURE_CUBE_MAP_SEAMLESS, true);

  stbi::image_info info(res_dir / folder / "posx.hdr");

  stbi::float_image posx(res_dir / folder / "posx.hdr", 3);
  glTextureStorage3D(cubemap, 1, GL_RGB16F, info.width, info.height, 6);
  glTextureSubImage3D(cubemap, 0, 0, 0, 0, info.width, info.height, 1, GL_RGB, GL_FLOAT, posx.data.get());

  stbi::float_image negx(res_dir / folder / "negx.hdr", 3);
  glTextureSubImage3D(cubemap, 0, 0, 0, 1, info.width, info.height, 1, GL_RGB, GL_FLOAT, negx.data.get());

  stbi::float_image posy(res_dir / folder / "posy.hdr", 3);
  glTextureSubImage3D(cubemap, 0, 0, 0, 2, info.width, info.height, 1, GL_RGB, GL_FLOAT, posy.data.get());

  stbi::float_image negy(res_dir / folder / "negy.hdr", 3);
  glTextureSubImage3D(cubemap, 0, 0, 0, 3, info.width, info.height, 1, GL_RGB, GL_FLOAT, negy.data.get());

  stbi::float_image posz(res_dir / folder / "posz.hdr", 3);
  glTextureSubImage3D(cubemap, 0, 0, 0, 4, info.width, info.height, 1, GL_RGB, GL_FLOAT, posz.data.get());

  stbi::float_image negz(res_dir / folder / "negz.hdr", 3);
  glTextureSubImage3D(cubemap, 0, 0, 0, 5, info.width, info.height, 1, GL_RGB, GL_FLOAT, negz.data.get());

  uint32_t view{0};
  glGenTextures(1, &view);
  glTextureView(view, GL_TEXTURE_CUBE_MAP, cubemap, GL_RGB16F, 0, 1, 0, 6);

  return {view, cube_sampler};
}