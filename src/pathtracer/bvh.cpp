#include "bvh.hpp"
#include <numeric>
#include <algorithm>
#include <format>
#include <iostream>

namespace myrt
{
  struct sah_axis_info_t
  {
    std::array<aabb_t, bvh::binned_sah_bin_count> axis_bins{};
    std::array<int, bvh::binned_sah_bin_count> axis_counts{};
    std::array<aabb_t, bvh::binned_sah_bin_count> accum_aabbs{};
    std::array<int, bvh::binned_sah_bin_count> accum_counts{};
  };

  struct build_state_t
  {
    aabb_t full_aabb;
    std::vector<aabb_t> aabbs;
    std::vector<sah_axis_info_t> sah_axises;
  };

  namespace {
    [[nodiscard]] std::tuple<int, float, bool> compute_split_axis(bvh_node_t const& node, build_state_t& state)
    {
      aabb_t sub_aabb;
      auto const count = node.second_child - node.first_child + 1;
      for (auto index = node.first_child; index <= node.second_child; ++index)
      {
        const auto centroid = state.aabbs[index].centroid();
        sub_aabb.enclose(centroid);
      }

      auto const sub_aabb_size = sub_aabb.dimension();
      auto const step = sub_aabb_size / float(bvh::binned_sah_bin_count);

      auto& axises = state.sah_axises;

      for (auto index = node.first_child; index <= node.second_child; ++index)
      {
        const auto centroid = state.aabbs[index].centroid();
        auto relative = centroid - sub_aabb.min;
        auto integral = relative / step;
        rnu::vec3i const bin_id = rnu::clamp(rnu::vec3i(integral), rnu::vec3i(0), rnu::vec3i(bvh::binned_sah_bin_count - 1));
        axises[0].axis_bins[bin_id.x].enclose(state.aabbs[index]);
        axises[1].axis_bins[bin_id.y].enclose(state.aabbs[index]);
        axises[2].axis_bins[bin_id.z].enclose(state.aabbs[index]);
        axises[0].axis_counts[bin_id.x]++;
        axises[1].axis_counts[bin_id.y]++;
        axises[2].axis_counts[bin_id.z]++;
      }

      const auto enclose_reduce = [](aabb_t current, const aabb_t& next) -> aabb_t { current.enclose(next); return current; };
      for (int axis = 0; axis < 3; ++axis)
      {
        std::partial_sum(begin(axises[axis].axis_bins), end(axises[axis].axis_bins), begin(axises[axis].accum_aabbs), enclose_reduce);
        std::partial_sum(begin(axises[axis].axis_counts), end(axises[axis].axis_counts), begin(axises[axis].accum_counts));
      }

      const float sap = sub_aabb.surface_area();
      float best_cost = std::numeric_limits<float>::max();
      int best_axis = 0;
      int best_axis_partition = 0;
      for (int axis = 0; axis < 3; ++axis)
      {
        for (size_t index = 0; index < bvh::binned_sah_bin_count; ++index)
        {
          const auto count_first = axises[axis].accum_counts[index];
          const auto count_second = axises[axis].accum_counts[bvh::binned_sah_bin_count - 1 - index];
          const auto area_first = axises[axis].accum_aabbs[index].surface_area();
          const auto area_second = axises[axis].accum_aabbs[bvh::binned_sah_bin_count - 1 - index].surface_area();
          const auto cost = count_first * area_first * area_first / sap + count_second * area_second * area_second / sap;

          if (cost < best_cost)
          {
            best_axis = axis;
            best_axis_partition = static_cast<int>(index);
            best_cost = cost;
          }
        }
      }

      float const cost_without_split = sap * (node.second_child - node.first_child - 1);
      float const cost_factor = best_cost / cost_without_split;

      return std::make_tuple(best_axis, sub_aabb.min[best_axis] + step[best_axis] * float(best_axis_partition), cost_factor < 3);
    }
  }

  [[nodiscard]] std::vector<aabb_t> generate_triangle_bounds(std::span<detail::default_index_type const> indices, std::function<detail::default_point_type(detail::default_index_type)> const& get_point)
  {
    std::vector<aabb_t> aabbs(indices.size() / 3);
    for (size_t i = 0; i < indices.size(); i += 3)
    {
      const auto& p0 = get_point(indices[i + 0]);
      const auto& p1 = get_point(indices[i + 1]);
      const auto& p2 = get_point(indices[i + 2]);

      aabb_t& aabb = aabbs[i / 3];
      aabb.enclose(p0);
      aabb.enclose(p1);
      aabb.enclose(p2);
      aabb.pad(1e-3f);
      aabb.weighted_centroid = (p0 + p1 + p2) / 3.0f;
    }
    return aabbs;
  }

