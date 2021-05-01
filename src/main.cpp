#include <rnu/camera.hpp>
#include "pathtracer/scene.hpp"
#include "pathtracer/pathtracer.hpp"
#include "obj.hpp"
#include "sfml.hpp"
#include "gl.hpp"
#include <stb_image.h>

#include "sdf.hpp"

const static std::filesystem::path res_dir = "../../../res";
/*
*
* #define MAT material_info_t
* MAT load_material(int i) { return mat_buf[i];}
* MAT mix_material(MAT a, MAT b, float fac) { return ...; }

float prim(vec3 p, inout MAT in_material)
{
  // code...
  material = load_material(in_param0);
}

float op(float d0, float d1, MAT m0, MAT m1, out MAT omat)
{

}

map(vec3, out MAT mat)
{

}

*/

void render_function(std::stop_token stop_token, sf::RenderWindow* window);
std::vector<myrt::geometric_object> load_object_file(myrt::scene& scene, std::filesystem::path const& path, float import_scale = 1.0f);
std::pair<GLuint, GLuint> load_cubemap();


struct sdf_host {
  template<typename T>
  sdf_host(std::shared_ptr<T> const& root) requires std::is_base_of_v<myrt::sdf_instruction, T> {
    glsl_string = myrt::generate_glsl(root, offsets);
    buf.resize(std::max_element(begin(offsets), end(offsets), [](auto const& pair, auto const& p2) { return pair.second < p2.second; })->second + 1);
  }

  void set_value(std::shared_ptr<myrt::sdf_parameter> const& param, float* data) {
    for (size_t i = 0; i < param->type->buffer_blocks; ++i)
    {
      myrt::sdf_parameter_link self{ param, i };
      auto const& link = param->get_link(i);
      if (!link.is_linked())
      {
        buf[offsets.at(self.hash())] = data[i];
      }
    }
  }
  void get_value(myrt::sdf_parameter_link const& self, float* data) const {
    auto const& link = self.other->get_link(self.block);
    if (!link.is_linked())
    {
      data[0] = buf[offsets.at(self.hash())];
    }
    else
    {
      get_value(link, data);
    }
  }
  void get_value(std::shared_ptr<myrt::sdf_parameter> const& param, float* data) const {
    for (size_t i = 0; i < param->type->buffer_blocks; ++i)
    {
      myrt::sdf_parameter_link self{ param, i };
      get_value(self, data + i);
    }
  }
  void set_value(std::shared_ptr<myrt::int_param> const& param, int data) {
    float cast = float(data);
    set_value(param, &cast);
  }
  void set_value(std::shared_ptr<myrt::vec3_param> const& param, rnu::vec3 data) {
    set_value(param, data.data());
  }
  void set_value(std::shared_ptr<myrt::float_param> const& param, float data) {
    set_value(param, &data);
  }

  void get_value(std::shared_ptr<myrt::int_param> const& param, int& data) const {
    float cast = float(data);
    get_value(param, &cast);
    data = int(cast);
  }
  void get_value(std::shared_ptr<myrt::vec3_param> const& param, rnu::vec3& data) const {
    get_value(param, data.data());
  }
  void get_value(std::shared_ptr<myrt::float_param> const& param, float& data) const {
    get_value(param, &data);
  }

  template<typename T, typename Param>
  T get_value(std::shared_ptr<Param> const& param) const requires std::is_base_of_v<myrt::sdf_parameter, Param>{
    T value;
    get_value(param, value);
    return value;
  }

  std::string glsl_string;
  std::unordered_map<size_t, int> offsets;
  std::vector<float> buf;
};

int main(int argc, char** argv) {

  auto material = myrt::make_int_param();
  auto material2 = myrt::make_int_param();

  auto sphere_radius = myrt::make_float_param();

  auto sphere1 = myrt::sdfs::sphere(sphere_radius, material);

  auto box_size = myrt::make_vec3_param();
  box_size->link_value_block(0, sphere_radius, 0);
  auto sphere2 = myrt::sdfs::box_rounded(box_size, myrt::make_float_param(), material2);

  auto union_smoothness = myrt::make_float_param();
  auto sunion = myrt::sdfs::smooth_union(union_smoothness);
  sunion->set_lhs(sphere1);
  sunion->set_rhs(sphere2);

  auto soff = myrt::sdfs::translate(myrt::make_vec3_param());
  auto soff2 = myrt::sdfs::translate(myrt::make_vec3_param());

  sphere1->set_parent(soff);
  soff2->set_parent(soff);
  sphere2->set_parent(soff2);

  sdf_host host(sunion);
  host.set_value(sphere_radius, 2.0);
  host.set_value(union_smoothness, 0.2);
  host.set_value(box_size, rnu::vec3(1, 1, 1));
  host.set_value(material, 0);
  host.set_value(material2, 1);

  auto val = host.get_value<rnu::vec3>(box_size);

  std::string str;
  for (auto const& el : host.buf)
    str += std::to_string(el) + ", ";
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