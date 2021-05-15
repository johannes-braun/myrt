#pragma once

#include <rnu/math/math.hpp>
#include <optional>
#include <span>
#include <vector>
#include <functional>
#include <experimental/generator>

namespace myrt
{
  struct build_state_t;

  namespace detail
  {
    namespace glsl
    {
      constexpr char const bvh_struct_name[] = "bvh_node_t";
      constexpr char const bvh_index_type[] = "uint";
      constexpr char const bvh_point_type[] = "vec3";
    }

    using default_point_type = rnu::vec3;
    using default_index_type = std::uint32_t;
    constexpr size_t bvh_node_alignment = sizeof(float) * 4;
  }

  struct aabb_t
  {
    using point_type = detail::default_point_type;

    point_type min = point_type(std::numeric_limits<float>::max());
    point_type max = point_type(std::numeric_limits<float>::lowest());
    std::optional<point_type> weighted_centroid;

    [[nodiscard]] constexpr point_type operator[](ptrdiff_t index) const noexcept {
      return index == 0 ? min : max;
    }
    [[nodiscard]] constexpr point_type dimension() const noexcept {
      return max - min;
    }
    [[nodiscard]] constexpr point_type center() const noexcept {
      return (max + min) * 0.5f;
    }
    [[nodiscard]] constexpr point_type centroid() const noexcept {
      return weighted_centroid.has_value() ? weighted_centroid.value() : center();
    }
    [[nodiscard]] constexpr auto surface_area() const noexcept {
      const auto dim = dimension();
      return ((dim.x + dim.y) * dim.z + (dim.x * dim.y)) * 2.f;
    }

    constexpr std::pair<std::optional<aabb_t>, std::optional<aabb_t>> split(int axis, float position) {
      using pair = std::pair<std::optional<aabb_t>, std::optional<aabb_t>>;
      if (position <= min[axis])
        return pair(std::nullopt, *this);
      else if (position >= max[axis])
        return pair(*this, std::nullopt);
      else
      {
        point_type smax = max;
        smax[axis] = position;
        point_type smin = min;
        smin[axis] = smax[axis];

        return pair(aabb_t{ min, smax }, aabb_t{ smin, max });
      }
    }

    constexpr void enclose(point_type point) noexcept {
      min = rnu::min(min, point);
      max = rnu::max(max, point);
    }
    constexpr void enclose(aabb_t aabb) noexcept {
      min = rnu::min(min, aabb.min);
      max = rnu::max(max, aabb.max);
    }
    constexpr void pad(float padding) noexcept {
      min -= padding;
      max += padding;
    }

    constexpr bool empty(float eps = 1e-7) noexcept {
      auto const dim = dimension();
      return dim.x < eps || dim.y < eps || dim.z < eps;
    }
  };

  struct ray_t
  {
    rnu::vec3 origin;
    rnu::vec3 direction;
    float length;

    [[nodiscard]] std::optional<float> intersect(const rnu::vec3 v1, const rnu::vec3 v2, const rnu::vec3 v3, rnu::vec2& barycentric);
    [[nodiscard]] std::optional<float> intersect(aabb_t const& aabb) const noexcept;
  };

  struct bvh_node_t
  {
    using point_type = detail::default_point_type;
    using index_type = detail::default_index_type;

    constexpr static unsigned type_shift = 31;
    constexpr static unsigned type_mask = 1 << type_shift;
    constexpr static unsigned parent_mask = ~type_mask;

    [[nodiscard]] constexpr static index_type make_type_and_parent(bool leaf, index_type parent)
    {
      return (leaf << type_shift) | parent;
    }
    [[nodiscard]] constexpr bool is_leaf() const noexcept {
      return (type_and_parent & type_mask) != 0;
    }
    constexpr void make_leaf(bool leaf) noexcept {
      type_and_parent = (type_and_parent & parent_mask) | (leaf << type_shift);
    }
    [[nodiscard]] constexpr index_type parent() const noexcept {
      return type_and_parent & parent_mask;
    }
    constexpr void set_parent(index_type new_parent) noexcept {
      type_and_parent = (type_and_parent & type_mask) | (new_parent & parent_mask);
    }
    [[nodiscard]] constexpr aabb_t aabb() const noexcept {
      return aabb_t{ min_extents, max_extents };
    }
    constexpr void set_aabb(aabb_t new_aabb) noexcept {
      min_extents = new_aabb.min;
      max_extents = new_aabb.max;
    }

    point_type min_extents;
    index_type type_and_parent = type_mask;
    point_type max_extents;
    index_type first_child;
    index_type second_child;
  };

  struct aligned_node_t
  {
    alignas(detail::bvh_node_alignment) bvh_node_t node;
  };
  static_assert(sizeof(aligned_node_t) % detail::bvh_node_alignment == 0, "BVH node alignment failed.");

  [[nodiscard]] std::vector<aabb_t> generate_triangle_bounds(std::span<detail::default_index_type const> indices, std::function<detail::default_point_type(detail::default_index_type)> const& get_point);
  [[nodiscard]] std::vector<aabb_t> generate_triangle_bounds(std::span<detail::default_index_type const> indices, std::span<detail::default_point_type const> points);

  struct primitive_split_request_t
  {
    detail::default_index_type index;
    aabb_t aabb;
    int axis;
    float position;
  };

  struct primitive_split_result_t
  {
    std::optional<aabb_t> lower;
    std::optional<aabb_t> higher;
  };
  using primitive_split_func = std::function<primitive_split_result_t(primitive_split_request_t request)>;

  extern const primitive_split_func dont_split;
  extern const primitive_split_func split_aabbs;

  class bvh
  {
  public:
    using index_type = detail::default_index_type;
    using point_type = detail::default_point_type;

    constexpr static size_t binned_sah_bin_count = 8;
    constexpr static int min_leaf_primitives = 2;

    [[nodiscard]] bvh(std::span<aabb_t const> aabbs, primitive_split_func split = nullptr);
    [[nodiscard]] std::experimental::generator<index_type> traverse(ray_t const& ray) const;
    [[nodiscard]] std::vector<aligned_node_t> const& nodes() const noexcept;
    [[nodiscard]] std::vector<index_type> const& reordered_indices() const noexcept;
    [[nodiscard]] aabb_t aabb() const noexcept;

  private:
    void create(build_state_t& initial_state);
    [[nodiscard]] std::optional<std::pair<bvh_node_t, bvh_node_t>> split(index_type current_node_index, const bvh_node_t& current_node, build_state_t& build_state);
    [[nodiscard]] std::tuple<int, float, bool, int, int> compute_split_axis(bvh_node_t const& node, build_state_t& state);

    std::vector<aligned_node_t> m_nodes;
    std::vector<index_type> m_reordered_indices;

    primitive_split_func m_split_primitive;
  };
}