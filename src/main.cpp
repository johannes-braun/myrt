#include <rnu/camera.hpp>
#include "pathtracer/scene.hpp"
#include "pathtracer/pathtracer.hpp"
#include "obj.hpp"
#include "sfml.hpp"
#include "gl.hpp"
#include <stb_image.h>

#include "sdf.hpp"

namespace myrt {
  inline std::string generate_glsl(
    std::shared_ptr<sdf_instruction> const& root,
    std::stringstream& functions,
    std::stringstream& distance_function,
    std::stringstream& mul,
    std::unordered_set<size_t>& used_functions,
    std::unordered_set<size_t>& used_objects,
    std::unordered_set<sdf_param::type>& used_param_types,
    std::unordered_map<std::shared_ptr<sdf_param>, int>& buffer_offsets,
    int& current_offset)
  {
    const auto hash = std::hash<std::string>{}(root->glsl_string);
    const auto fun_name = "f" + std::to_string(hash);
    if (used_functions.emplace(hash).second) {

      for (auto const& par : root->params)
      {
        if (used_param_types.emplace(par->param_type).second)
          functions << get_function_string(*par);
      }

      switch (root->instruction_type)
      {
      case sdf_instruction::type::prim:
        functions << "float " << fun_name << "(vec3 in_position";
        break;
      case sdf_instruction::type::op:
        functions << "float " << fun_name << "(float in_distance0, float in_distance1";
        break;
      case sdf_instruction::type::mod:
        functions << "vec3 " << fun_name << "(vec3 in_position, out float out_multiplier";
        break;
      }

      int i = 0;
      for (auto const& par : root->params)
        functions << "," << par->name() << " in_param" << i++;

      functions << "){" << root->glsl_string << "}";
    }

    switch (root->instruction_type)
    {
    case sdf_instruction::type::prim:
    {
      auto h = std::hash<std::shared_ptr<sdf_instruction>>{}(root);
      auto const dname = "p" + std::to_string(h);
      if (used_objects.emplace(h).second) {
        int off = current_offset;
        std::vector<std::string> param_names;
        for (auto const& par : root->params)
        {
          auto param_hash = std::hash<std::shared_ptr<sdf_param>>{}(par);
          std::string param_name = "par" + std::to_string(param_hash);
          if (used_objects.emplace(param_hash).second) {
            buffer_offsets[par] = current_offset;
            distance_function << par->name() << " " << param_name << "=sdrd_" << par->name() << "(" << current_offset << ");";
            current_offset += par->blocks();
          }
          param_names.push_back(param_name);
        }
        auto const p = std::static_pointer_cast<sdf_prim>(root);
        std::string in_pos = "in_position";
        if (!p->parent.expired())
        {
          auto parent = p->parent.lock();
          in_pos = generate_glsl(parent, functions, distance_function, mul, used_functions, used_objects, used_param_types, buffer_offsets, current_offset);
        }
        distance_function << "float " << dname << "=" << fun_name << "(" << in_pos;

        for (auto const& n : param_names)
          distance_function << "," << n;
        distance_function << ");";
      }
      return dname;
    }
    break;
    case sdf_instruction::type::op:
    {
      auto h = std::hash<std::shared_ptr<sdf_instruction>>{}(root);
      auto const dname = "o" + std::to_string(h);
      if (used_objects.emplace(h).second) {
        int off = current_offset;
        std::vector<std::string> param_names;
        for (auto const& par : root->params)
        {
          auto param_hash = std::hash<std::shared_ptr<sdf_param>>{}(par);
          std::string param_name = "par" + std::to_string(param_hash);
          if (used_objects.emplace(param_hash).second) {
            buffer_offsets[par] = current_offset;
            distance_function << par->name() << " " << param_name << "=sdrd_" << par->name() << "(" << current_offset << ");";
            current_offset += par->blocks();
          }
          param_names.push_back(param_name);
        }
        auto const p = std::static_pointer_cast<sdf_op>(root);
        std::string in_pos1 = "in_position";
        std::string in_pos2 = "in_position";
        if (!p->lhs.expired())
        {
          auto parent = p->lhs.lock();
          in_pos1 = generate_glsl(parent, functions, distance_function, mul, used_functions, used_objects, used_param_types, buffer_offsets, current_offset);
        }
        if (!p->rhs.expired())
        {
          auto parent = p->rhs.lock();
          in_pos2 = generate_glsl(parent, functions, distance_function, mul, used_functions, used_objects, used_param_types, buffer_offsets, current_offset);
        }
        distance_function << "float " << dname << "=" << fun_name << "(" << in_pos1 << "," << in_pos2;

        for (auto const& n : param_names)
          distance_function << "," << n;
        distance_function << ");";
      }
      return dname;
    }
    break;
    case sdf_instruction::type::mod:
    {
      auto h = std::hash<std::shared_ptr<sdf_instruction>>{}(root);
      auto const dname = "m" + std::to_string(h);
      auto mul_name = "j" + dname;
      mul << "*" << mul_name;

      if (used_objects.emplace(h).second) {
        distance_function << "float " << mul_name << "=1.0;";
        int off = current_offset;
        std::vector<std::string> param_names;
        for (auto const& par : root->params)
        {
          auto param_hash = std::hash<std::shared_ptr<sdf_param>>{}(par);
          std::string param_name = "par" + std::to_string(param_hash);
          if (used_objects.emplace(param_hash).second) {
            buffer_offsets[par] = current_offset;
            distance_function << par->name() << " " << param_name << "=sdrd_" << par->name() << "(" << current_offset << ");";
            current_offset += par->blocks();
          }
          param_names.push_back(param_name);
        }
        auto const p = std::static_pointer_cast<sdf_mod>(root);
        std::string in_pos = "in_position";
        if (!p->parent.expired())
        {
          auto parent = p->parent.lock();
          in_pos = generate_glsl(parent, functions, distance_function, mul, used_functions, used_objects, used_param_types, buffer_offsets, current_offset);
        }
        distance_function << "vec3 " << dname << "=" << fun_name << "(" << in_pos << "," << mul_name;

        for (auto const& n : param_names)
          distance_function << "," << n;
        distance_function << ");";
      }
      return dname;
    }
    break;
  }
}

inline std::string generate_glsl(std::shared_ptr<sdf_instruction> const& root, std::unordered_map<std::shared_ptr<sdf_param>, int>& buffer_offsets)
{
  std::stringstream functions;
  std::stringstream mul;
  mul << "1.0";
  std::stringstream distance_function;
  std::unordered_set<size_t> used_functions;
  std::unordered_set<size_t> used_objects;
  std::unordered_set<sdf_param::type> used_param_types;
  int current_offset = 0;
  auto d = generate_glsl(root, functions, distance_function, mul, used_functions, used_objects, used_param_types, buffer_offsets, current_offset);

  return functions.str() + "float map(vec3 in_position) { int in_buffer_offset=0;" + distance_function.str() + "return " + mul.str() + "*" + d + ";}";
}

template<typename T> requires std::is_base_of_v<sdf_instruction, T>
  inline std::string generate_glsl(std::shared_ptr<T> const& root, std::unordered_map<std::shared_ptr<sdf_param>, int>& buffer_offsets)
  {
    return generate_glsl(std::static_pointer_cast<sdf_instruction>(root), buffer_offsets);
  }
}

