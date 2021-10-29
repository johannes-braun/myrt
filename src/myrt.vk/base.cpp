#include "base.hpp"
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>
#include <unordered_set>
#include <vector>
#include <iostream>

#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace myrt::vulkan {
std::vector<char const*> filter_extensions(
    std::unordered_set<std::string_view>& desired, std::vector<vk::ExtensionProperties> const& properties) {
  std::vector<char const*> found_extensions;
  for (auto const& ext : properties) {
    std::printf("extension: %s\n", ext.extensionName);
    if (desired.contains(std::string_view(ext.extensionName))) {
      std::printf("extension found: %s\n", ext.extensionName);
      found_extensions.push_back(ext.extensionName);
      desired.erase(std::string_view(ext.extensionName));
    }
  }
  for (auto const& e : desired) std::printf("extension not found: %s\n", e.data());
  return found_extensions;
}

std::vector<char const*> filter_layers(
    std::unordered_set<std::string_view>& desired, std::vector<vk::LayerProperties> const& properties) {
  std::vector<char const*> found_layers;
  for (auto const& layer : properties) {
    std::printf("layer: %s\n", layer.layerName);
    if (desired.contains(std::string_view(layer.layerName))) {
      std::printf("layer found: %s\n", layer.layerName);
      found_layers.push_back(layer.layerName);
      desired.erase(std::string_view(layer.layerName));
    }
  }
  for (auto const& e : desired) std::printf("extension not found: %s\n", e.data());

  return found_layers;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
  std::cerr << "-- -- -- \n" << pCallbackData->pMessage << "\n";

  return VK_FALSE;
}

base::~base() {
  if (m_debug_messenger)
    m_instance->destroyDebugUtilsMessengerEXT(m_debug_messenger, nullptr, m_dynamic_dispatch);
  if (m_allocator)
    vmaDestroyAllocator(m_allocator);
  if (m_window)
    glfwDestroyWindow(m_window);
}

vk::Instance const& base::instance() const {
  return m_instance.get();
}

vk::Device const& base::device() const {
  return m_device.get();
}

vk::PhysicalDevice const& base::physical_device() const {
  return m_physical_device;
}

vk::SurfaceKHR const& base::surface() const {
  return m_surface.get();
}

GLFWwindow* const& base::window() const {
  return m_window;
}

ImGuiContext* const& base::imgui_context() const {
  return m_imgui_context;
}

VmaAllocator const& base::allocator() const {
  return m_allocator;
}

vk::SwapchainKHR const& base::swapchain() const {
  return m_swapchain.get();
}

vk::SurfaceFormatKHR const& base::surface_format() const {
  return m_surface_format;
}

std::vector<vk::Image> const& base::swapchain_images() const {
  return m_swapchain_images;
}

std::vector<vk::UniqueImageView> const& base::swapchain_views() const {
  return m_swapchain_views;
}

