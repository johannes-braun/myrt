#include "forward_renderer.hpp"

#include <mygl/mygl.hpp>
#include <myrt/sfml/utils.hpp>
#include "bindings.hpp"
#include <filesystem>
#include <glsp/glsp.hpp>
#include <rnu/camera.hpp>

namespace myrt
{
  const static std::filesystem::path res_dir = "../../../res";

  std::shared_ptr<texture_t> forward_renderer::run(texture_provider_t& provider, scene& scene, int width, int height)
  {
    auto const prepare = scene.prepare();

    if (prepare.drawables_changed)
    {
      if (!glIsBuffer(m_multidraw_buffer))
        glCreateBuffers(1, &m_multidraw_buffer);

      std::int64_t size = 0;
      glGetNamedBufferParameteri64v(scene.get_scene_buffers().drawable_buffer, GL_BUFFER_SIZE, &size);
      m_num_multidraws = size / sizeof(scene::drawable_geometry_t);
      glNamedBufferData(m_multidraw_buffer, m_num_multidraws * sizeof(multi_draw_indirect_t), nullptr, GL_DYNAMIC_DRAW);
    }
    rnu::vec2ui const new_image_size(width, height);
    if (std::ranges::any_of(m_image_size != new_image_size, [](bool b) { return b; })) {
      m_image_size = new_image_size;
      m_gbuffer_texture = nullptr;
      m_result_texture = nullptr;
      m_depth_texture = nullptr;
    }

    if (!m_gbuffer_texture) {
      m_gbuffer_texture = provider.get(GL_TEXTURE_2D_ARRAY, GL_RGBA16F, width, height, 3, 1);
    }
    if (!m_result_texture) {
      m_result_texture = provider.get(GL_TEXTURE_2D, GL_RGBA16F, width, height, 1);
    }
    if (!m_depth_texture)
      m_depth_texture = provider.get(GL_TEXTURE_2D, GL_DEPTH24_STENCIL8, width, height, 1);
    if (!m_shadow_map)
      m_shadow_map = provider.get(GL_TEXTURE_2D, GL_RGBA16F, 2048, 2048, 1);
    if (!m_shadow_pass_depth)
      m_shadow_pass_depth = provider.get(GL_TEXTURE_2D, GL_DEPTH24_STENCIL8, 2048, 2048, 1);

    glViewport(0, 0, width, height);
    glClearColor(0,0,0,0);
    pass_gen_multidraw(scene);
    pass_render(scene);
    glViewport(0, 0, 2048, 2048);
    pass_shadow(scene);
    glViewport(0, 0, width, height);
    pass_resolve(scene);

    return m_result_texture;
  }
  void forward_renderer::set_view_matrix(rnu::mat4 const& matrix)
  {
    m_view = matrix;
  }
  void forward_renderer::set_projection_matrix(rnu::mat4 const& matrix)
  {
    m_projection = matrix;
  }
  void forward_renderer::reload_resolve_shader() {
    if (glIsProgram(m_resolve_program)) {
      glDeleteProgram(m_resolve_program);
      m_resolve_program = 0;
    }
  }
  void forward_renderer::reload_render_shader() {
    if (glIsProgram(m_render)) {
      glDeleteProgram(m_render);
      m_render = 0;
    }
  }
  void forward_renderer::create_gen_multidraw_shader()
  {
    glsp::preprocess_file_info generate_file;
    generate_file.file_path = res_dir / "../src/pt/fwd_generate.comp";
    auto const preprocessed_file = glsp::preprocess_file(generate_file);

    auto const program = make_program(preprocessed_file.contents);

    if (program) {
      if (glIsProgram(m_gen_multidraw))
        glDeleteProgram(m_gen_multidraw);

      m_gen_multidraw = program.value();
      load_gen_multidraw_bindings();
    }
  }
  void forward_renderer::load_gen_multidraw_bindings()
  {
    glUseProgram(m_gen_multidraw);
    m_gen_multidraw_bindings.geometries = if_empty(storage_buffer_binding(m_gen_multidraw, "Geometries"));
    m_gen_multidraw_bindings.multidraws = if_empty(storage_buffer_binding(m_gen_multidraw, "MultiDrawIndirect"));

    std::int32_t group_sizes[3]{};
    glGetProgramiv(m_gen_multidraw, GL_COMPUTE_WORK_GROUP_SIZE, group_sizes);
    m_gen_multidraw_group_size = group_sizes[0];
  }
  void forward_renderer::pass_gen_multidraw(scene& scene)
  {
    if (!glIsProgram(m_gen_multidraw))
      create_gen_multidraw_shader();

    glUseProgram(m_gen_multidraw);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_gen_multidraw_bindings.geometries, scene.get_scene_buffers().drawable_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_gen_multidraw_bindings.multidraws, m_multidraw_buffer);
    auto const count_x = (m_num_multidraws + m_gen_multidraw_group_size - 1) / m_gen_multidraw_group_size;
    glDispatchCompute(count_x, 1, 1);
    glMemoryBarrierByRegion(GL_SHADER_STORAGE_BARRIER_BIT);
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT);
  }
  void forward_renderer::create_render_shader()
  {
    glsp::preprocess_file_info vs_info;
    vs_info.file_path = res_dir / "../src/pt/forward/vert.vert";
    const auto vertex_shader = glsp::preprocess_file(vs_info);

    glsp::preprocess_file_info fs_info;
    fs_info.file_path = res_dir / "../src/pt/forward/frag.frag"; 
    const auto fragment_shader = glsp::preprocess_file(fs_info);

    const auto program = make_program(vertex_shader.contents, fragment_shader.contents);
    if (program) {
      if (glIsProgram(m_render))
        glDeleteProgram(m_render);

      m_render = program.value();
      load_render_bindings();
    }

    if (!glIsFramebuffer(m_framebuffer))
      glCreateFramebuffers(1, &m_framebuffer);
  }
  void forward_renderer::load_render_bindings()
  {
    glUseProgram(m_render);
    m_render_bindings.points_in = if_empty(input_location(m_render, "point"));
    m_render_bindings.normal_in = if_empty(input_location(m_render, "normal"));
    m_render_bindings.uv_in = if_empty(input_location(m_render, "uv"));
    m_render_bindings.view = if_empty(uniform_location(m_render, "view"));
    m_render_bindings.proj = if_empty(uniform_location(m_render, "proj"));
    m_render_bindings.geometries = if_empty(storage_buffer_binding(m_render, "Geometries"));

    if (!glIsVertexArray(m_vertex_array))
    {
      glCreateVertexArrays(1, &m_vertex_array);

      glEnableVertexArrayAttrib(m_vertex_array, m_render_bindings.points_in);
      glVertexArrayAttribFormat(m_vertex_array, m_render_bindings.points_in, 4, GL_FLOAT, false, 0);
      glVertexArrayAttribBinding(m_vertex_array, m_render_bindings.points_in, 0);

      glEnableVertexArrayAttrib(m_vertex_array, m_render_bindings.normal_in);
      glVertexArrayAttribFormat(m_vertex_array, m_render_bindings.normal_in, 4, GL_FLOAT, false, 0);
      glVertexArrayAttribBinding(m_vertex_array, m_render_bindings.normal_in, 1);

      glEnableVertexArrayAttrib(m_vertex_array, m_render_bindings.uv_in);
      glVertexArrayAttribFormat(m_vertex_array, m_render_bindings.uv_in, 2, GL_FLOAT, false, 0);
      glVertexArrayAttribBinding(m_vertex_array, m_render_bindings.uv_in, 2);
    }
  }
  void forward_renderer::pass_render(scene& scene)
  {
    if (!glIsProgram(m_render)) {
      create_render_shader();
    }
    
    glNamedFramebufferTextureLayer(m_framebuffer, GL_COLOR_ATTACHMENT0, m_gbuffer_texture->id(), 0, 0);
    glNamedFramebufferTextureLayer(m_framebuffer, GL_COLOR_ATTACHMENT1, m_gbuffer_texture->id(), 0, 1);
    glNamedFramebufferTextureLayer(m_framebuffer, GL_COLOR_ATTACHMENT2, m_gbuffer_texture->id(), 0, 2);
    glNamedFramebufferTexture(m_framebuffer, GL_DEPTH_ATTACHMENT, m_depth_texture->id(), 0);
    GLenum draw_buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    glNamedFramebufferDrawBuffers(m_framebuffer, std::size(draw_buffers), draw_buffers);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(m_render);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_multidraw_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_render_bindings.geometries, scene.get_scene_buffers().drawable_buffer);
    
    glBindVertexArray(m_vertex_array);
    glVertexArrayVertexBuffer(m_vertex_array, 0, scene.get_scene_buffers().vertices_buffer, 0, sizeof(rnu::vec4));
    glVertexArrayVertexBuffer(m_vertex_array, 1, scene.get_scene_buffers().normals_buffer, 0, sizeof(rnu::vec4));
    glVertexArrayVertexBuffer(m_vertex_array, 2, scene.get_scene_buffers().uvs_buffer, 0, sizeof(rnu::vec2));
    glVertexArrayElementBuffer(m_vertex_array, scene.get_scene_buffers().indices_buffer);

    glUniformMatrix4fv(m_render_bindings.view, 1, false, m_view.data());
    glUniformMatrix4fv(m_render_bindings.proj, 1, false, m_projection.data());

    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, m_num_multidraws, sizeof(multi_draw_indirect_t));
  }
  void forward_renderer::create_resolve_shader(scene& scene) {
    glsp::preprocess_file_info vs_info;
    vs_info.file_path = res_dir / "../src/pt/forward/resolve.vert";
    const auto vertex_shader = glsp::preprocess_file(vs_info);

    glsp::preprocess_file_info fs_info;
    fs_info.file_path = res_dir / "../src/pt/forward/resolve.frag";
    glsp::definition mat_inject;
    mat_inject.name = "MYRT_INJECT_MATERIAL_CODE_HERE";
    mat_inject.info.replacement = scene.get_material_assembler().get_assembled_glsl();
    fs_info.definitions = {mat_inject};
    fs_info.expand_in_macros = true;
    const auto fragment_shader = glsp::preprocess_file(fs_info);

    const auto program = make_program(vertex_shader.contents, fragment_shader.contents);
    if (program) {
      if (glIsProgram(m_resolve_program))
        glDeleteProgram(m_resolve_program);

      m_resolve_program = program.value();
      load_resolve_bindings();
    }
    if (!glIsFramebuffer(m_resolve_framebuffer))
      glCreateFramebuffers(1, &m_resolve_framebuffer);
    if (!glIsVertexArray(m_resolve_vertex_array))
      glCreateVertexArrays(1, &m_resolve_vertex_array);
  }
  void forward_renderer::load_resolve_bindings() {
    m_resolve_bindings.materials = if_empty(storage_buffer_binding(m_resolve_program, "MaterialsBuffer"));
    m_resolve_bindings.material_data = if_empty(storage_buffer_binding(m_resolve_program, "MaterialsDataBuffer"));
    m_resolve_bindings.gbuffer = if_empty(sampler_binding(m_resolve_program, "gbuffer"));
    m_resolve_bindings.view = if_empty(uniform_location(m_resolve_program, "view"));
    m_resolve_bindings.proj = if_empty(uniform_location(m_resolve_program, "proj"));
    m_resolve_bindings.random_seed = if_empty(uniform_location(m_resolve_program, "random_seed"));

    m_resolve_bindings.shadow_view = if_empty(uniform_location(m_resolve_program, "shadow_view"));
    m_resolve_bindings.shadow_proj = if_empty(uniform_location(m_resolve_program, "shadow_proj"));
    m_resolve_bindings.shadow_map  = if_empty(sampler_binding(m_resolve_program, "shadow_map"));
  }
  void forward_renderer::pass_resolve(scene& scene) {
    if (!glIsProgram(m_resolve_program)) {
      create_resolve_shader(scene);
    }
    glDisable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, m_resolve_framebuffer);
    glNamedFramebufferTexture(m_resolve_framebuffer, GL_COLOR_ATTACHMENT0, m_result_texture->id(), 0);
    glUseProgram(m_resolve_program);
    glBindTextureUnit(m_resolve_bindings.gbuffer, m_gbuffer_texture->id());
    glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER, m_resolve_bindings.materials, scene.get_scene_buffers().materials_buffer);
    glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER, m_resolve_bindings.material_data, scene.get_scene_buffers().materials_data_buffer);
    glUniformMatrix4fv(m_resolve_bindings.view, 1, false, m_view.data());
    glUniformMatrix4fv(m_resolve_bindings.proj, 1, false, m_projection.data());

    glBindTextureUnit(m_resolve_bindings.shadow_map, m_shadow_map->id());
    glUniformMatrix4fv(m_resolve_bindings.shadow_view, 1, false, m_shadow_view.data());
    glUniformMatrix4fv(m_resolve_bindings.shadow_proj, 1, false, m_shadow_proj.data());
    
    glUniform1ui(m_resolve_bindings.random_seed, m_dist(m_rng));

    glBindVertexArray(m_resolve_vertex_array);
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }
  void forward_renderer::create_shadow_shader() {
    glsp::preprocess_file_info vs_info;
    vs_info.definitions.push_back(glsp::definition{"FORWARD_ONLY_POSITION"});
    vs_info.file_path = res_dir / "../src/pt/forward/vert.vert";
    const auto vertex_shader = glsp::preprocess_file(vs_info);

    glsp::preprocess_file_info fs_info;
    fs_info.definitions.push_back(glsp::definition{"FORWARD_ONLY_POSITION"});
    fs_info.file_path = res_dir / "../src/pt/forward/frag.frag";
    const auto fragment_shader = glsp::preprocess_file(fs_info);

    const auto program = make_program(vertex_shader.contents, fragment_shader.contents);
    if (program) {
      if (glIsProgram(m_shadow_program))
        glDeleteProgram(m_shadow_program);

      m_shadow_program = program.value();
      load_shadow_bindings();
    }
    if (!glIsFramebuffer(m_shadow_framebuffer))
      glCreateFramebuffers(1, &m_shadow_framebuffer);
  }
  void forward_renderer::load_shadow_bindings() {
    glUseProgram(m_shadow_program);
    m_shadow_bindings.points_in = if_empty(input_location(m_shadow_program, "point"));
    m_shadow_bindings.view = if_empty(uniform_location(m_shadow_program, "view"));
    m_shadow_bindings.proj = if_empty(uniform_location(m_shadow_program, "proj"));
    m_shadow_bindings.geometries = if_empty(storage_buffer_binding(m_shadow_program, "Geometries"));

    if (!glIsVertexArray(m_shadow_vertex_array)) {
      glCreateVertexArrays(1, &m_shadow_vertex_array);

      glEnableVertexArrayAttrib(m_shadow_vertex_array, m_shadow_bindings.points_in);
      glVertexArrayAttribFormat(m_shadow_vertex_array, m_shadow_bindings.points_in, 4, GL_FLOAT, false, 0);
      glVertexArrayAttribBinding(m_shadow_vertex_array, m_shadow_bindings.points_in, 0);
    }
  }
  void forward_renderer::pass_shadow(scene& scene) {
    if (!glIsProgram(m_shadow_program)) {
      create_shadow_shader();
    }

    glNamedFramebufferTexture(m_shadow_framebuffer, GL_COLOR_ATTACHMENT0, m_shadow_map->id(), 0);
    glNamedFramebufferTexture(m_shadow_framebuffer, GL_DEPTH_ATTACHMENT, m_shadow_pass_depth->id(), 0);
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadow_framebuffer);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(m_shadow_program);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_multidraw_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_shadow_bindings.geometries, scene.get_scene_buffers().drawable_buffer);

    glBindVertexArray(m_shadow_vertex_array);
    glVertexArrayVertexBuffer(m_shadow_vertex_array, 0, scene.get_scene_buffers().vertices_buffer, 0, sizeof(rnu::vec4));
    glVertexArrayElementBuffer(m_shadow_vertex_array, scene.get_scene_buffers().indices_buffer);
    rnu::mat4 view = inverse(rnu::rotation(rnu::quat(rnu::vec3(1, 0, 0), rnu::radians(-45))) * rnu::translation(rnu::vec3(0, 0, 15)));

    m_shadow_view = view;
    m_shadow_proj = rnu::cameraf::orthographic(-20, 20, 20, -20, 0.01, 1000.0);
    glUniformMatrix4fv(m_shadow_bindings.view, 1, false, m_shadow_view.data());
    glUniformMatrix4fv(m_shadow_bindings.proj, 1, false, m_shadow_proj.data());

    glMultiDrawElementsIndirect(
        GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, m_num_multidraws, sizeof(multi_draw_indirect_t));
  }
  }