const static std::filesystem::path res_dir = "../../../res";

void render_function(std::stop_token stop_token, sf::RenderWindow* window);
std::vector<myrt::geometric_object> load_object_file(myrt::scene& scene, std::filesystem::path const& path, float import_scale = 1.0f);
std::pair<GLuint, GLuint> load_cubemap();

int main(int argc, char** argv) {

  auto param = myrt::make_float_param();

  auto sphere1 = myrt::sphere(param);
  auto sphere2 = myrt::sphere(param);

  auto sunion = myrt::smooth_union(param);
  sunion->set_lhs(sphere1);
  sunion->set_rhs(sphere2);

  auto soff = myrt::translate(myrt::make_vec3_param());
  auto soff2 = myrt::translate(myrt::make_vec3_param());

  sphere1->set_parent(soff);
  soff2->set_parent(soff);
  sphere2->set_parent(soff2);

  std::unordered_map<std::shared_ptr<myrt::sdf_param>, int> buffer_offsets;
  auto test = myrt::generate_glsl(sunion, buffer_offsets);

  // todo: "generate aabbs?"
  //  -> add to bvh?
  //  -> aabb per sdf?
  //  -> 
  // todo: scene.create_sdf() (from struct/json/whatever)
  // todo: pathtracer.invalidate_shader(), wenn sdf changed.
  // todo: scene.set_sdf_parameter(shared_ptr<param>, Ty&& value);

  sf::ContextSettings settings;
  settings.majorVersion = 4;
  settings.minorVersion = 6;
  settings.attributeFlags |= sf::ContextSettings::Debug;

  sf::RenderWindow window(sf::VideoMode(1600, 1200), "MyRT", sf::Style::Default, settings);
  window.setActive(false);

  std::jthread render_thread(&render_function, &window);

  for (auto event : myrt::sfml::poll_event(window)) {

  }
  render_thread.request_stop();
}