base::base(int width, int height) {
  glfwInit();
#pragma region InstanceCreation
  auto instance_extensions = vk::enumerateInstanceExtensionProperties();
  std::unordered_set<std::string_view> desired_extensions;
  if constexpr (debug_layers) {
    desired_extensions.emplace(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    desired_extensions.emplace(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    desired_extensions.emplace(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
  }

  std::uint32_t num_ext;
  char const* const* arr = glfwGetRequiredInstanceExtensions(&num_ext);
  for (int i = 0; i < num_ext; ++i) {
    desired_extensions.emplace(arr[i]);
  }

  auto found_extensions = filter_extensions(desired_extensions, instance_extensions);
  auto instance_layers = vk::enumerateInstanceLayerProperties();
  std::unordered_set<std::string_view> desired_layers;

  if constexpr (debug_layers) {
    desired_layers.emplace("VK_LAYER_KHRONOS_validation");
  }

  auto found_layers = filter_layers(desired_layers, instance_layers);

  vk::ApplicationInfo app_info(
      app_name, app_version, engine_name, engine_version, api_version);

  vk::InstanceCreateInfo instance_info;
  vk::ValidationFeaturesEXT validation_features;
  if constexpr (debug_layers) {
    std::array enable_validations{vk::ValidationFeatureEnableEXT::eBestPractices,
        vk::ValidationFeatureEnableEXT::eDebugPrintf, vk::ValidationFeatureEnableEXT::eSynchronizationValidation};

    validation_features.setEnabledValidationFeatures(enable_validations);
    instance_info.pNext = &validation_features;
  }

  instance_info.pApplicationInfo = &app_info;
  instance_info.ppEnabledExtensionNames = found_extensions.data();
  instance_info.enabledExtensionCount = found_extensions.size();
  instance_info.ppEnabledLayerNames = found_layers.data();
  instance_info.enabledLayerCount = found_layers.size();
  m_instance = vk::createInstanceUnique(instance_info);
#pragma endregion

#pragma region CreateSurface
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  m_window = glfwCreateWindow(width, height, app_name, nullptr, nullptr);

  {
    VkSurfaceKHR surface_impl;
    auto err = glfwCreateWindowSurface(m_instance.get(), m_window, nullptr, &surface_impl);
    m_surface = vk::UniqueSurfaceKHR(surface_impl, {});
  }
  m_imgui_context = ImGui::CreateContext();
  ImGui::SetCurrentContext(m_imgui_context);

  ImGui_ImplGlfw_InitForVulkan(m_window, true);
  ImGui::GetIO().DisplaySize = ImVec2(width, height);
#pragma endregion

  auto physical_devices = m_instance->enumeratePhysicalDevices();

  for (auto const& pd : physical_devices) {
    auto properties = pd.getProperties();
    std::cout << "Device detected: " << properties.deviceName.data() << '\n';
  }
  m_physical_device = physical_devices[0];
  auto device_extensions = m_physical_device.enumerateDeviceExtensionProperties();
  auto device_layers = m_physical_device.enumerateDeviceLayerProperties();
  auto features = m_physical_device.getFeatures2();

  struct queue_index {
    std::uint32_t family;
    std::uint32_t index;
  };
  std::vector<queue_index> queues;

  struct family_members {
    std::uint32_t index;
    std::uint32_t count = 0;
    std::vector<float> priorities;
  };
  std::unordered_map<std::uint32_t, family_members> queue_family_members;

  auto queueFamilyProperties = m_physical_device.getQueueFamilyProperties();
  auto const find_queue = [&](float priority, auto&& predicate) -> std::uint32_t {
    for (auto it = queueFamilyProperties.begin(); it != queueFamilyProperties.end(); ++it) {
      if (predicate(it)) {
        auto index = std::uint32_t(std::distance(queueFamilyProperties.begin(), it));
        auto& members = queue_family_members[index];
        members.index = index;
        members.priorities.push_back(priority);
        queues.push_back(queue_index{.family = members.index, .index = std::uint32_t(members.count)});
        ++members.count;
        return queues.size() - 1;
      }
    }
    return -1;
  };

  m_compute_queue_index = find_queue(1.0f, [](auto it) { return it->queueFlags & vk::QueueFlagBits::eCompute; });
  m_graphics_queue_index = find_queue(1.0f, [](auto it) { return it->queueFlags & vk::QueueFlagBits::eGraphics; });
  m_transfer_queue_index = find_queue(0.8f, [](auto it) { return it->queueFlags & vk::QueueFlagBits::eTransfer; });
  m_present_queue_index = find_queue(0.5f, [&](auto it) {
    return m_physical_device.getSurfaceSupportKHR(std::distance(begin(queueFamilyProperties), it), m_surface.get());
  });

  std::vector<vk::DeviceQueueCreateInfo> queue_infos;
  for (auto const& [iter, members] : queue_family_members) {
    vk::DeviceQueueCreateInfo queue_info;
    queue_info.queueCount = members.count;
    queue_info.queueFamilyIndex = members.index;
    queue_info.pQueuePriorities = members.priorities.data();
    queue_infos.push_back(queue_info);
  }

  std::unordered_set<std::string_view> desired_device_extensions;
  if constexpr (debug_layers) {
    desired_device_extensions.emplace(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }
  desired_device_extensions.emplace(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  desired_device_extensions.emplace(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
  desired_device_extensions.emplace(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME);
  desired_device_extensions.emplace(VK_KHR_SHADER_CLOCK_EXTENSION_NAME);

  auto found_device_extensions = filter_extensions(desired_device_extensions, device_extensions);
  std::unordered_set<std::string_view> desired_device_layers;
  if constexpr (debug_layers) {
    desired_device_layers.emplace("VK_LAYER_KHRONOS_validation");
  }
  auto found_device_layers = filter_layers(desired_device_layers, device_layers);

  vk::PhysicalDeviceShaderClockFeaturesKHR sclock;
  sclock.shaderDeviceClock = true;

  vk::PhysicalDeviceSynchronization2FeaturesKHR sync2;
  sync2.pNext = &sclock;
  sync2.synchronization2 = true;

  vk::DeviceCreateInfo device_info;
  device_info.pNext = &sync2;
  device_info.pEnabledFeatures = &features.features;
  device_info.ppEnabledExtensionNames = found_device_extensions.data();
  device_info.enabledExtensionCount = found_device_extensions.size();
  device_info.ppEnabledLayerNames = found_device_layers.data();
  device_info.enabledLayerCount = found_device_layers.size();
  device_info.queueCreateInfoCount = queue_infos.size();
  device_info.pQueueCreateInfos = queue_infos.data();

  m_device = m_physical_device.createDeviceUnique(device_info);
  m_dynamic_dispatch =
      vk::DispatchLoaderDynamic(m_instance.get(), vkGetInstanceProcAddr, m_device.get(), vkGetDeviceProcAddr);
  
  if constexpr (debug_layers) {
    vk::DebugUtilsMessengerCreateInfoEXT msg_info;
    msg_info.messageSeverity =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    msg_info.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
    msg_info.pfnUserCallback = debugCallback;

    m_debug_messenger = m_instance->createDebugUtilsMessengerEXT(msg_info, nullptr, m_dynamic_dispatch);
  }

  m_queues.resize(size_t(queue_kind::total_queue_count));
  m_queue_indices.resize(size_t(queue_kind::total_queue_count));

  m_queues[m_compute_queue_index] = device().getQueue(queues[m_compute_queue_index].family, queues[m_compute_queue_index].index);
  m_queues[m_present_queue_index] =
      device().getQueue(queues[m_present_queue_index].family, queues[m_present_queue_index].index);
  m_queues[m_transfer_queue_index] =
      device().getQueue(queues[m_transfer_queue_index].family, queues[m_transfer_queue_index].index);
  m_queues[m_graphics_queue_index] =
      device().getQueue(queues[m_graphics_queue_index].family, queues[m_graphics_queue_index].index);

  VmaAllocatorCreateInfo alloc_info{};
  alloc_info.device = m_device.get();
  alloc_info.instance = m_instance.get();
  alloc_info.physicalDevice = m_physical_device;
  alloc_info.preferredLargeHeapBlockSize = 1 << 29;
  vmaCreateAllocator(&alloc_info, &m_allocator);

  create_swapchain(width, height);
}

vk::DispatchLoaderStatic const& base::loader_static() const {
  return m_static_dispatch;
}
vk::DispatchLoaderDynamic const& base::loader_dynamic() const {
  return m_dynamic_dispatch;
}
vk::Queue const& base::queue(queue_kind q) const {
  return m_queues[queue_index(q)];
}
std::uint32_t base::queue_family(queue_kind q) const {
  return m_queue_indices[queue_index(q)];
}
void base::resize() {
  int ww, wh;
  glfwGetWindowSize(m_window, &ww, &wh);
  create_swapchain(ww, wh);
}
std::int32_t base::queue_index(queue_kind q) const {
  switch (q) {
    case myrt::vulkan::queue_kind::graphics: return m_graphics_queue_index;
    case myrt::vulkan::queue_kind::transfer: return m_transfer_queue_index;
    case myrt::vulkan::queue_kind::compute: return m_compute_queue_index;
    case myrt::vulkan::queue_kind::present: return m_present_queue_index;
    default: break;
  }
}
void base::create_swapchain(std::uint32_t w, std::uint32_t h) {
  if (m_swapchain) {
    m_swapchain_views.clear();
    m_swapchain_images.clear();
  }
  auto const surface_caps = m_physical_device.getSurfaceCapabilitiesKHR(m_surface.get());
  auto const formats = m_physical_device.getSurfaceFormatsKHR(m_surface.get());
  auto const presentModes = m_physical_device.getSurfacePresentModesKHR(m_surface.get());

  m_surface_format = formats[1];
  auto present_mode = vk::PresentModeKHR::eMailbox;
  vk::SwapchainCreateInfoKHR swapchain_info;
  swapchain_info.oldSwapchain = m_swapchain.get();
  swapchain_info.surface = m_surface.get();
  swapchain_info.imageColorSpace = m_surface_format.colorSpace;
  swapchain_info.imageArrayLayers = 1;
  swapchain_info.imageExtent = vk::Extent2D(w, h);
  swapchain_info.imageFormat = m_surface_format.format;
  swapchain_info.imageSharingMode = vk::SharingMode::eExclusive;
  swapchain_info.clipped = true;
  swapchain_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  swapchain_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
  swapchain_info.minImageCount =
      std::clamp<std::uint32_t>(swapchain_length, surface_caps.minImageCount, surface_caps.maxImageCount);

  std::unordered_set<uint32_t> families(std::begin(m_queue_indices), std::end(m_queue_indices));
  std::vector<uint32_t> final_families(begin(families), end(families));
  if (final_families.size() > 1) {
    swapchain_info.imageSharingMode = vk::SharingMode::eConcurrent;
  }
  swapchain_info.setQueueFamilyIndices(final_families);
  swapchain_info.presentMode = present_mode;
  swapchain_info.preTransform = surface_caps.currentTransform;
  m_swapchain = m_device->createSwapchainKHRUnique(swapchain_info, nullptr, m_dynamic_dispatch);
  m_swapchain_images = m_device->getSwapchainImagesKHR(m_swapchain.get());

  for (auto const& image : m_swapchain_images) {
    vk::ImageViewCreateInfo info;
    info.image = image;
    info.components = vk::ComponentMapping();
    info.format = m_surface_format.format;
    info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.viewType = vk::ImageViewType::e2D;
    m_swapchain_views.push_back(m_device->createImageViewUnique(info, nullptr, m_static_dispatch));
  }
}
} // namespace myrt::vulkan