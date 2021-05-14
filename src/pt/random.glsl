#pragma once

const float random_inverse_max_uint = 1.f / float(0xFFFFFFFFu);
const int random_primes[6] = { 69019, 96013, 32159, 22783, 87011, 45263 };

struct random_state_t {
  uint rng_state;
  int  current_prime;
  int random_count;
};

random_state_t random_state;

uint random_wang_hash(uint a)
{
  a = (a ^ 61) ^ (a >> 16);
  a = a + (a << 3);
  a = a ^ (a >> 4);
  a = a * 0x27d4eb2d;
  a = a ^ (a >> 15);
  return a;
}
uint random_xorshift()
{
  // Xorshift algorithm from George Marsaglia's paper
  random_state.rng_state ^= (random_state.rng_state << 13);
  random_state.rng_state ^= (random_state.rng_state >> 17);
  random_state.rng_state ^= (random_state.rng_state << 5);
  return random_state.rng_state;
}

float random_value(int seed)
{
  random_state.rng_state += random_wang_hash(uint(seed));
  return clamp(float(random_xorshift()) * random_inverse_max_uint, 0.f, 1.f);
}

int random_int(int seed)
{
  float v = random_value(seed);
  const int INT_MAX = int(0x7fffffff);
  return int(v * INT_MAX);
}

void random_init(sampler1D random_texture, ivec2 pixel, int start_seed)
{
  random_state.rng_state = uint(start_seed);
  random_state.current_prime = 0;
  random_state.random_count = int(textureSize(random_texture, 0));
  int seed_base = int(random_int(pixel.x ^ 0xba77fa) * random_primes[0] + random_int(pixel.y ^ 0xcca6df) * random_primes[1] + random_int(start_seed));
  random_value(seed_base + int(0xffaf86 * texelFetch(random_texture, int(seed_base % random_state.random_count), 0).x));
}

float random_next()
{
  return random_value(random_state.current_prime = (random_state.current_prime + 1) % 6);
}

vec2 random_next_2d() {
  return vec2(random_next(), random_next());
}

vec3 random_next_3d() {
  return vec3(random_next(), random_next(), random_next());
}

vec2 random_uniform_circle(float radius)
{
  vec2 rands = vec2(
    random_next() * 2 * 3.1415926535897f,
    random_next()
  );

  return sqrt(rands.y) * radius * vec2(sin(rands.x), cos(rands.x));
}