void render_function(std::stop_token stop_token, sf::RenderWindow* window)
{
  myrt::gl::start(*window);

  float focus = 10.0f;
  myrt::scene scene;
  myrt::pathtracer pathtracer;
  rnu::cameraf camera(rnu::vec3{ 0.0f, 0.0f, -15.f });
  auto const objects = load_object_file(scene, "plane.obj");
  auto [cubemap, cube_sampler] = load_cubemap();

  for (auto frame : myrt::gl::next_frame(*window)) {
    if (stop_token.stop_requested())
      break;

    for (auto& obj : objects)
      obj.enqueue();

    if (window->hasFocus() && !ImGui::GetIO().WantCaptureKeyboard) {
      camera.axis(frame.delta_time.count() * (1.f + 5 * sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)),
        sf::Keyboard::isKeyPressed(sf::Keyboard::W),
        sf::Keyboard::isKeyPressed(sf::Keyboard::S),
        sf::Keyboard::isKeyPressed(sf::Keyboard::A),
        sf::Keyboard::isKeyPressed(sf::Keyboard::D),
        sf::Keyboard::isKeyPressed(sf::Keyboard::E),
        sf::Keyboard::isKeyPressed(sf::Keyboard::Q));
    }
    if (window->hasFocus() && !ImGui::GetIO().WantCaptureMouse) {
      camera.mouse(
        float(sf::Mouse::getPosition().x),
        float(sf::Mouse::getPosition().y),
        sf::Mouse::isButtonPressed(sf::Mouse::Left));
    }

    auto view_matrix = camera.matrix(true);
    auto proj_matrix = camera.projection(rnu::radians(70), float(window->getSize().x) / float(window->getSize().y), 0.01f, 1000.f, true);
    pathtracer.set_view(view_matrix);
    pathtracer.set_projection(proj_matrix);
    pathtracer.set_focus(focus);
    pathtracer.set_enable_russian_roulette(true);
    pathtracer.set_cubemap(myrt::pathtracer::cubemap_texture{ cubemap, cube_sampler });
    pathtracer.sample_to_display(scene, window->getSize().x, window->getSize().y);
  }
}

std::vector<myrt::geometric_object> load_object_file(myrt::scene& scene, std::filesystem::path const& path, float import_scale)
{
  std::vector<myrt::geometric_object> objects;
  const auto load_obj = [&](auto path) {
    objects.clear();
    auto o = myrt::obj::load_obj(path);
    for (const auto& obj : o)
    {
      auto tri = myrt::obj::triangulate(obj);

      for (const auto m : tri)
      {
        auto mesh = scene.push_geometry(m.indices,
          { (rnu::vec3*)m.positions[0].data(), m.positions.size() },
          { (rnu::vec3*)m.normals[0].data(), m.positions.size() });

        auto mat = m.material;

        auto& obj = objects.emplace_back();
        obj.name = m.name;
        obj.geometry = mesh;
        obj.material = scene.push_material({
            .albedo_rgba = rnu::vec4ui8(mat->diffuse[0] * 255, mat->diffuse[1] * 255, mat->diffuse[2] * 255, 255),
            .ior = mat->ior,
            .roughness = std::powf(1.f / mat->specularity, 1 / 3.1415926535897f)
          });
        obj.transformation = rnu::scale(rnu::vec3(import_scale, import_scale, import_scale));
      }
    }
  };
  char input_file_buf[256] = "plane.obj";
  load_obj(res_dir / input_file_buf);
  return objects;
}

std::pair<GLuint, GLuint> load_cubemap()
{
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

  int w, h, c;
  // face 1
  float* img = stbi_loadf((res_dir / "whipple_creek/posx.hdr").string().c_str(), &w, &h, &c, 3);
  const int num_mips = static_cast<int>(1 + floor(log(float(std::max(w, h)))));
  glTextureStorage3D(cubemap, num_mips, GL_RGB16F, w, h, 6);
  glTextureSubImage3D(cubemap, 0, 0, 0, 0, w, h, 1, GL_RGB, GL_FLOAT, img);
  stbi_image_free(img);

  img = stbi_loadf((res_dir / "whipple_creek/negx.hdr").string().c_str(), &w, &h, &c, 3);
  glTextureSubImage3D(cubemap, 0, 0, 0, 1, w, h, 1, GL_RGB, GL_FLOAT, img);
  stbi_image_free(img);

  img = stbi_loadf((res_dir / "whipple_creek/posy.hdr").string().c_str(), &w, &h, &c, 3);
  glTextureSubImage3D(cubemap, 0, 0, 0, 2, w, h, 1, GL_RGB, GL_FLOAT, img);
  stbi_image_free(img);

  img = stbi_loadf((res_dir / "whipple_creek/negy.hdr").string().c_str(), &w, &h, &c, 3);
  glTextureSubImage3D(cubemap, 0, 0, 0, 3, w, h, 1, GL_RGB, GL_FLOAT, img);
  stbi_image_free(img);

  img = stbi_loadf((res_dir / "whipple_creek/posz.hdr").string().c_str(), &w, &h, &c, 3);
  glTextureSubImage3D(cubemap, 0, 0, 0, 4, w, h, 1, GL_RGB, GL_FLOAT, img);
  stbi_image_free(img);

  img = stbi_loadf((res_dir / "whipple_creek/negz.hdr").string().c_str(), &w, &h, &c, 3);
  glTextureSubImage3D(cubemap, 0, 0, 0, 5, w, h, 1, GL_RGB, GL_FLOAT, img);
  stbi_image_free(img);
  glGenerateTextureMipmap(cubemap);
  uint32_t view{ 0 };
  glGenTextures(1, &view);
  glTextureView(view, GL_TEXTURE_CUBE_MAP, cubemap, GL_RGB16F, 0, num_mips, 0, 6);

  return { view, cube_sampler };
}