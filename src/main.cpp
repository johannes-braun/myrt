#include <rnu/camera.hpp>
#include "pathtracer/scene.hpp"
#include "pathtracer/pathtracer.hpp"
#include "obj.hpp"
#include "sfml.hpp"
#include "gl.hpp"
#include <stb_image.h>
#include "material.hpp"

#include "pathtracer/sequential_pathtracer.hpp"

#include "thread_pool.hpp"

#include <format>

namespace stbi
{
  template<typename T>
  struct deleter {
    void operator()(T* data) const
    {
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

  template<typename T>
  using image_data = std::unique_ptr<T[], deleter<T>>;

  struct float_image : image_info
  {
    float_image(std::filesystem::path const& file, int desired_channels = 0) {
      data = image_data<float>{ stbi_loadf(file.string().c_str(), &width, &height, &channels, desired_channels) };
      channels = desired_channels;
    }

    image_data<float> data;
  };

  struct ldr_image : image_info
  {
    ldr_image(std::filesystem::path const& file, int desired_channels = 0) {
      data = image_data<stbi_uc>{ stbi_load(file.string().c_str(), &width, &height, &channels, desired_channels) };
      channels = desired_channels;
    }

    image_data<stbi_uc> data;
  };
}

const static std::filesystem::path res_dir = "../../../res";

void render_function(std::stop_token stop_token, sf::RenderWindow* window);
std::vector<myrt::geometric_object> load_object_file(myrt::scene& scene, std::filesystem::path const& path, float import_scale = 6.0f);
std::pair<GLuint, GLuint> load_cubemap(std::filesystem::path folder);
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
  myrt::thread_pool loading_pool;
  myrt::gl::start(*window);
  std::format_to(std::ostreambuf_iterator(std::cout), "{} starting...", "Rendering");

  myrt::scene scene;
  myrt::pathtracer pathtracer;

  myrt::sequential_pathtracer seq_pt;

  rnu::cameraf camera(rnu::vec3{ 0.0f, 0.0f, -15.f });
  myrt::async_resource<std::vector<myrt::geometric_object>> objects_resource(loading_pool, [&] { sf::Context context; return load_object_file(scene, "bigone.obj", 1); });
  myrt::async_resource<std::pair<GLuint, GLuint>> cube_resource(loading_pool, [] { sf::Context context; return load_cubemap("whipple_creek"); });

  GLuint bokeh = load_bokeh();
  myrt::scene::material_pointer texmat;
  myrt::sdf_object abstract_art_sdf;

  myrt::async_resource<std::vector<int>> items{ {1, 2, 3, 4, 5} };

  myrt::sdf::vertical_capsule capsule(3.f, 0.5f);
  myrt::sdf::sphere sphere(1.f);
  sphere.transform(myrt::sdf::translate(rnu::vec3(0.5, 0, 0.5)).transform(myrt::sdf::mirror_axis{ 0 }));

  myrt::sdf::smooth_union unite(0.3f);
  unite.apply(sphere, capsule);

  myrt::sdf::sphere sphere2;
  sphere2.link_parameter(sphere2[sphere.radius], sphere[sphere.radius]);
  sphere2.transform(myrt::sdf::translate(rnu::vec3(0.5, 3, 0.5)).transform(myrt::sdf::mirror_axis{ 0 }));
  auto root = myrt::sdf::smooth_union{}.apply(sphere2, unite);
  root.link_parameter(root[unite.factor], unite[unite.factor]);

  {
    GLuint texture{};
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);

    uint32_t sampler;
    glCreateSamplers(1, &sampler);
    glSamplerParameterf(sampler, GL_TEXTURE_MAX_ANISOTROPY, 16.f);
    glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_R, GL_REPEAT);
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi::ldr_image bokeh(res_dir / "uvcheck.png", 3);
    glTextureStorage2D(texture, 1, GL_RGB8, bokeh.width, bokeh.height);
    glTextureSubImage2D(texture, 0, 0, 0, bokeh.width, bokeh.height, GL_RGB, GL_UNSIGNED_BYTE, bokeh.data.get());

    auto handle = glGetTextureSamplerHandleARB(texture, sampler);
    glMakeTextureHandleResidentARB(handle);