  std::vector<aabb_t> myrt::generate_triangle_bounds(std::span<detail::default_index_type const> indices, std::span<detail::default_point_type const> points)
  {
    std::vector<aabb_t> aabbs(indices.size() / 3);
    for (size_t i = 0; i < indices.size(); i += 3)
    {
      const auto& p0 = points[indices[i + 0]];
      const auto& p1 = points[indices[i + 1]];
      const auto& p2 = points[indices[i + 2]];

      aabb_t& aabb = aabbs[i / 3];
      aabb.enclose(p0);
      aabb.enclose(p1);
      aabb.enclose(p2);
      aabb.pad(1e-3f);
      aabb.weighted_centroid = (p0 + p1 + p2) / 3.0f;
    }
    return aabbs;
  }

  bvh::bvh(std::span<aabb_t const> aabbs)
  {
    build_state_t build_state;
    build_state.aabbs = { aabbs.begin(),aabbs.end() };

    for (size_t i = 0; i < aabbs.size(); ++i)
      build_state.full_aabb.enclose(aabbs[i]);

    create(build_state);
  }

  std::experimental::generator<bvh::index_type> bvh::traverse(ray_t const& ray) const
  {
    using bit_stack = std::uint32_t;

    bit_stack visited_stack = 0;   // 1 -> node not fully done yet.
    bit_stack left_right_stack = 0;// 1 -> visited second,must visit first.

    index_type node_index = 0;

    const auto t_scene = ray.intersect(m_nodes[node_index].node.aabb());

    while (t_scene.has_value())
    {
      bvh_node_t const* current = &m_nodes[node_index].node;
      while (!current->is_leaf()) {
        bvh_node_t const& first = m_nodes[current->first_child].node;
        bvh_node_t const& second = m_nodes[current->second_child].node;
        const auto aabb_curr = current->aabb();
        const auto aabb_first = first.aabb();
        const auto aabb_right = second.aabb();

        const auto t_left = ray.intersect(first.aabb());
        const auto t_right = ray.intersect(second.aabb());

        if (!t_left.has_value() && !t_right.has_value())
          break;

        left_right_stack = (t_left.has_value() && t_right.has_value() && t_left.value() > t_right.value()) | (left_right_stack << 1);
        visited_stack = (t_left.has_value() && t_right.has_value()) | (visited_stack << 1);

        node_index = (!t_left.has_value() || (t_right.has_value() && (left_right_stack & 0x1))) ? current->second_child : current->first_child;
        current = &m_nodes[node_index].node;
      }

      if (current->is_leaf())
        for (auto candidate = current->first_child; candidate <= current->second_child; ++candidate)
          co_yield m_reordered_indices[candidate];

      while (!(visited_stack & 1))
      {
        if (!visited_stack)
          co_return;

        node_index = m_nodes[node_index].node.parent();
        visited_stack >>= 1;
        left_right_stack >>= 1;
      }

      node_index = (left_right_stack & 0x1)
        ? m_nodes[m_nodes[node_index].node.parent()].node.first_child
        : m_nodes[m_nodes[node_index].node.parent()].node.second_child;
      visited_stack ^= 1;
    }
  }

  std::vector<aligned_node_t> const& bvh::nodes() const noexcept
  {
    return m_nodes;
  }

  std::vector<bvh::index_type> const& bvh::reordered_indices() const noexcept
  {
    return m_reordered_indices;
  }

