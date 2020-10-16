#pragma once

#include <glm/glm.hpp>
#include <array>
#include <cstddef>
#include <cinttypes>
#include <bit>
#include <vector>
#include <span>
#include <ranges>
#include <optional>
#include <functional>
#include <string_view>
#include <experimental/generator>

namespace myrt
{
	struct build_state_t;

	namespace detail
	{
		namespace glsl
		{
			constexpr char const bvh_struct_name[] = "bvh_node_t";
			constexpr char const bvh_node_buffer_name[] = "BVH_NODES";
			constexpr char const bvh_index_type[] = "uint";
			constexpr char const bvh_indices_buffer_name[] = "BVH_INDICES";
			constexpr char const bvh_point_type[] = "vec3";

			// Fixed signature: bool(index_type index, inout float tmax);
			constexpr char const bvh_visit_primitive_function_name[] = "myrt_bvh_visit_primitive";
		}

		using default_point_type = glm::vec3;
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

		constexpr void enclose(point_type point) noexcept {
			min = glm::min(min, point);
			max = glm::max(max, point);
		}
		constexpr void enclose(aabb_t aabb) noexcept {
			min = glm::min(min, aabb.min);
			max = glm::max(max, aabb.max);
		}
		constexpr void pad(float padding) noexcept {
			min -= padding;
			max += padding;
		}
	};

	struct ray_t
	{
		glm::vec3 origin;
		glm::vec3 direction;
		float length;

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

	class bvh
	{
	public:
		using index_type = detail::default_index_type;
		using point_type = detail::default_point_type;

		constexpr static size_t binned_sah_bin_count = 64;
		constexpr static int max_leaf_primitives = 1;

		[[nodiscard]] bvh(std::span<aabb_t const> aabbs);
		[[nodiscard]] std::experimental::generator<index_type> traverse(ray_t const& ray) const;
		[[nodiscard]] std::vector<aligned_node_t> const& nodes() const noexcept;
		[[nodiscard]] std::vector<index_type> const& reordered_indices() const noexcept;
		[[nodiscard]] aabb_t aabb() const noexcept;
		
	private:
		void create(build_state_t& initial_state);
		[[nodiscard]] std::optional<std::pair<bvh_node_t, bvh_node_t>> split(const bvh_node_t& current_node, build_state_t& build_state);

		std::vector<aligned_node_t> m_nodes;
		std::vector<index_type> m_reordered_indices;
	};

	namespace glsl {
		[[nodiscard]] std::string bvh_definitions_code(std::string const& traversal_name);
		[[nodiscard]] std::string bvh_code(std::string const& traversal_name, std::string const& primitive_name, std::string const& nodes, std::string const& indices);
		[[nodiscard]] std::string intersect_triangle_code();
	}
}