#include <rnu/camera.hpp>
#include "pathtracer/scene.hpp"
#include "pathtracer/pathtracer.hpp"
#include "obj.hpp"
#include "sfml.hpp"
#include "gl.hpp"
#include <stb_image.h>

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



int main(int argc, char** argv) {
  /*std::string str = "float buf[" + std::to_string(host.buf.size()) + "] = float[" + std::to_string(host.buf.size()) + "](";
  for (auto const& el : host.buf)
    str += std::to_string(el) + ",";
  if(!host.buf.empty())
    str.pop_back();
  str += ");\n" + host.glsl_string;*/
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

  // Step 1: build sdf scene
  myrt::sdfs::torus torus1;
  myrt::sdfs::sphere sphere2;
  myrt::sdfs::sphere sphere3;
  myrt::sdfs::menger_fractal fractal;

  myrt::sdfs::translate fractal_offset;
  myrt::sdfs::translate torus1_top;
  myrt::sdfs::translate sphere2_middle;
  myrt::sdfs::translate sphere3_bottom;

  fractal.transform(fractal_offset);
  torus1.transform(torus1_top);
  sphere2.transform(sphere2_middle);
  sphere3.transform(sphere3_bottom);

  myrt::sdfs::hard_union unite_torus_fractal;
  unite_torus_fractal.set_left(torus1).set_right(fractal);

  myrt::sdfs::smooth_union unite_1_2;
  unite_1_2.set_left(unite_torus_fractal).set_right(sphere2);

  myrt::sdfs::smooth_union unite_12_3;
  unite_12_3.set_left(unite_1_2).set_right(sphere3);

  float focus = 10.0f;
  myrt::scene scene;
  myrt::pathtracer pathtracer;
  rnu::cameraf camera(rnu::vec3{ 0.0f, 0.0f, -15.f });
  auto objects = load_object_file(scene, "podium.obj");
  auto [cubemap, cube_sampler] = load_cubemap();

  myrt::sdf_object abstract_art_sdf;
  abstract_art_sdf.name = "Abstract Art";
  abstract_art_sdf.sdf = scene.push_sdf(myrt::sdf_info_t{ .root = unite_12_3.get_pointer() });


  auto& host = scene.lock_sdf_host(abstract_art_sdf.sdf);
  // Step 2: set parameter values
  torus1.set(myrt::sdfs::torus::material, host, 0);
  sphere2.set(myrt::sdfs::sphere::material, host, 1);
  sphere3.set(myrt::sdfs::sphere::material, host, 2);
  fractal.set(myrt::sdfs::menger_fractal::material, host, 3);
  torus1.set(myrt::sdfs::torus::radius_small, host, 0.3f);
  torus1.set(myrt::sdfs::torus::radius_large, host, 1.2f);
  sphere2.set(myrt::sdfs::sphere::radius, host, 0.8f);
  sphere3.set(myrt::sdfs::sphere::radius, host, 1.3f);
  fractal.set(myrt::sdfs::menger_fractal::size, host, rnu::vec3(1, 1, 1));
  torus1_top.set(myrt::sdfs::translate::offset, host, rnu::vec3(0.8, 1.9, 0));
  fractal_offset.set(myrt::sdfs::translate::offset, host, rnu::vec3(1.9, 2.1, 0));
  sphere2_middle.set(myrt::sdfs::translate::offset, host, rnu::vec3(0, 0.95, 0));
  sphere3_bottom.set(myrt::sdfs::translate::offset, host, rnu::vec3(0, -0.6, 0));
  unite_1_2.set(myrt::sdfs::smooth_union::factor, host, 0.1f);
  unite_12_3.set(myrt::sdfs::smooth_union::factor, host, 0.1f);


  bool cubemap_enabled = false;
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
      if (ImGui::DragFloat("Sphere radius", &pg_sphere_radius, 0.1f, 0.1f, 100.f))
      {
        scene.set_parameter(abstract_art_sdf.sdf, sphere3.get_parameter(sphere3.radius), pg_sphere_radius);
      }
      if (ImGui::DragFloat("Smoothness 1", &pg_smoothness1, 0.01f, 0.0f, 100.0f))
      {
        scene.set_parameter(abstract_art_sdf.sdf, unite_12_3.get_parameter(unite_12_3.factor), pg_smoothness1);
      }
      if (ImGui::DragFloat2("Torus Size", torus_size.data(), 0.01f, 0.0f, 100.0f))
      {
        scene.set_parameter(abstract_art_sdf.sdf, torus1.get_parameter(torus1.radius_small), torus_size[0]);
        scene.set_parameter(abstract_art_sdf.sdf, torus1.get_parameter(torus1.radius_large), torus_size[1]);
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
   /* if (ImGui::Checkbox("Enable Bokeh", &bokeh_enabled))
    {
      if (bokeh_enabled)
        pathtracer.set_bokeh_texture(bokeh);
      else
        pathtracer.set_bokeh_texture(std::nullopt);
    }*/
    if (ImGui::Checkbox("Enable Russian Roulette", &rr_enabled))
    {
      pathtracer.set_enable_russian_roulette(rr_enabled);
    }
    //ImGui::DragInt("Samples Per Iteration", &samples_per_iteration, 0.1f, 1, 10);
    if (ImGui::DragInt("Bounces Per Iteration", &bounces_per_iteration, 0.1f, 1, 50))
    {
      pathtracer.set_max_bounces(bounces_per_iteration);
    }
    if (ImGui::DragFloat("Lens Radius", &lens_radius, 0.1f, 0.0f, 1000.0f))
    {
      pathtracer.set_lens_radius(lens_radius);
    }
    //ImGui::Checkbox("Enable Animation", &animate);
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