  void bvh::create(build_state_t& initial_state)
  {
    m_reordered_indices.resize(initial_state.aabbs.size());
    std::iota(begin(m_reordered_indices), end(m_reordered_indices), index_type(0));
    m_nodes.reserve(2 * m_reordered_indices.size());

    index_type current_node_index = 0;
    ptrdiff_t active_nodes = 1;

    m_nodes.emplace_back().node = bvh_node_t{
        .min_extents = initial_state.full_aabb.min,
        .type_and_parent = bvh_node_t::make_type_and_parent(true,0),
        .max_extents = initial_state.full_aabb.max,
        .first_child = 0,
        .second_child = static_cast<index_type>(m_reordered_indices.size()) - 1
    };

    std::vector<size_t> depths;
    depths.reserve(2 * m_reordered_indices.size());
    depths.emplace_back(0);

    while (active_nodes--)
    {
      if (depths[current_node_index] + 1 < 32)
      {
        bvh_node_t& current_node = m_nodes[current_node_index].node;

        initial_state.sah_axises.clear();
        initial_state.sah_axises.resize(3);
        const auto split_nodes = split(current_node, initial_state);

        if (split_nodes.has_value())
        {
          const auto& [first, second] = split_nodes.value();
          active_nodes += 2;
          current_node.first_child = static_cast<index_type>(m_nodes.size());
          current_node.second_child = static_cast<index_type>(m_nodes.size() + 1);
          current_node.make_leaf(false);

          m_nodes.emplace_back(aligned_node_t{ .node = first }).node.set_parent(current_node_index);
          m_nodes.emplace_back(aligned_node_t{ .node = second }).node.set_parent(current_node_index);
          depths.emplace_back(depths[current_node_index] + 1);
          depths.emplace_back(depths[current_node_index] + 1);
        }
      }
      current_node_index++;
    }
    m_nodes.shrink_to_fit();

    size_t max_depth = *std::ranges::max_element(depths);
    std::format_to(std::ostreambuf_iterator(std::cout), "Max depth was {}\n", max_depth);

    return;
  }
  std::optional<std::pair<bvh_node_t, bvh_node_t>> bvh::split(const bvh_node_t& current_node, build_state_t& build_state)
  {
    if (current_node.second_child - current_node.first_child < max_leaf_primitives)
      return std::nullopt;

    const auto [split_axis, split_plane, should_split] = compute_split_axis(current_node, build_state);

    if (!should_split)
      return std::nullopt;

    const auto split_test = [&build_state, axis = split_axis, plane = split_plane](size_t index) {
      return build_state.aabbs[index].centroid()[axis] < plane;
    };

    // Partition
    auto first = current_node.first_child;
    auto last = current_node.second_child + 1;
    while (first != last && split_test(first)) first++;
    if (first != last)
    {
      for (auto i = first + 1; i != last; ++i) {
        if (split_test(i)) {
          std::swap(m_reordered_indices[i], m_reordered_indices[first]);
          std::swap(build_state.aabbs[i], build_state.aabbs[first]);
          ++first;
        }
      }
    }
    const index_type split_index = first == current_node.first_child ? first + 1 : first;

    std::pair<bvh_node_t, bvh_node_t> split_result;
    split_result.first.first_child = current_node.first_child;
    split_result.first.second_child = split_index - 1;
    split_result.second.first_child = split_index;
    split_result.second.second_child = current_node.second_child;

    aabb_t first_aabb;
    for (auto i = split_result.first.first_child; i <= split_result.first.second_child; ++i)
      first_aabb.enclose(build_state.aabbs[i]);
    split_result.first.set_aabb(first_aabb);
    aabb_t second_aabb;
    for (auto i = split_result.second.first_child; i <= split_result.second.second_child; ++i)
      second_aabb.enclose(build_state.aabbs[i]);
    split_result.second.set_aabb(second_aabb);

    return split_result;
  }

  std::optional<float> ray_t::intersect(const rnu::vec3 v1, const rnu::vec3 v2, const rnu::vec3 v3, rnu::vec2& barycentric)
  {
    constexpr float float_epsilon = 1e-23f;
    constexpr float border_epsilon = 1e-6f;
    rnu::vec3 e1 = v2 - v1;
    rnu::vec3 e2 = v3 - v1;
    rnu::vec3 P = cross(rnu::vec3(direction), e2);
    float det = dot(e1, P);
    if (det > -float_epsilon && det < float_epsilon)
      return false;
    float inv_det = 1.0f / det;
    rnu::vec3 T = origin - v1;
    barycentric.x = dot(T, P) * inv_det;
    if (barycentric.x < -border_epsilon || barycentric.x > 1.0f + border_epsilon)
      return false;
    rnu::vec3 Q = cross(T, e1);
    barycentric.y = dot(rnu::vec3(direction), Q) * inv_det;
    if (barycentric.y < -border_epsilon || barycentric.x + barycentric.y > 1.0f + border_epsilon)
      return false;
    float tt = dot(e2, Q) * inv_det;
    if (tt > float_epsilon) {
      return tt;
    }
    return std::nullopt;
  }

  std::optional<float> ray_t::intersect(aabb_t const& aabb) const noexcept
  {
    rnu::vec3 inv_direction = 1.f / direction;
    rnu::vec3 t135 = (aabb.min - origin) * inv_direction;
    rnu::vec3 t246 = (aabb.max - origin) * inv_direction;

    rnu::vec3 min_values = min(t135, t246);
    rnu::vec3 max_values = max(t135, t246);

    float tmin = rnu::max(rnu::max(min_values.x, min_values.y), min_values.z);
    float tmax = rnu::min(rnu::min(max_values.x, max_values.y), max_values.z);

    return (tmax >= 0 && tmin <= tmax && tmin <= length) ? std::optional(tmin < 0 ? tmax : tmin) : std::nullopt;
  }

  aabb_t bvh::aabb() const noexcept
  {
    return m_nodes[0].node.aabb();
  }
}