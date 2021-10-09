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

#include "types.hpp"
#include "material.hpp"

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

auto pbr_material_glsl = std::string("#include <pbr_material.glsl>\n");

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

struct bouncer_component : myrt::component<bouncer_component> {
  float radius = 1;
  float current_speed = 0;
};

struct bouncer_system : public myrt::typed_system<transform_component, bouncer_component> {
public:
  void update(duration_type delta, transform_component* t, bouncer_component* b) const override {
    b->current_speed -= 9.81 * delta.count();
    t->position.y += b->current_speed * delta.count();
    if (t->position.y < b->radius) {
      t->position = rnu::vec3(t->position.x, b->radius, t->position.y);
      b->current_speed = -b->current_speed;
    }
  }
};

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
      ImGui::Text("Framerate: %.00f fps", 1.f / dt);
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

struct color_object_type : myrt::object_type {
public:
  color_object_type(myrt::types_registry& registry) : object_type("color_t") {
    add_parameter("value", myrt::parameter_type_id::type_vec4unorm8);
  }
};

struct material_object_type : myrt::object_type {
public:
  material_object_type(myrt::types_registry& registry) : myrt::object_type("material") {
    add_parameter("albedo", registry.find("color_t"));
    add_parameter("ior", myrt::parameter_type_id::type_float);
    add_parameter("roughness", myrt::parameter_type_id::type_float);
    add_parameter("metallic", myrt::parameter_type_id::type_float);
    add_parameter("transmission", myrt::parameter_type_id::type_float);
    add_parameter("albedo_texture", myrt::parameter_type_id::type_uvec2);
  }
};

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

static myrt::material_buffer::material_type_t pbr_material_type;

//
//static myrt::basic_material_type pbr_material_type = [] {
//  std::map<std::string, std::shared_ptr<myrt::parameter_type>> pars;
//  nlohmann::json j = nlohmann::json::parse(R"(
//{
//  "parameters": {
//    "albedo": "vec4",
//    "ior": "float",
//    "roughness": "float",
//    "metallic": "float",
//    "transmission": "float",
//    "albedo_texture": "uvec2"
//  },
//  "source": "${sourceDir}/file.glsl"
//}
//)");
//  for (auto const& [key, value] : j["parameters"].get<nlohmann::json::object_t>()) {
//    pars[key] = myrt::type_registry[value];
//  }
//  return myrt::basic_material_type(pars, pbr_material_glsl);
//}();

std::vector<myrt::geometric_object> gen_terrain(myrt::scene& scene) {
  int size = 128;

  std::vector<float> heights(size * size);
  std::generate(begin(heights), end(heights), [] { return 0.f; });
  std::vector<rnu::vec3> corner_normals(size * size);

  auto height_at = [&](int x, int z) -> float& { return heights[x + z * size]; };

  std::vector<std::uint32_t> indices;
  std::vector<rnu::vec3> positions;
  std::vector<rnu::vec3> normals;
  std::vector<rnu::vec2> uvs;

  for (int i = 0; i < size - 1; ++i) {
    for (int j = 0; j < size - 1; ++j) {
      rnu::vec3 a00(i, height_at(i, j), j);
      rnu::vec3 a10(i + 1, height_at(i + 1, j), j);
      rnu::vec3 a01(i, height_at(i, j + 1), j + 1);
      rnu::vec3 a11(i + 1, height_at(i + 1, j + 1), j + 1);

      rnu::vec2 u00(0, 0);
      rnu::vec2 u10(0 + 1, 0);
      rnu::vec2 u01(0, 0 + 1);
      rnu::vec2 u11(0 + 1, 0 + 1);

      std::uint32_t const base_index = positions.size();

      uvs.push_back(u00);
      uvs.push_back(u10);
      uvs.push_back(u01);
      uvs.push_back(u11);

      positions.push_back(a00);
      positions.push_back(a10);
      positions.push_back(a01);
      positions.push_back(a11);

      rnu::vec3 n = cross(a00 - a10, a00 - a01);
      normals.push_back(n);
      normals.push_back(n);
      normals.push_back(n);
      normals.push_back(n);

      indices.push_back(base_index + 0);
      indices.push_back(base_index + 1);
      indices.push_back(base_index + 2);
      indices.push_back(base_index + 2);
      indices.push_back(base_index + 1);
      indices.push_back(base_index + 3);
    }
  }

  auto pbrmat0 = scene.push_material(pbr_material_type);
  scene.set_parameter(pbrmat0, "albedo", rnu::vec4ui8(255*rnu::vec4(0.2, 0.6, 0.2, 1)));
  scene.set_parameter(pbrmat0, "ior", 1.4f);
  scene.set_parameter(pbrmat0, "roughness", 0.8f);
  scene.set_parameter(pbrmat0, "transmission", 0.f);
  scene.set_parameter(pbrmat0, "albedo_texture", rnu::vec2ui(0));

  auto gob = scene.push_geometry(indices, positions, normals, uvs);

  myrt::geometric_object obj;
  obj.name = "Terrain";
  obj.geometry = gob;
  obj.material = pbrmat0;
  obj.transformation = rnu::translation(rnu::vec3(-size / 2, 0, -size / 2));
  return {obj};
}

