#pragma once

#include "vectoring.hpp"

namespace myrt
{
  inline float distance(rnu::vec2d point, rnu::vec2d start, line_data_t const& line, bool relative)
  {
    const auto& p0 = start;
    auto p1 = line.value;
    if (relative)
      p1 += start;

    const auto AB = point - p0;
    const auto CD = p1 - p0;

    const auto dot = rnu::dot(AB, CD);
    const auto len_sq = rnu::dot(CD, CD);
    auto param = -1.0;
    if (len_sq != 0) //in case of 0 length line
      param = dot / len_sq;

    auto xxyy = p0 + param * CD;

    if (param <= 0) {
      xxyy = p0;
    }
    else if (param >= 1) {
      xxyy = p1;
    }

    const auto x = rnu::dot(AB, rnu::vec2d(-CD.y, CD.x));
    const auto sign = x < 0 ? -1 : (x>0?1:1000000);

    const auto d = point - xxyy;
    return sign * rnu::norm(d);
  }

  namespace detail
  {
    using vec2 = rnu::vec2;
    using vec3 = rnu::vec3;
    
    template<typename T>
    T mix(T a, T b, float t)
    {
      return (1 - t) * a + t * b;
    }
    
    vec2 mix(vec2 a, vec2 b, vec2 t)
    {
      return vec2{ mix(a.x, b.x, float(t.x==1)), mix(a.y, b.y, float(t.y == 1)) };
    }
    template<typename T>
    T sign(T x)
    {
      return x < 0 ? -1 : (x > 0 ? 1 : 0);
    }
    vec2 sign(vec2 x)
    {
      return vec2{ sign(x.x), sign(x.y) };
    }
    float step(float x, float th)
    {
      return x < th ? 0.f : 1.f;
    }

    // Test if point p crosses line (a, b), returns sign of result
    float testCross(vec2 a, vec2 b, vec2 p) {
      return sign((b.y - a.y) * (p.x - a.x) - (b.x - a.x) * (p.y - a.y));
    }

    // Determine which side we're on (using barycentric parameterization)
    float signBezier(vec2 A, vec2 B, vec2 C, vec2 p)
    {
      vec2 a = C - A, b = B - A, c = p - A;
      vec2 bary = vec2(c.x * b.y - b.x * c.y, a.x * c.y - c.x * a.y) / (a.x * b.y - b.x * a.y);
      vec2 d = vec2(bary.y * 0.5, 0.0) + 1.0f - bary.x - bary.y;

      auto s = sign(d.x * d.x - d.y);
      auto e = mix(-1.0f, 1.0f, step(testCross(A, B, p) * testCross(B, C, p), 0.0));
      auto t = step((d.x - d.y), 0.0f);

      return mix(s, e, 0)* testCross(A, C, B);
    }

    // Solve cubic equation for roots
    vec3 solveCubic(float a, float b, float c)
    {
      float p = b - a * a / 3.0, p3 = p * p * p;
      float q = a * (2.0 * a * a - 9.0 * b) / 27.0 + c;
      float d = q * q + 4.0 * p3 / 27.0;
      float offset = -a / 3.0;
      if (d >= 0.0) {
        float z = sqrt(d);
        vec2 x = (vec2(z, -z) - q) / 2.0f;
        vec2 uv = sign(x) * pow(abs(x), vec2(1.0 / 3.0));
        return vec3(offset + uv.x + uv.y);
      }
      float v = acos(-sqrt(-27.0 / p3) * q / 2.0) / 3.0;
      float m = cos(v), n = sin(v) * 1.732050808;
      return vec3(m + m, -n - m, n - m) * sqrt(-p / 3.0f) + offset;
    }

    // Find the signed distance from a point to a bezier curve
    float sdBezier(vec2 A, vec2 B, vec2 C, vec2 p)
    {
      B = mix(B + vec2(1e-4), B, abs(sign(B * 2.0f - A - C)));
      vec2 a = B - A, b = A - B * 2.0f + C, c = a * 2.0f, d = A - p;
      vec3 k = vec3(3. * dot(a, b), 2. * dot(a, a) + dot(d, b), dot(d, a)) / dot(b, b);
      vec3 t = clamp(solveCubic(k.x, k.y, k.z), 0.0, 1.0);
      vec2 pos = A + (c + b * t.x) * t.x;
      float dis = norm(pos - p);
      pos = A + (c + b * t.y) * t.y;
      dis = rnu::min(dis, norm(pos - p));
      pos = A + (c + b * t.z) * t.z;
      dis = rnu::min(dis, norm(pos - p));
      return -dis * signBezier(A, B, C, p);
    }
  }

  inline float distance(rnu::vec2d point, rnu::vec2d start, quad_bezier_data_t const& x, bool relative)
  {
    auto control = rnu::vec2(x.x1, x.y1);
    auto end = rnu::vec2(x.x, x.y);
    if (relative) {
      control += rnu::vec2(start);
      end += rnu::vec2(start);
    }
    return detail::sdBezier(rnu::vec2(start), control, end, rnu::vec2(point));
  }

  template<typename T>
  inline float distance(rnu::vec2d point, rnu::vec2d start, T const& x, bool relative)
  {
    return -1;
  }

  inline float distance(rnu::vec2d point, rnu::vec2d start, path_action_t const& action)
  {
    const auto relative = is_relative(action.type);
    return visit(action, [&](auto val) { return distance(point, start, val, relative); });
  }
}