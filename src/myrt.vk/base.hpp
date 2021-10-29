#pragma once

#include <vulkan/vulkan.hpp>
#include <imgui.h>
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>

namespace myrt::vulkan {
enum class queue_kind { graphics, transfer, compute, present, total_queue_count };

class base {
public:
  static constexpr auto app_name = "MYRT";
  static constexpr auto app_version = VK_MAKE_API_VERSION(0, 1, 0, 0);
  static constexpr auto engine_name = "MYRT_ENG";
  static constexpr auto engine_version = VK_MAKE_API_VERSION(0, 1, 0, 0);
  static constexpr auto api_version = VK_VERSION_1_2;
  static constexpr auto debug_layers = true;
  static constexpr auto swapchain_length = 3;

  base(int width, int height);
  ~base();

  vk::Instance const& instance() const;
  vk::Device const& device() const;
  vk::PhysicalDevice const& physical_device() const;
  vk::SurfaceKHR const& surface() const;

  GLFWwindow* const& window() const;
  ImGuiContext* const& imgui_context() const;
  VmaAllocator const& allocator() const;

  vk::SwapchainKHR const& swapchain() const;
  vk::SurfaceFormatKHR const& surface_format() const;
  std::vector<vk::Image> const& swapchain_images() const;
  std::vector<vk::UniqueImageView> const& swapchain_views() const;

  vk::DispatchLoaderStatic const& loader_static() const;
  vk::DispatchLoaderDynamic const& loader_dynamic() const;

  vk::Queue const& queue(queue_kind q) const;
  std::uint32_t queue_family(queue_kind q) const;

  void resize();

private:
  std::int32_t queue_index(queue_kind q) const;
  void create_swapchain(std::uint32_t w, std::uint32_t h);

  vk::DispatchLoaderStatic m_static_dispatch;
  vk::DispatchLoaderDynamic m_dynamic_dispatch;

  vk::UniqueInstance m_instance;
  vk::UniqueDevice m_device;
  vk::PhysicalDevice m_physical_device;
  vk::UniqueSurfaceKHR m_surface;
  vk::DebugUtilsMessengerEXT m_debug_messenger;
  vk::UniqueHandle<vk::SwapchainKHR, vk::DispatchLoaderDynamic> m_swapchain;
  vk::SurfaceFormatKHR m_surface_format;
  std::vector<vk::Image> m_swapchain_images;
  std::vector<vk::UniqueImageView> m_swapchain_views;

  GLFWwindow* m_window;
  ImGuiContext* m_imgui_context;
  VmaAllocator m_allocator;

  std::int32_t m_compute_queue_index;
  std::int32_t m_graphics_queue_index;
  std::int32_t m_transfer_queue_index;
  std::int32_t m_present_queue_index;

  std::vector<vk::Queue> m_queues;
  std::vector<std::uint32_t> m_queue_indices;
};
} // namespace myrt::vulkan