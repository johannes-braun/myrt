#version 430

layout(location = 0) flat out vec3 cam_pos;

uniform mat4 view;
uniform mat4 proj;

void main() {
  // (1, 1), (1, 0), (0, 1)
  const uint bitmask_x = 0x3;
  const uint bitmask_y = 0x5;
  
  uint index = uint(gl_VertexID);
  float x = float((bitmask_x >> index) & 1) * 4.0 - 3.0;
  float y = float((bitmask_y >> index) & 1) * 4.0 - 3.0;

  cam_pos = inverse(view)[3].xyz;
  gl_Position = vec4(x, y, 0, 1);
}