void render_function(std::stop_token stop_token, sf::RenderWindow* window) {
  myrt::thread_pool loading_pool;
  myrt::gl::start(*window);
  std::format_to(std::ostreambuf_iterator(std::cout), "{} starting...", "Rendering");

  //myrt::types_registry registry;
  //registry.create<color_object_type>();
  //registry.create<material_object_type>();

  ////nlohmann::json j = nlohmann::json::parse(type_json);
  ////auto dynamic_type = std::make_shared<myrt::object_type>(j["name"]);
  ////for (auto const& x : j["members"]) {
  ////  for (auto const& [key, value] : x.get<nlohmann::json::object_t>()) {
  ////    dynamic_type->add_parameter(key, registry.find(value));
  ////  }
  ////}
  ////registry.push_back(std::move(dynamic_type));

  ////auto const genc = myrt::generate_object_loaders(registry);

  //buffer << t.rdbuf();
  //myrt::material_registry mat_reg;
  //pbr_material_type = mat_reg.push_type(registry.find_object_type("material"), buffer.str());
  //mat_reg.push_type(registry.find_object_type("color_t"), buffer.str());




  //auto glslsl = myrt::generate_object_loaders(registry) + mat_reg.generate_code();


  myrt::scene scene;
  myrt::sequential_pathtracer pathtracer_renderer;
  myrt::forward_renderer forward_renderer;

  std::ifstream t(res_dir / "../src/pt/material_new.glsl");
  std::stringstream buffer;
  buffer << t.rdbuf();

  scene.register_type<color_object_type>();
  pbr_material_type = scene.push_material_type(scene.register_type<material_object_type>(), buffer.str());


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
  bouncer_system bouncer_system;

  myrt::system_list systems;
  systems.add(object_system);
  systems.add(bouncer_system);
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

  render_entity->get<camera_component>()->camera = {rnu::vec3{0.0f, 1.0f, 3.f}};

  auto const import_scale = 1;

  object_component instance{.object = load_object_file(scene, "ball.obj", 1)};
  entities.push_back(ecs.create_entity(instance, transform_component{.position = rnu::vec3(0, 3, 0), .scale = rnu::vec3(0.1f)},
          object_ui_component{}, bouncer_component{.radius=0.1f}));

  object_component terrain{.object = gen_terrain(scene)};
  entities.push_back(ecs.create_entity(terrain, transform_component{}, object_ui_component{}));

  object_component fsf{.object = load_object_file(scene, "fischl/fischl.obj", 1)};
  entities.push_back(
      ecs.create_entity(fsf, transform_component{.position = rnu::vec3(0, 0, 1)},
          object_ui_component{}));

  
  object_component grass{.object = load_object_file(scene, "grass.obj", 1)};
  
  std::mt19937 gens;
  std::uniform_real_distribution<float> dists(-64, 64);

  for (int i = 0; i < 1000; ++i) {
    auto pbrmat0 = scene.push_material(pbr_material_type);
    scene.set_parameter(pbrmat0, "albedo", rnu::vec4ui8(255*rnu::vec4(0.2, 0.6, 0.2, 1)));
    scene.set_parameter(pbrmat0, "ior", 1.4f);
    scene.set_parameter(pbrmat0, "roughness", 0.8f);
    scene.set_parameter(pbrmat0, "transmission", 0.f);
    scene.set_parameter(pbrmat0, "albedo_texture", rnu::vec2ui(0));
    std::get<std::vector<myrt::geometric_object>>(grass.object)[0].material = pbrmat0;
    entities.push_back(
        ecs.create_entity(grass, transform_component{.position = rnu::vec3(dists(gens), 0, dists(gens))}));
  }

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

    object_component instance{.object = std::move(abstract_art_sdf)};
    entities.push_back(ecs.create_entity(instance, transform_component{.scale = {1}}, object_ui_component{}));
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
  myrt::linear_scale down_scale;

  for (auto frame : myrt::gl::next_frame(*window)) {
    if (stop_token.stop_requested())
      break;
    global_texture_provider.new_frame();
    rnu::vec2i size(window->getSize().x, window->getSize().y);

    static float super_sampling = 1.25;

    {
      render_entity->get<camera_component>()->width = super_sampling * size.x;
      render_entity->get<camera_component>()->height = super_sampling * size.y;
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
          .direction = rnu::normalize<rnu::vec3>(rnu::vec3(vd)),
          .length = std::numeric_limits<float>::max()};
      auto picked = scene.pick(ray);
     /* if (picked) {
        scene.set_parameter(scene.materials()[picked->drawable->material_index], "metallic", 1.0f);
        scene.set_parameter(scene.materials()[picked->drawable->material_index], "roughness", 0.22f);
        scene.set_parameter(
            scene.materials()[picked->drawable->material_index], "albedo", rnu::vec4(1.f, 0.4f, 0.1f, 1));
        scene.set_parameter(scene.materials()[picked->drawable->material_index], "albedo_texture", rnu::vec2ui(0));
        printf("Picked object: #%zd at distance %.5f\n", picked->index, picked->t);
      }*/
    }

    static float scaling1 = 2.f;
    static float scaling2 = 8.f;
    static float fac1 = 0.3f;
    static float fac2 = 0.15f;
    static float step = 1.25f;

    if (ImGui::Begin("PP")) {
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
    down_scale.factor = 1.f / super_sampling;
    result = down_scale.process(global_texture_provider, result->id());
    result = bloom.process(global_texture_provider, result->id());
    result = tonemap.process(global_texture_provider, result->id());
    result = fxaa.process(global_texture_provider, result->id());
    glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, result->id(), 0);
    glBlitNamedFramebuffer(fbo, 0, 0, 0, size.x, size.y, 0, 0, size.x, size.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    if (ImGui::Begin("Settings")) {
      ImGui::DragFloat("SuperSampling", &super_sampling, 0.001f, 0.01f, 4.f);
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
  glTextureStorage2D(texture, 8, GL_RGB8, image.width, image.height);
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
  /*std::map<std::string, std::shared_ptr<myrt::parameter_type>> pars;
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

  myrt::basic_material_type pbr_material_type(pars, pbr_material_glsl);*/

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

        auto pbrmat0 = scene.push_material(pbr_material_type);
        scene.set_parameter(pbrmat0, "albedo", rnu::vec4ui8(255*rnu::vec4(mat->diffuse[0], mat->diffuse[1], mat->diffuse[2], 1)));
        scene.set_parameter(pbrmat0, "ior", mat->ior);
        scene.set_parameter(pbrmat0, "roughness", std::max(0.054f, std::powf(1.f / mat->specularity, 1 / 3.1415926535897f)));
        scene.set_parameter(pbrmat0, "transmission", 1.0f - mat->dissolve);
        scene.set_parameter(pbrmat0, "albedo_texture", rnu::vec2ui(0));

        if (!mat->map_diffuse.empty()) {
          auto [texture, handle] = ldr_texture<residence::resident>(par / mat->map_diffuse);

          auto ui = rnu::vec2ui((handle)&0xffffffff, (handle >> 32) & 0xffffffff);
          scene.set_parameter(pbrmat0, "albedo_texture", ui);
        }

        auto& obj = objects.emplace_back();
        obj.name = m.name;
        obj.geometry = mesh;
        obj.material = pbrmat0;
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