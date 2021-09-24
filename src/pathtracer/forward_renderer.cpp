#include "forward_renderer.hpp"

#include <mygl/mygl.hpp>
#include <myrt/sfml/utils.hpp>
#include "bindings.hpp"
#include <filesystem>
#include <glsp/glsp.hpp>

namespace myrt
{
  const static std::filesystem::path res_dir = "../../../res";

  std::shared_ptr<texture_t> forward_renderer::run(texture_provider_t& provider, scene& scene, int width, int height)
  {
    auto const prepare = scene.prepare();

    if (!glIsFramebuffer(m_framebuffer))
      glCreateFramebuffers(1, &m_framebuffer);

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
      m_color_texture = nullptr;
      m_depth_texture = nullptr;
    }

    if (!m_color_texture)
      m_color_texture = provider.get(GL_TEXTURE_2D, GL_RGBA8, width, height, 1);
    if (!m_depth_texture)
      m_depth_texture = provider.get(GL_TEXTURE_2D, GL_DEPTH24_STENCIL8, width, height, 1);

    glNamedFramebufferTexture(m_framebuffer, GL_COLOR_ATTACHMENT0, m_color_texture->id(), 0);
    glNamedFramebufferTexture(m_framebuffer, GL_DEPTH_ATTACHMENT, m_depth_texture->id(), 0);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

    pass_gen_multidraw(scene);
    pass_render(scene);

    return m_color_texture;
  }
  void forward_renderer::set_view_matrix(rnu::mat4 const& matrix)
  {
    m_view = matrix;
  }
  void forward_renderer::set_projection_matrix(rnu::mat4 const& matrix)
  {
    m_projection = matrix;
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
  void forward_renderer::create_render_shader(scene& scene)
  {
    glsp::preprocess_file_info vs_info;
    vs_info.file_path = res_dir / "../src/pt/forward/vert.vert";
    const auto vertex_shader = glsp::preprocess_file(vs_info);

    glsp::preprocess_file_info fs_info;
    fs_info.file_path = res_dir / "../src/pt/forward/frag.frag"; 
    glsp::definition mat_inject;
    mat_inject.name = "MYRT_INJECT_MATERIAL_CODE_HERE";
    mat_inject.info.replacement = scene.get_material_assembler().get_assembled_glsl();
    fs_info.definitions = { mat_inject };
    fs_info.expand_in_macros = true;
    const auto fragment_shader = glsp::preprocess_file(fs_info);

    const auto program = make_program(vertex_shader.contents, fragment_shader.contents);
    if (program) {
      if (glIsProgram(m_render))
        glDeleteProgram(m_render);

      m_render = program.value();
      load_render_bindings();
    }
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
    m_render_bindings.materials = if_empty(storage_buffer_binding(m_render, "MaterialsBuffer"));
    m_render_bindings.material_data = if_empty(storage_buffer_binding(m_render, "MaterialsDataBuffer"));

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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    if (!glIsProgram(m_render))
      create_render_shader(scene);

    glUseProgram(m_render);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_multidraw_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_render_bindings.geometries, scene.get_scene_buffers().drawable_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_render_bindings.materials, scene.get_scene_buffers().materials_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_render_bindings.material_data, scene.get_scene_buffers().materials_data_buffer);
    
    glBindVertexArray(m_vertex_array);
    glVertexArrayVertexBuffer(m_vertex_array, 0, scene.get_scene_buffers().vertices_buffer, 0, sizeof(rnu::vec4));
    glVertexArrayVertexBuffer(m_vertex_array, 1, scene.get_scene_buffers().normals_buffer, 0, sizeof(rnu::vec4));
    glVertexArrayVertexBuffer(m_vertex_array, 2, scene.get_scene_buffers().uvs_buffer, 0, sizeof(rnu::vec2));
    glVertexArrayElementBuffer(m_vertex_array, scene.get_scene_buffers().indices_buffer);

    glUniformMatrix4fv(m_render_bindings.view, 1, false, m_view.data());
    glUniformMatrix4fv(m_render_bindings.proj, 1, false, m_projection.data());

    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, m_num_multidraws, sizeof(multi_draw_indirect_t));

  }
}