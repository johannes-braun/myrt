#include <rnu/camera.hpp>
#include "pathtracer/scene.hpp"
#include "pathtracer/pathtracer.hpp"
#include "obj.hpp"
#include "sfml.hpp"
#include "gl.hpp"
#include <stb_image.h>

const static std::filesystem::path res_dir = "../../../res";

void render_function(std::stop_token stop_token, sf::RenderWindow* window);
std::vector<myrt::geometric_object> load_object_file(myrt::scene& scene, std::filesystem::path const& path, float import_scale = 1.0f);
std::pair<GLuint, GLuint> load_cubemap();
GLuint load_bokeh();
float focus = 10.0f;

int main(int argc, char** argv) {
  sf::ContextSettings settings;
  settings.majorVersion = 4;
  settings.minorVersion = 6;
  settings.attributeFlags |= sf::ContextSettings::Debug;

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

std::mt19937 twister;
myrt::scene::material_pointer create_random_material(myrt::scene& scene)
{
  std::uniform_real_distribution<float> const distribution(0.0, 1.0);
  return scene.push_material({
            .albedo_rgba = rnu::vec4ui8(distribution(twister) * 255, distribution(twister) * 255, distribution(twister) * 255, 255),
            .ior = distribution(twister) + 1.0f,
            .roughness = distribution(twister),
            .metallic = distribution(twister)
    });
}

void render_function(std::stop_token stop_token, sf::RenderWindow* window)
{
  myrt::gl::start(*window);

  myrt::scene scene;
  myrt::pathtracer pathtracer;
  rnu::cameraf camera(rnu::vec3{ 0.0f, 0.0f, -15.f });
  auto objects = load_object_file(scene, "podium.obj");
  auto [cubemap, cube_sampler] = load_cubemap();
  auto bokeh = load_bokeh();

  myrt::sdfs::vertical_capsule capsule(3.f, 0.5f);
  myrt::sdfs::sphere sphere(1.f);
  sphere.transform(myrt::sdfs::translate(rnu::vec3(0.5, 0, 0.5)).transform(myrt::sdfs::mirror_x{}));

  myrt::sdfs::smooth_union unite(0.3f);
  unite.apply(sphere, capsule);

  myrt::sdf_object abstract_art_sdf;
  abstract_art_sdf.name = "Abstract Art";
  abstract_art_sdf.sdf = scene.push_sdf(myrt::sdf_info_t{ .root = unite.get_pointer() });
  abstract_art_sdf.set(capsule.get_parameter(capsule.material), 1);
  abstract_art_sdf.set(sphere.get_parameter(sphere.material), 2);

  bool cubemap_enabled = false;
  bool bokeh_enabled = false;
  bool rr_enabled = false;
  float lens_radius = 100.f;
  int bounces_per_iteration = 8;

  float pg_sphere_radius = 3;
  float pg_smoothness1 = 0.1f;
  rnu::vec2 torus_size(0.3, 1.2);

  for (auto frame : myrt::gl::next_frame(*window)) {
    if (stop_token.stop_requested())
      break;

    for (auto& obj : objects)
      obj.enqueue();
    abstract_art_sdf.enqueue();

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
    pathtracer.sample_to_display(scene, window->getSize().x, window->getSize().y);

    if (ImGui::Begin("SDF Playground"))
    {
      if (ImGui::DragFloat("Sphere radius", &pg_sphere_radius, 0.01f, 0.1f, 100.f))
      {
        scene.set_parameter(abstract_art_sdf.sdf, sphere.get_parameter(sphere.radius), pg_sphere_radius);
      }
      if (ImGui::DragFloat("Smoothness 1", &pg_smoothness1, 0.01f, 0.0f, 100.0f))
      {
        scene.set_parameter(abstract_art_sdf.sdf, unite.get_parameter(unite.factor), pg_smoothness1);
      }
      if (ImGui::DragFloat2("Capsule Size", torus_size.data(), 0.01f, 0.0f, 100.0f))
      {
        scene.set_parameter(abstract_art_sdf.sdf, capsule.get_parameter(capsule.height), torus_size[0]);
        scene.set_parameter(abstract_art_sdf.sdf, capsule.get_parameter(capsule.radius), torus_size[1]);
      }
    }
    ImGui::End();

    ImGui::Begin("Settings");
    ImGui::Text("Samples: %d (%.00f sps)", pathtracer.sample_count(), 1.f / frame.delta_time.count());
    if (ImGui::Button("Restart Sampling"))
      pathtracer.invalidate_counter();
    if (ImGui::Checkbox("Enable Cubemap", &cubemap_enabled))
    {
      if (cubemap_enabled)
        pathtracer.set_cubemap(myrt::pathtracer::cubemap_texture{ cubemap, cube_sampler });
      else
        pathtracer.set_cubemap(std::nullopt);
    }
     if (ImGui::Checkbox("Enable Bokeh", &bokeh_enabled))
     {
       if (bokeh_enabled)
         pathtracer.set_bokeh_texture(bokeh);
       else
         pathtracer.set_bokeh_texture(std::nullopt);
     }
    if (ImGui::Checkbox("Enable Russian Roulette", &rr_enabled))
    {
      pathtracer.set_enable_russian_roulette(rr_enabled);
    }
    if (ImGui::DragInt("Bounces Per Iteration", &bounces_per_iteration, 0.1f, 1, 50))
    {
      pathtracer.set_max_bounces(bounces_per_iteration);
    }
    if (ImGui::DragFloat("Lens Radius", &lens_radius, 0.1f, 0.0f, 1000.0f))
    {
      pathtracer.set_lens_radius(lens_radius);
    }
    if (ImGui::Button("Reload Shaders"))
    {
      pathtracer.reload_shaders(scene);
    }
    ImGui::End();

    if (ImGui::Begin("Materials")) {

      for (auto& mat : scene.materials())
      {
        ImGui::PushID(static_cast<const void*>(mat.get()));
        auto mat_info = scene.info_of(mat);

        if (ImGui::DragFloat("material.roughness", &mat_info.roughness, 0.01f, 0.0f, 1.0f))
          scene.update_material(mat, mat_info);
        if (ImGui::DragFloat("material.ior", &mat_info.ior, 0.01f, 0.01f, 100.0f))
          scene.update_material(mat, mat_info);
        if (ImGui::DragFloat("material.metallic", &mat_info.metallic, 0.01f, 0.0f, 1.0f))
          scene.update_material(mat, mat_info);
        if (ImGui::DragFloat("material.transmission", &mat_info.transmission, 0.01f, 0.0f, 1.0f))
          scene.update_material(mat, mat_info);

        rnu::vec4 col = mat_info.albedo_rgba / 255.f;
        if (ImGui::ColorEdit4("Color", col.data())) {
          mat_info.albedo_rgba = col * 255.f;
          scene.update_material(mat, mat_info);
        }

        ImGui::Separator();
        ImGui::PopID();
      }

    }
    ImGui::End();

    if (ImGui::Begin("Objects"))
    {
      for (auto& obj : objects)
      {
        ImGui::PushID(&obj);
        ImGui::Text("%s", obj.name.c_str());
        ImGui::Checkbox("Show", &obj.show);
        ImGui::PopID();
      }
      {
        ImGui::PushID(&abstract_art_sdf);
        ImGui::Text("%s", abstract_art_sdf.name.c_str());
        ImGui::Checkbox("Show", &abstract_art_sdf.show);
        ImGui::PopID();
      }
    }
    ImGui::End();
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
  char input_file_buf[256] = "podium.obj";
  load_obj(res_dir / input_file_buf);
  return objects;
}

GLuint load_bokeh() {
  GLuint bokeh{};
  glCreateTextures(GL_TEXTURE_2D, 1, &bokeh);
  int bw{}, bh{}, bc{};
  stbi_uc* img_data = stbi_load((res_dir / "bokeh_hexagon.jpg").string().c_str(), &bw, &bh, &bc, 3);
  glTextureStorage2D(bokeh, 1, GL_RGB8, bw, bh);
  glTextureSubImage2D(bokeh, 0, 0, 0, bw, bh, GL_RGB, GL_UNSIGNED_BYTE, img_data);
  stbi_image_free(img_data);
  return bokeh;
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