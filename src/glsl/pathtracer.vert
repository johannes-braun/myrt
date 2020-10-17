#version 450 core

const vec2 positions[3] = vec2[3](
    vec2(-1, -1),
    vec2(-1, 3),
    vec2(3, -1)
);

layout(location=0) uniform mat4 u_inv_proj;
layout(location=1) uniform mat4 u_inv_view;
layout(location=3) uniform uint u_random_seed;

layout(location=0) out vec3 out_ray_origin;
layout(location=1) out vec3 out_ray_direction;
layout(location=2) out vec3 out_pixel_right;
layout(location=3) out vec3 out_pixel_up;

void main()
{
    gl_Position = vec4(positions[gl_VertexID], 0, 1);
    out_ray_origin = (u_inv_view * vec4(0, 0, 0, 1)).xyz;
    out_pixel_right = (u_inv_view * vec4(1, 0, 0, 0)).xyz;
    out_pixel_up = (u_inv_view * vec4(0, 1, 0, 0)).xyz;
    out_ray_direction = (u_inv_view * u_inv_proj*vec4(gl_Position.xy, 1, 1)).xyz;
}