    std::uniform_real_distribution<float> const distribution(0.0, 1.0);
    texmat = scene.push_material({
              .albedo_rgba = rnu::vec4ui8(distribution(twister) * 255, distribution(twister) * 255, distribution(twister) * 255, 255),
              .ior = distribution(twister) + 1.0f,
              .roughness = distribution(twister),
              .metallic = distribution(twister),
              .has_albedo_texture = true,
              .albedo_texture = handle
      });
  }

  abstract_art_sdf.name = "Abstract Art";
  abstract_art_sdf.sdf = scene.push_sdf(myrt::sdf_info_t{ .root = unite.get_pointer() });
  abstract_art_sdf.set(capsule.get_parameter(capsule.material), 1);
  abstract_art_sdf.set(sphere.get_parameter(sphere.material), 2);
  //scene.set_material_parameter(abstract_art_sdf.sdf, sphere2[sphere2.material], texmat);

  bool cubemap_enabled = false;
  bool bokeh_enabled = false;
  bool rr_enabled = false;
  float lens_radius = 100.f;
  int bounces_per_iteration = 8;

  int march_steps = 400;
  int march_eps_exp = 6;
  float march_eps_fac = 0.75f;

  bool show_debug = false;

  float pg_sphere_radius = 3;
  float pg_smoothness1 = 0.1f;
  rnu::vec2 torus_size(0.3, 1.2);

  GLuint fbo;
  glCreateFramebuffers(1, &fbo);

  objects_resource.wait();

  for (auto frame : myrt::gl::next_frame(*window)) {
    if (stop_token.stop_requested())
      break;

    for (auto& obj : objects_resource.iterate())
      obj->enqueue();

    abstract_art_sdf.enqueue();

    if (window->hasFocus() && !ImGui::GetIO().WantCaptureKeyboard) {
      camera.axis(float(frame.delta_time.count()) * (1.f + 5 * sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)),
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

    auto view_matrix = camera.matrix(false);
    auto proj_matrix = camera.projection(rnu::radians(70), float(window->getSize().x) / float(window->getSize().y), 0.01f, 1000.f, true);

    rnu::vec2i size(window->getSize().x, window->getSize().y);
    seq_pt.set_dof_enabled(true);
    seq_pt.set_lens_size({ 0.2, 0.2 });
    //seq_pt.set_bokeh_mask(bokeh);
    seq_pt.set_view_matrix(camera.matrix(false));
    seq_pt.set_projection_matrix(camera.projection(rnu::radians(70), float(window->getSize().x) / float(window->getSize().y), 0.01f, 1000.f, true));
    seq_pt.set_focus(focus);
    seq_pt.run(scene, size.x, size.y);

    glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, show_debug ? seq_pt.debug_texture_id() : seq_pt.color_texture_id(), 0);
    glBlitNamedFramebuffer(fbo, 0, 0, 0, size.x, size.y, 0, 0, size.x, size.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    ImGui::Begin("Settings");
    ImGui::Text("Samples: %d (%.00f sps)", seq_pt.sample_count(), 1.f / frame.delta_time.count());
    ImGui::Checkbox("show_debug", &show_debug);
    ImGui::Checkbox("Enable Cubemap", &cubemap_enabled);

    if (cubemap_enabled)
      cube_resource.current([&](auto const& cur) { seq_pt.set_cubemap(cur.first, cur.second); });
    else
      seq_pt.set_cubemap(0, 0);
    ImGui::End();

    //pathtracer.set_view(view_matrix);
    //pathtracer.set_projection(proj_matrix);
    //pathtracer.set_focus(focus);
    //pathtracer.sample_to_display(scene, window->getSize().x, window->getSize().y);

    if (ImGui::Begin("SDF Playground"))
    {
      if (ImGui::DragFloat("Sphere radius", &pg_sphere_radius, 0.01f, 0.1f, 100.f))
      {
        scene.set_parameter(abstract_art_sdf.sdf, sphere[sphere.radius], pg_sphere_radius);
      }
      if (ImGui::DragFloat("Smoothness 1", &pg_smoothness1, 0.01f, 0.0f, 100.0f))
      {
        scene.set_parameter(abstract_art_sdf.sdf, unite[unite.factor], pg_smoothness1);
      }
      if (ImGui::DragFloat2("Capsule Size", torus_size.data(), 0.01f, 0.0f, 100.0f))
      {
        scene.set_parameter(abstract_art_sdf.sdf, capsule[capsule.height], torus_size[0]);
        scene.set_parameter(abstract_art_sdf.sdf, capsule[capsule.radius], torus_size[1]);
      }
    }
    ImGui::End();
/*
    ImGui::Begin("Settings");
    ImGui::Text("Samples: %d (%.00f sps)", pathtracer.sample_count(), 1.f / frame.delta_time.count());
    if (ImGui::Button("Restart Sampling"))
    pathtracer.invalidate_counter();
    ImGui::Checkbox("Enable Cubemap", &cubemap_enabled);

    if (cubemap_enabled)
      cube_resource.current([&](auto const& cur) { pathtracer.set_cubemap(myrt::pathtracer::cubemap_texture{ cur.first, cur.second }); });
    else
      pathtracer.set_cubemap(std::nullopt);

    if (cube_resource.is_ready()) {
      if (ImGui::Button("Whipple Creek"))
      {
        cube_resource.load_resource(loading_pool, [] { sf::Context context; return load_cubemap("whipple_creek"); });
      }
      if (ImGui::Button("christmas_photo"))
      {
        cube_resource.load_resource(loading_pool, [] { sf::Context context; return load_cubemap("christmas_photo"); });
      }
    }
    else
    {
      ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.0f, 1.0f), "Loading Cubemap...");
    }
    if (ImGui::Checkbox("Enable Bokeh", &bokeh_enabled))
    {
      if (bokeh_enabled)
        pathtracer.set_bokeh_texture(bokeh);
      else
        pathtracer.set_bokeh_texture(std::nullopt);
    }
    if (ImGui::Checkbox("Enable Russian Roulette", &rr_enabled))
      pathtracer.set_enable_russian_roulette(rr_enabled);

    if (ImGui::DragInt("Bounces Per Iteration", &bounces_per_iteration, 0.1f, 1, 50))
      pathtracer.set_max_bounces(bounces_per_iteration);

    if (ImGui::DragFloat("Lens Radius", &lens_radius, 0.1f, 0.0f, 1000.0f))
      pathtracer.set_lens_radius(lens_radius);

    if (ImGui::DragInt("March Steps", &march_steps, 0.1f, 1, 10000))
      pathtracer.set_sdf_marching_steps(march_steps);

    if (ImGui::DragInt("March eps (1e-...)", &march_eps_exp, 0.1f, 0, 10))
      pathtracer.set_sdf_marching_epsilon(march_eps_fac * static_cast<float>(1.0 / std::pow(10, march_eps_exp)));
    if (ImGui::DragFloat("March eps fac", &march_eps_fac, 0.01f, 0.0f, 10.0f))
      pathtracer.set_sdf_marching_epsilon(march_eps_fac * static_cast<float>(1.0 / std::pow(10, march_eps_exp)));

    if (pathtracer.is_compiling())
    {
      ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.0f, 1.0f), "Shaders are compiling...");
    }
    else if (ImGui::Button("Reload Shaders"))
      pathtracer.reload_shaders(scene);

    ImGui::End();*/

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
      for(auto obj : objects_resource.iterate())
      {
        ImGui::PushID(obj);
        ImGui::Text("%s", obj->name.c_str());
        if (ImGui::Checkbox("Show", &obj->show))
        {
          std::cout << "map";
        }
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
  char input_file_buf[256]{};
  auto const pstr = path.string();
  std::strncpy(input_file_buf, pstr.c_str(), pstr.length());
  load_obj(res_dir / input_file_buf);
  return objects;
}

GLuint load_bokeh() {
  GLuint texture{};
  glCreateTextures(GL_TEXTURE_2D, 1, &texture);

  stbi::ldr_image bokeh(res_dir / "bokeh_gear.jpg");
  glTextureStorage2D(texture, 1, GL_RGB8, bokeh.width, bokeh.height);
  glTextureSubImage2D(texture, 0, 0, 0, bokeh.width, bokeh.height, GL_RGB, GL_UNSIGNED_BYTE, bokeh.data.get());
  return texture;
}

std::pair<GLuint, GLuint> load_cubemap(std::filesystem::path folder)
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

  uint32_t view{ 0 };
  glGenTextures(1, &view);
  glTextureView(view, GL_TEXTURE_CUBE_MAP, cubemap, GL_RGB16F, 0, 1, 0, 6);

  return { view, cube_sampler };
}