#version 460 core

void main() {
  // (1, 1), (1, 0), (0, 1)
  const uint bitmask_x = 0x3;
  const uint bitmask_y = 0x5;
  
  uint index = uint(gl_VertexIndex);
  float x = float((bitmask_x >> index) & 1) * 4.0 - 3.0;
  float y = float((bitmask_y >> index) & 1) * 4.0 - 3.0;

  gl_Position = vec4(x, y, 0, 1);
}