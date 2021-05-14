#include "sequential_pathtracer.hpp"
#include <glsp/glsp.hpp>
#include "utils.hpp"

namespace myrt {
  std::optional<GLint> uniform_location(GLuint program, const char* name)
  {
    auto const location = glGetProgramResourceLocation(program, GL_UNIFORM, name);
    if (location == GL_INVALID_INDEX)
      return std::nullopt;
    return location;
  }
  std::optional<GLint> storage_buffer_binding(GLuint program, const char* name)
  {
    GLint index = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, name);
    if (index == GL_INVALID_INDEX)
      return std::nullopt;

    constexpr GLenum prop = GL_BUFFER_BINDING;
    GLint len;
    GLint binding = GL_INVALID_INDEX;
    glGetProgramResourceiv(program, GL_SHADER_STORAGE_BLOCK, index, 1, &prop, 1, &len, &binding);
    
    if (binding == GL_INVALID_INDEX)
      return std::nullopt;
    return binding;
  }
  std::optional<GLint> sampler_binding(GLuint program, const char* name)
  {
    GLint binding = GL_INVALID_INDEX;
    auto const location = uniform_location(program, name);
    if (!location)
      return std::nullopt;
    glGetUniformiv(program, location.value(), &binding);
    if (binding == GL_INVALID_INDEX)
      return std::nullopt;
    return binding;
  }
  GLuint if_empty(std::optional<GLint> const& val)
  {
    return val ? *val : -1;
  }

  const static std::filesystem::path res_dir = "../../../res";

  void sequential_pathtracer::run(scene& scene, int width, int height)
  {
    rnu::vec2ui const new_image_size(width, height);
    if (std::ranges::any_of(m_image_size != new_image_size, [](bool b) { return b; }))
    {
      m_image_size = new_image_size;
      invalidate_texture();
    }

    if (!m_color_texture)
      m_color_texture = m_texture_provider.get(GL_TEXTURE_2D, GL_RGBA32F, width, height, 1);
    if (!m_last_color_texture)
      m_last_color_texture = m_texture_provider.get(GL_TEXTURE_2D, GL_RGBA32F, width, height, 1);
    if (!m_debug_texture)
      m_debug_texture = m_texture_provider.get(GL_TEXTURE_2D, GL_RGBA32F, width, height, 1);

    if (!glIsBuffer(m_filter_control_buffer))
    {
      glCreateBuffers(1, &m_filter_control_buffer);
      glNamedBufferStorage(m_filter_control_buffer, sizeof(filter_access_t), nullptr, GL_DYNAMIC_STORAGE_BIT);
    }
    if (!glIsBuffer(m_filter_control_buffer_target))
    {
      glCreateBuffers(1, &m_filter_control_buffer_target);
      glNamedBufferStorage(m_filter_control_buffer_target, sizeof(filter_access_t), nullptr, GL_DYNAMIC_STORAGE_BIT);
    }

    pass_generate();
    glFinish();

    for (int i = 0; i < 8; ++i)
    {
      pass_trace(scene);
      glFinish();
      pass_color(false);
      glFinish();
      pass_filter();
      glFinish();
    }
    pass_trace(scene);
    glFinish();
    pass_color(true);
    glFinish();

    m_sample_counter++;
  }
  void sequential_pathtracer::set_view_matrix(rnu::mat4 const& matrix)
  {
    if ((matrix != m_camera_view).any())
    {
      m_camera_view = matrix;
      m_inverse_camera_view = inverse(matrix);
      invalidate_counter();
    }
  }
  void sequential_pathtracer::set_projection_matrix(rnu::mat4 const& matrix)
  {
    if ((matrix != m_camera_projection).any())
    {
      m_camera_projection = matrix;
      m_inverse_camera_projection = inverse(matrix);
      invalidate_counter();
    }
  }
  void sequential_pathtracer::set_dof_enabled(bool enable)
  {
    if (enable != m_dof_active)
    {
      m_dof_active = enable;
      invalidate_counter();
    }
  }
  void sequential_pathtracer::set_lens_size(rnu::vec2 lens_size)
  {
    if ((lens_size != m_lens_size).any())
    {
      m_lens_size = lens_size;
      invalidate_counter();
    }
  }
  void sequential_pathtracer::set_focus(float focus)
  {
    if (focus != m_focus)
    {
      m_focus = focus;
      invalidate_counter();
    }
  }
  void sequential_pathtracer::set_bokeh_mask(GLuint bokeh_texture)
  {
    if (bokeh_texture != m_bokeh_texture)
    {
      m_bokeh_texture = bokeh_texture;
      invalidate_counter();
    }
  }
  void sequential_pathtracer::set_cubemap(GLuint map, GLuint sampler)
  {
    if (m_cubemap != map || m_cubemap_sampler != sampler)
    {
      m_cubemap = map;
      m_cubemap_sampler = sampler;
      invalidate_counter();
    }
  }
  std::uint32_t sequential_pathtracer::sample_count() const
  {
    return m_sample_counter;
  }
  void sequential_pathtracer::invalidate_texture()
  {
    invalidate_counter();
    m_color_texture.reset();
    m_last_color_texture.reset();
    m_debug_texture.reset();

    if (glIsBuffer(m_generate_buffer))
      glDeleteBuffers(1, &m_generate_buffer);
    glCreateBuffers(1, &m_generate_buffer);
    glNamedBufferStorage(m_generate_buffer, m_image_size.x * m_image_size.y * sizeof(generate_output_t),
      nullptr, GL_DYNAMIC_STORAGE_BIT);

    if (glIsBuffer(m_trace_buffer))
      glDeleteBuffers(1, &m_trace_buffer);
    glCreateBuffers(1, &m_trace_buffer);
    glNamedBufferStorage(m_trace_buffer, m_image_size.x * m_image_size.y * sizeof(trace_output_t),
      nullptr, GL_DYNAMIC_STORAGE_BIT);

    if (glIsBuffer(m_filter_buffer))
      glDeleteBuffers(1, &m_filter_buffer);
    glCreateBuffers(1, &m_filter_buffer);
    glNamedBufferStorage(m_filter_buffer, m_image_size.x * m_image_size.y * sizeof(generate_output_t),
      nullptr, GL_DYNAMIC_STORAGE_BIT);
  }
  void sequential_pathtracer::invalidate_counter()
  {
    m_sample_counter = 0;
  }

  GLuint sequential_pathtracer::color_texture_id() const {
    return m_color_texture ? m_color_texture->id() : 0u;
  }

  GLuint sequential_pathtracer::debug_texture_id() const
  {
    return m_debug_texture->id();
  }

  void sequential_pathtracer::create_generate_shader()
  {
    glsp::preprocess_file_info generate_file;
    generate_file.file_path = res_dir / "../src/pt/pt_generate.comp";
    auto const preprocessed_file = glsp::preprocess_file(generate_file);

    auto const program = make_program(preprocessed_file.contents);

    if (program) {
      if (glIsProgram(m_generate_shader))
        glDeleteProgram(m_generate_shader);

      m_generate_shader = program.value();
      load_generate_bindings();
    }
  }
  void sequential_pathtracer::load_generate_bindings()
  {
    glUseProgram(m_generate_shader);

    m_generate_bindings.debug_texture = if_empty(sampler_binding(m_generate_shader, "debug_image"));
    m_generate_bindings.inverse_camera_view = if_empty(uniform_location(m_generate_shader, "inverse_camera_view"));
    m_generate_bindings.inverse_camera_projection = if_empty(uniform_location(m_generate_shader, "inverse_camera_projection"));
    m_generate_bindings.dof_active = if_empty(uniform_location(m_generate_shader, "dof_active"));
    m_generate_bindings.image_size = if_empty(uniform_location(m_generate_shader, "image_size"));
    m_generate_bindings.lens_size = if_empty(uniform_location(m_generate_shader, "lens_size"));
    m_generate_bindings.focus = if_empty(uniform_location(m_generate_shader, "focus"));
    m_generate_bindings.random_seed = if_empty(uniform_location(m_generate_shader, "random_seed"));

    m_generate_bindings.random_texture = if_empty(sampler_binding(m_generate_shader, "random_texture"));
    m_generate_bindings.bokeh_shape = if_empty(sampler_binding(m_generate_shader, "bokeh_shape"));

    m_generate_bindings.output_buffer = if_empty(storage_buffer_binding(m_generate_shader, "GenerateOutput"));

    GLint group_sizes[3]{};
    glGetProgramiv(m_generate_shader, GL_COMPUTE_WORK_GROUP_SIZE, group_sizes);
    m_generate_group_sizes = { group_sizes[0], group_sizes[1] };

    if (!m_random_texture) {
      m_random_texture_data.resize(1024);
      m_random_texture = m_texture_provider.get(GL_TEXTURE_1D, GL_R32F, int(m_random_texture_data.size()), 1);
      m_random_texture->lock();
    }
  }
  void sequential_pathtracer::pass_generate()
  {
    std::uniform_int_distribution<> dist;
    if (!glIsProgram(m_generate_shader))
      create_generate_shader();

    if (!glIsProgram(m_generate_shader))
      return;

    auto const count_linear_x = ((m_image_size.x * m_image_size.y) + linear_group_size - 1) / linear_group_size;
    const filter_access_t empty{ static_cast<std::uint32_t>(m_image_size.x * m_image_size.y), count_linear_x, 1, 1 };
    glNamedBufferSubData(m_filter_control_buffer, 0, sizeof(filter_access_t), &empty);

    glUseProgram(m_generate_shader);
    glUniformMatrix4fv(m_generate_bindings.inverse_camera_view, 1, false, m_inverse_camera_view.data());
    glUniformMatrix4fv(m_generate_bindings.inverse_camera_projection, 1, false, m_inverse_camera_projection.data());

    glUniform1i(m_generate_bindings.dof_active, m_dof_active);
    glUniform2uiv(m_generate_bindings.image_size, 1, m_image_size.data());
    glUniform2fv(m_generate_bindings.lens_size, 1, m_lens_size.data());
    glUniform1f(m_generate_bindings.focus, m_focus);
    glUniform1i(m_generate_bindings.random_seed, dist(m_rng));

    std::uniform_real_distribution<float> distribution(0.f, 1.f);
    std::generate(m_random_texture_data.begin(), m_random_texture_data.end(), [&] {return distribution(m_rng); });
    glTextureSubImage1D(m_random_texture->id(), 0, 0, GLsizei(m_random_texture_data.size()), GL_RED, GL_FLOAT, m_random_texture_data.data());
    glBindTextureUnit(m_generate_bindings.random_texture, m_random_texture->id());

    glBindTextureUnit(m_generate_bindings.bokeh_shape, m_bokeh_texture);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_generate_bindings.output_buffer, m_generate_buffer);

    auto const count_x = (m_image_size.x + m_generate_group_sizes.x - 1) / m_generate_group_sizes.x;
    auto const count_y = (m_image_size.y + m_generate_group_sizes.y - 1) / m_generate_group_sizes.y;
    glDispatchCompute(count_x, count_y, 1);
    glMemoryBarrierByRegion(GL_SHADER_STORAGE_BARRIER_BIT);
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT);
  }
  void sequential_pathtracer::pass_trace(scene& scene)
  {
    std::uniform_int_distribution<> dist;

    if (&scene != m_last_scene)
    {
      glDeleteProgram(m_trace_shader);
      invalidate_counter();
      m_last_scene = &scene;
    }

    auto const prepare = scene.prepare();

    if (prepare.drawables_changed || prepare.geometries_changed || prepare.materials_changed || prepare.sdf_buffer_changed)
      invalidate_counter();

    if (prepare.sdfs_changed)
    {
      glDeleteProgram(m_trace_shader);
      invalidate_counter();
    }

    if (!glIsProgram(m_trace_shader))
      create_trace_shader(scene);

    glUseProgram(m_trace_shader);

    glUniform1i(m_trace_bindings.random_sample, dist(m_rng));

    std::uniform_real_distribution<float> distribution(0.f, 1.f);
    std::generate(m_random_texture_data.begin(), m_random_texture_data.end(), [&] {return distribution(m_rng); });
    glTextureSubImage1D(m_random_texture->id(), 0, 0, GLsizei(m_random_texture_data.size()), GL_RED, GL_FLOAT, m_random_texture_data.data());
    glBindTextureUnit(m_trace_bindings.random_texture, m_random_texture->id());

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_trace_bindings.access_control, m_filter_control_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_trace_bindings.input_buffer, m_generate_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_trace_bindings.output_buffer, m_trace_buffer);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_trace_bindings.bvh, scene.get_scene_buffers().bvh_nodes_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_trace_bindings.bvh_indices, scene.get_scene_buffers().bvh_indices_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_trace_bindings.mesh_indices, scene.get_scene_buffers().indices_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_trace_bindings.mesh_points, scene.get_scene_buffers().vertices_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_trace_bindings.mesh_normals, scene.get_scene_buffers().normals_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_trace_bindings.mesh_geometries, scene.get_scene_buffers().drawable_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_trace_bindings.global_bvh, scene.get_scene_buffers().global_bvh_nodes_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_trace_bindings.global_bvh_indices, scene.get_scene_buffers().global_bvh_indices_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_trace_bindings.sdf_data, scene.get_scene_buffers().sdf_data_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_trace_bindings.sdf_drawables, scene.get_scene_buffers().sdf_drawable_buffer);

    glUniform1i(m_trace_bindings.sdf_marching_steps, 400);
    glUniform1f(m_trace_bindings.sdf_marching_epsilon, 1e-5f);

    glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_filter_control_buffer);
    glDispatchComputeIndirect(offsetof(filter_access_t, num_groups_x));
    glMemoryBarrierByRegion(GL_SHADER_STORAGE_BARRIER_BIT);
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT);
  }
  void sequential_pathtracer::create_trace_shader(scene& scene)
  {
    glsp::definition sdf_inject;
    auto const& sdfs = scene.sdfs();
    sdf_inject.name = "MYRT_INJECT_SDF_MAP_CODE_HERE";
    sdf_inject.info.replacement = scene.get_sdf_assembler().get_assembled_glsl();
    glsp::preprocess_file_info trace_file;
    trace_file.file_path = res_dir / "../src/pt/pt_trace.comp";
    trace_file.definitions = {
        glsp::definition("MYRT_POINT_TYPE", detail::glsl::bvh_point_type),
        glsp::definition("MYRT_INDEX_TYPE", detail::glsl::bvh_index_type),
        glsp::definition("MYRT_BVH_NODE_PADDING_BYTES", std::to_string((sizeof(aligned_node_t) - sizeof(bvh_node_t)) / sizeof(detail::default_index_type))),
        glsp::definition("MYRT_BVH_NODE_TYPE_SHIFT", std::to_string(bvh_node_t::type_shift)),
        glsp::definition("MYRT_BVH_NODE_STRUCT", detail::glsl::bvh_struct_name),
        sdf_inject };

    auto const preprocessed_file = glsp::preprocess_file(trace_file);

    auto const program = make_program(preprocessed_file.contents);
    if (program) {
      if (glIsProgram(m_trace_shader))
        glDeleteProgram(m_trace_shader);

      m_trace_shader = program.value();
      load_trace_bindings();
    }
  }
  void sequential_pathtracer::load_trace_bindings()
  {
    glUseProgram(m_trace_shader);

    m_trace_bindings.access_control = if_empty(storage_buffer_binding(m_trace_shader, "AccessControl"));
    m_trace_bindings.input_buffer = if_empty(storage_buffer_binding(m_trace_shader, "GenerateInput"));
    m_trace_bindings.output_buffer = if_empty(storage_buffer_binding(m_trace_shader, "TraceOutput"));


    m_trace_bindings.bvh = if_empty(storage_buffer_binding(m_trace_shader, "BVH"));
    m_trace_bindings.bvh_indices = if_empty(storage_buffer_binding(m_trace_shader, "BVHIndices"));
    m_trace_bindings.mesh_indices = if_empty(storage_buffer_binding(m_trace_shader, "MeshIndices"));
    m_trace_bindings.mesh_points = if_empty(storage_buffer_binding(m_trace_shader, "MeshPoints"));
    m_trace_bindings.mesh_normals = if_empty(storage_buffer_binding(m_trace_shader, "MeshNormals"));
    m_trace_bindings.mesh_geometries = if_empty(storage_buffer_binding(m_trace_shader, "Geometries"));
    m_trace_bindings.global_bvh = if_empty(storage_buffer_binding(m_trace_shader, "BVHGlobal"));
    m_trace_bindings.global_bvh_indices = if_empty(storage_buffer_binding(m_trace_shader, "BVHIndicesGlobal"));
    m_trace_bindings.sdf_data = if_empty(storage_buffer_binding(m_trace_shader, "SDFData"));
    m_trace_bindings.sdf_drawables = if_empty(storage_buffer_binding(m_trace_shader, "SDFDrawables"));

    m_trace_bindings.sdf_marching_steps = if_empty(uniform_location(m_trace_shader, "sdf_marching_steps"));
    m_trace_bindings.sdf_marching_epsilon = if_empty(uniform_location(m_trace_shader, "sdf_marching_epsilon"));

    m_trace_bindings.random_texture = if_empty(sampler_binding(m_trace_shader, "random_texture"));
    m_trace_bindings.random_sample = if_empty(uniform_location(m_trace_shader, "random_sample"));
  }
  void sequential_pathtracer::pass_filter()
  {
    if (!glIsProgram(m_ray_filter_shader))
      create_filter_shader();

    const filter_access_t empty{ 0u, 0u, 1, 1 };
    glNamedBufferSubData(m_filter_control_buffer_target, 0, sizeof(filter_access_t), &empty);

    glUseProgram(m_ray_filter_shader);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_ray_filter_bindings.trace_buffer, m_trace_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_ray_filter_bindings.filter_buffer, m_filter_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_ray_filter_bindings.generate_buffer, m_generate_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_ray_filter_bindings.access_control, m_filter_control_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_ray_filter_bindings.access_control_target, m_filter_control_buffer_target);

    glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_filter_control_buffer);
    glDispatchComputeIndirect(offsetof(filter_access_t, num_groups_x));

    std::swap(m_filter_control_buffer_target, m_filter_control_buffer);
    std::swap(m_generate_buffer, m_filter_buffer);
    glMemoryBarrierByRegion(GL_SHADER_STORAGE_BARRIER_BIT);
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT);
  }
  void sequential_pathtracer::create_filter_shader()
  {
    glsp::preprocess_file_info filter_file;
    filter_file.file_path = res_dir / "../src/pt/pt_ray_filter.comp";
    auto const preprocessed_file = glsp::preprocess_file(filter_file);

    auto const program = make_program(preprocessed_file.contents);
    if (program) {
      if (glIsProgram(m_ray_filter_shader))
        glDeleteProgram(m_ray_filter_shader);

      m_ray_filter_shader = program.value();
      load_filter_bindings();
    }
  }
  void sequential_pathtracer::load_filter_bindings()
  {
    m_ray_filter_bindings.access_control = if_empty(storage_buffer_binding(m_ray_filter_shader, "AccessControl"));
    m_ray_filter_bindings.access_control_target = if_empty(storage_buffer_binding(m_ray_filter_shader, "AccessControlTarget"));
    m_ray_filter_bindings.generate_buffer = if_empty(storage_buffer_binding(m_ray_filter_shader, "GenerateOutput"));
    m_ray_filter_bindings.trace_buffer = if_empty(storage_buffer_binding(m_ray_filter_shader, "TraceOutput"));
    m_ray_filter_bindings.filter_buffer = if_empty(storage_buffer_binding(m_ray_filter_shader, "FilterOutput"));
  }
  void sequential_pathtracer::pass_color(bool force_write)
  {
    std::uniform_int_distribution<> dist;

    if (!glIsProgram(m_color_shader))
      create_color_shader();

    glUseProgram(m_color_shader);

    std::uniform_real_distribution<float> distribution(0.f, 1.f);
    std::generate(m_random_texture_data.begin(), m_random_texture_data.end(), [&] {return distribution(m_rng); });
    glTextureSubImage1D(m_random_texture->id(), 0, 0, GLsizei(m_random_texture_data.size()), GL_RED, GL_FLOAT, m_random_texture_data.data());
    glBindTextureUnit(m_color_bindings.random_texture, m_random_texture->id());

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_color_bindings.generate_output, m_generate_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_color_bindings.trace_output, m_trace_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_color_bindings.access_control, m_filter_control_buffer);
    glBindImageTexture(m_color_bindings.debug_texture, m_debug_texture->id(), 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
    glBindImageTexture(m_color_bindings.output_image, m_color_texture->id(), 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
    glUniform1i(m_color_bindings.random_sample, dist(m_rng));

    glUniform1ui(m_color_bindings.sample_index, m_sample_counter);
    glUniform1i(m_color_bindings.force_write_color, force_write);

    glBindSampler(m_color_bindings.cubemap, m_cubemap_sampler);
    glBindTextureUnit(m_color_bindings.cubemap, m_cubemap);

    glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, m_filter_control_buffer);
    glDispatchComputeIndirect(offsetof(filter_access_t, num_groups_x));
    glMemoryBarrierByRegion(GL_SHADER_STORAGE_BARRIER_BIT);
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT);
  }
  void sequential_pathtracer::create_color_shader()
  {
    glsp::preprocess_file_info color_file;
    color_file.file_path = res_dir / "../src/pt/pt_color.comp";
    auto const preprocessed_file = glsp::preprocess_file(color_file);

    auto const program = make_program(preprocessed_file.contents);
    if (program) {
      if (glIsProgram(m_color_shader))
        glDeleteProgram(m_color_shader);

      m_color_shader = program.value();
      load_color_bindings();
    }
  }
  void sequential_pathtracer::load_color_bindings()
  {
    glUseProgram(m_color_shader);
    m_color_bindings.access_control = if_empty(storage_buffer_binding(m_color_shader, "AccessControl"));
    m_color_bindings.generate_output = if_empty(storage_buffer_binding(m_color_shader, "GenerateInput"));
    m_color_bindings.trace_output = if_empty(storage_buffer_binding(m_color_shader, "TraceOutput"));
    m_color_bindings.debug_texture = if_empty(sampler_binding(m_color_shader, "debug_image"));
    m_color_bindings.output_image = if_empty(sampler_binding(m_color_shader, "output_image"));
    m_color_bindings.sample_index = if_empty(uniform_location(m_color_shader, "sample_index"));
    m_color_bindings.force_write_color = if_empty(uniform_location(m_color_shader, "force_write_color"));
    m_color_bindings.random_texture = if_empty(sampler_binding(m_color_shader, "random_texture"));
    m_color_bindings.random_sample = if_empty(uniform_location(m_color_shader, "random_sample"));
    m_color_bindings.cubemap = if_empty(sampler_binding(m_color_shader, "cubemap"));
  }
}