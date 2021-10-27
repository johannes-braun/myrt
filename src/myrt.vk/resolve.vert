#version 460 core

layout(location = 0) flat out vec3 cam_pos;

layout(push_constant) uniform constants {
  mat4 view_matrix;
  mat4 projection_matrix;
} in_constants;

void main() {
  // (1, 1), (1, 0), (0, 1)
  const uint bitmask_x = 0x3;
  const uint bitmask_y = 0x5;
  
  uint index = uint(gl_VertexIndex);
  float x = float((bitmask_x >> index) & 1) * 4.0 - 3.0;
  float y = float((bitmask_y >> index) & 1) * 4.0 - 3.0;

  cam_pos = inverse(in_constants.view_matrix)[3].xyz;

  gl_Position = vec4(x, y, 0, 1);
}