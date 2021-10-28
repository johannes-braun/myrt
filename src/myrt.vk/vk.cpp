#if _WIN32
#  define VK_USE_PLATFORM_WIN32_KHR
#  define NOMINMAX
#  define WIN32_LEAN_AND_MEAN
#endif

#include <rnu/camera.hpp>
#include <rnu/math/math.hpp>

#include <vulkan/vulkan.hpp>

#include <shaderc/shaderc.hpp>
#include <string_view>
#include <unordered_set>
#include <iostream>
#include "shader_build.hpp"

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "../obj.hpp"

#include <GLFW/glfw3.h>

template <typename Fun> struct call_at_scope_end {
  call_at_scope_end(Fun&& f) : _f(std::forward<Fun>(f)) {}
  ~call_at_scope_end() {
    _f();
  }

private:
  Fun _f;
};

template <typename Fun> auto at_scope_end(Fun&& f) {
  return call_at_scope_end<Fun>(std::forward<Fun>(f));
}

constexpr bool debug_layers = false;
int initial_width = 1920;
int initial_height = 1200;
constexpr int image_count = 4;

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

vk::DispatchLoaderStatic sdl;

struct vk_image {
  vk_image() = default;
  vk_image(vk_image const&) = delete;
  vk_image(vk_image&& other) noexcept {
    allocation = std::exchange(other.allocation, nullptr);
    allocator = std::exchange(other.allocator, nullptr);
    image = std::move(other.image);
  }
  vk_image& operator=(vk_image const&) = delete;
  vk_image& operator=(vk_image&& other) noexcept {
    allocation = std::exchange(other.allocation, nullptr);
    allocator = std::exchange(other.allocator, nullptr);
    image = std::move(other.image);
    return *this;
  }

  ~vk_image() {
    if (allocator && allocation)
      vmaFreeMemory(allocator, allocation);
  }

  vk::UniqueImage image;
  VmaAllocator allocator;
  VmaAllocation allocation;
};

vk_image get_image(vk::Device device, VmaAllocator allocator, std::uint32_t w, std::uint32_t h, vk::Format fmt,
    vk::ImageUsageFlags usage) {
  vk::ImageCreateInfo img1;
  img1.arrayLayers = 1;
  img1.extent = vk::Extent3D{w, h, 1};
  img1.imageType = vk::ImageType::e2D;
  img1.initialLayout = vk::ImageLayout::eUndefined;
  img1.format = fmt;
  img1.mipLevels = 1;
  img1.samples = vk::SampleCountFlagBits::e1;
  img1.sharingMode = vk::SharingMode::eExclusive;
  img1.tiling = vk::ImageTiling::eOptimal;
  img1.usage = usage;
  auto img = device.createImageUnique(img1, nullptr, sdl);

  VmaAllocationCreateInfo img1_alloc_info{};
  VmaAllocation img1_mem;
  VmaAllocationInfo img1_allocx_info;
  vmaAllocateMemoryForImage(allocator, img.get(), &img1_alloc_info, &img1_mem, &img1_allocx_info);

  device.bindImageMemory(img.get(), img1_mem->GetMemory(), img1_mem->GetOffset());

  vk_image i{};
  i.allocation = img1_mem;
  i.image = std::move(img);
  i.allocator = allocator;
  return i;
}
template <class T> inline void hash_combine(std::size_t& seed, const T& v) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct swapchain_data {
  vk::SurfaceFormatKHR surface_format;
  vk::UniqueSwapchainKHR swapchain;
  std::vector<vk::Image> images;
  std::vector<vk::UniqueImageView> views;

  void reset() {
    swapchain.reset();
    images.clear();
    views.clear();
  }
};

swapchain_data create_swapchain(vk::Device device, vk::PhysicalDevice default_pd, vk::SurfaceKHR surface, uint32_t w,
    uint32_t h, vk::ArrayProxy<uint32_t const> queue_families) {
  swapchain_data result;
#pragma region CreateSwapchain
  auto const& surface_caps = default_pd.getSurfaceCapabilitiesKHR(surface);
  auto const& formats = default_pd.getSurfaceFormatsKHR(surface);
  auto const& presentModes = default_pd.getSurfacePresentModesKHR(surface);

  result.surface_format = formats[1];
  auto present_mode = vk::PresentModeKHR::eMailbox;
  vk::SwapchainCreateInfoKHR swapchain_info;
  swapchain_info.surface = surface;
  swapchain_info.imageColorSpace = result.surface_format.colorSpace;
  swapchain_info.imageArrayLayers = 1;
  swapchain_info.imageExtent = vk::Extent2D(initial_width, initial_height);
  swapchain_info.imageFormat = result.surface_format.format;
  swapchain_info.imageSharingMode = vk::SharingMode::eExclusive;
  swapchain_info.clipped = true;
  swapchain_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  swapchain_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
  swapchain_info.minImageCount =
      std::clamp<std::uint32_t>(image_count, surface_caps.minImageCount, surface_caps.maxImageCount);

  std::unordered_set<uint32_t> families(std::begin(queue_families), std::end(queue_families));
  std::vector<uint32_t> final_families(begin(families), end(families));
  if (final_families.size() > 1) {
    swapchain_info.imageSharingMode = vk::SharingMode::eConcurrent;
  }
  swapchain_info.setQueueFamilyIndices(final_families);
  swapchain_info.presentMode = present_mode;
  swapchain_info.preTransform = surface_caps.currentTransform;
  result.swapchain = device.createSwapchainKHRUnique(swapchain_info, nullptr, sdl);
  result.images = device.getSwapchainImagesKHR(result.swapchain.get());

  for (auto const& image : result.images) {
    vk::ImageViewCreateInfo info;
    info.image = image;
    info.components = vk::ComponentMapping();
    info.format = result.surface_format.format;
    info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.viewType = vk::ImageViewType::e2D;
    result.views.push_back(device.createImageViewUnique(info, nullptr, sdl));
  }
#pragma endregion
  return result;
}

vk::UniqueDescriptorSetLayout make_ds_layout(
    vk::Device device, vk::ArrayProxy<vk::DescriptorSetLayoutBinding const> bindings) {
  vk::DescriptorSetLayoutCreateInfo dsl_info;
  dsl_info.setBindings(vk::ArrayProxyNoTemporaries(bindings.size(), bindings.data()));
  return device.createDescriptorSetLayoutUnique(dsl_info, nullptr, sdl);
}

int main() {
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
  std::unordered_set<std::string_view> desired_layers{"VK_LAYER_KHRONOS_validation"};
  auto found_layers = filter_layers(desired_layers, instance_layers);

  vk::ApplicationInfo app_info(
      "MYRT", VK_MAKE_API_VERSION(0, 1, 0, 0), "MYRT_ENG", VK_MAKE_API_VERSION(0, 1, 0, 0), VK_VERSION_1_2);

  vk::InstanceCreateInfo instance_info;
  if constexpr (debug_layers) {
    std::array enable_validations{vk::ValidationFeatureEnableEXT::eBestPractices,
        vk::ValidationFeatureEnableEXT::eDebugPrintf, vk::ValidationFeatureEnableEXT::eSynchronizationValidation};

    vk::ValidationFeaturesEXT validation_features;
    validation_features.setEnabledValidationFeatures(enable_validations);
    instance_info.pNext = &validation_features;
  }

  instance_info.pApplicationInfo = &app_info;
  instance_info.ppEnabledExtensionNames = found_extensions.data();
  instance_info.enabledExtensionCount = found_extensions.size();
  instance_info.ppEnabledLayerNames = found_layers.data();
  instance_info.enabledLayerCount = found_layers.size();
  auto vk_instance = vk::createInstanceUnique(instance_info);
#pragma endregion

#pragma region CreateSurface
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> window(
      glfwCreateWindow(initial_width, initial_height, "Window", nullptr, nullptr), &glfwDestroyWindow);

  vk::UniqueSurfaceKHR surface;
  {
    VkSurfaceKHR surface_impl;
    auto err = glfwCreateWindowSurface(vk_instance.get(), window.get(), nullptr, &surface_impl);
    surface = vk::UniqueSurfaceKHR(surface_impl, {});
  }
  auto imgui_c = ImGui::CreateContext();
  ImGui::SetCurrentContext(imgui_c);

  ImGui_ImplGlfw_InitForVulkan(window.get(), true);
  ImGui::GetIO().DisplaySize = ImVec2(initial_width, initial_height);
#pragma endregion

  auto physical_devices = vk_instance->enumeratePhysicalDevices();

  for (auto const& pd : physical_devices) {
    auto properties = pd.getProperties();
    std::cout << "Device detected: " << properties.deviceName.data() << '\n';
  }
  auto default_pd = physical_devices[0];
  auto device_extensions = default_pd.enumerateDeviceExtensionProperties();
  auto device_layers = default_pd.enumerateDeviceLayerProperties();
  auto features = default_pd.getFeatures2();

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

  auto queueFamilyProperties = default_pd.getQueueFamilyProperties();
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

  auto compute_queue_index = find_queue(1.0f, [](auto it) { return it->queueFlags & vk::QueueFlagBits::eCompute; });
  auto graphics_queue_index = find_queue(1.0f, [](auto it) { return it->queueFlags & vk::QueueFlagBits::eGraphics; });
  auto transfer_queue_index = find_queue(0.8f, [](auto it) { return it->queueFlags & vk::QueueFlagBits::eTransfer; });
  auto present_queue_index = find_queue(0.5f, [&](auto it) {
    return default_pd.getSurfaceSupportKHR(std::distance(begin(queueFamilyProperties), it), surface.get());
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

  auto device = default_pd.createDeviceUnique(device_info);
  vk::DispatchLoaderDynamic ext(vk_instance.get(), vkGetInstanceProcAddr, device.get(), vkGetDeviceProcAddr);

  struct framebuffer_getter {
    void new_frame() {
      for (auto it = begin(framebuffers); it != end(framebuffers);) {
        if (it->second.used_in_frame <= 0) {
          std::cout << "Erasing unused Framebuffer...\n";
          it = framebuffers.erase(it);
        } else {
          it->second.used_in_frame--;
          ++it;
        }
      }
    }

    framebuffer_getter(vk::Device device) : m_device(device) {}

    vk::Framebuffer operator()(
        uint32_t w, uint32_t h, vk::RenderPass pass, vk::ArrayProxyNoTemporaries<vk::ImageView const> images) {
      size_t hash = 0;
      hash_combine(hash, w);
      hash_combine(hash, h);
      hash_combine<VkRenderPass>(hash, pass);

      for (auto const& iv : images) hash_combine<VkImageView>(hash, iv);

      auto& ufb = framebuffers[hash];
      if (!ufb.framebuffer) {
        std::cout << "Framebuffer not found. Creating new one:\n"
                  << "  w=" << w << "\n"
                  << "  h=" << h << "\n"
                  << "  with " << images.size() << " attachments\n";

        vk::FramebufferCreateInfo fbo_info;
        fbo_info.width = w;
        fbo_info.height = h;
        fbo_info.layers = 1;
        fbo_info.renderPass = pass;
        fbo_info.setAttachments(images);
        ufb.framebuffer = m_device.createFramebufferUnique(fbo_info, nullptr, sdl);
      }
      ufb.used_in_frame = 50;
      return ufb.framebuffer.get();
    }

    struct info {
      vk::UniqueFramebuffer framebuffer;
      int used_in_frame = 0;
    };

    vk::Device m_device;
    std::unordered_map<std::uint64_t, info> framebuffers;
  } get_framebuffer(device.get());

  vk::DebugUtilsMessengerEXT messenger;
  if constexpr (debug_layers) {
    vk::DebugUtilsMessengerCreateInfoEXT msg_info;
    msg_info.messageSeverity =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    msg_info.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
    msg_info.pfnUserCallback = debugCallback;

    messenger = vk_instance->createDebugUtilsMessengerEXT(msg_info, nullptr, ext);
  }

  auto compute_queue = device->getQueue(queues[compute_queue_index].family, queues[compute_queue_index].index);
  auto present_queue = device->getQueue(queues[present_queue_index].family, queues[present_queue_index].index);
  auto transfer_queue = device->getQueue(queues[transfer_queue_index].family, queues[transfer_queue_index].index);
  auto graphics_queue = device->getQueue(queues[graphics_queue_index].family, queues[graphics_queue_index].index);

  auto swapchain_data = create_swapchain(device.get(), default_pd, surface.get(), initial_width, initial_height,
      {queues[present_queue_index].family, queues[present_queue_index].family});

#pragma region imgui
  ImGui::GetIO().Fonts->AddFontFromFileTTF("../../../../../res/alata.ttf", 17.0f);

  ImVec4* colors = ImGui::GetStyle().Colors;
  colors[ImGuiCol_Text] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
  colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.94f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
  colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.54f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.37f, 0.14f, 0.14f, 0.67f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.39f, 0.20f, 0.20f, 0.67f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.48f, 0.16f, 0.16f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.48f, 0.16f, 0.16f, 1.00f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
  colors[ImGuiCol_CheckMark] = ImVec4(0.56f, 0.10f, 0.10f, 1.00f);
  colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 0.19f, 0.19f, 0.40f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.89f, 0.00f, 0.19f, 1.00f);
  colors[ImGuiCol_Button] = ImVec4(1.00f, 0.19f, 0.19f, 0.40f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.80f, 0.17f, 0.00f, 1.00f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.89f, 0.00f, 0.19f, 1.00f);
  colors[ImGuiCol_Header] = ImVec4(0.33f, 0.35f, 0.36f, 0.53f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.76f, 0.28f, 0.44f, 0.67f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.47f, 0.47f, 0.47f, 0.67f);
  colors[ImGuiCol_Separator] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
  colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.85f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
  colors[ImGuiCol_Tab] = ImVec4(0.07f, 0.07f, 0.07f, 0.51f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.86f, 0.23f, 0.43f, 0.67f);
  colors[ImGuiCol_TabActive] = ImVec4(0.19f, 0.19f, 0.19f, 0.57f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.05f, 0.05f, 0.05f, 0.90f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.13f, 0.13f, 0.13f, 0.74f);
  colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
  colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
  colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
  colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
  colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
  colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
  colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
  colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
  colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
  colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
  colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
  colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

  vk::AttachmentDescription imgui_color_attachment;
  imgui_color_attachment.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
  imgui_color_attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;
  imgui_color_attachment.format = swapchain_data.surface_format.format;
  imgui_color_attachment.loadOp = vk::AttachmentLoadOp::eLoad;
  imgui_color_attachment.storeOp = vk::AttachmentStoreOp::eStore;
  imgui_color_attachment.samples = vk::SampleCountFlagBits::e1;

  vk::AttachmentReference imgui_subpass_attachment(0, vk::ImageLayout::eColorAttachmentOptimal);
  vk::SubpassDescription imgui_subpass;
  imgui_subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
  imgui_subpass.setColorAttachments(imgui_subpass_attachment);

  /*
        istr.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
  istr.dstAccessMask = vk::AccessFlagBits2KHR::eColorAttachmentWrite;
  istr.dstQueueFamilyIndex = queues[graphics_queue_index].family;
  istr.dstStageMask = vk::PipelineStageFlagBits2KHR::eColorAttachmentOutput;*/

  vk::SubpassDependency imgui_start_dependency;
  imgui_start_dependency.dependencyFlags = vk::DependencyFlagBits::eByRegion;
  imgui_start_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  imgui_start_dependency.srcAccessMask =
      vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
  imgui_start_dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  imgui_start_dependency.dstSubpass = 0;
  imgui_start_dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
  imgui_start_dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

  vk::SubpassDependency imgui_end_dependency;
  imgui_end_dependency.dependencyFlags = vk::DependencyFlagBits::eByRegion;
  imgui_end_dependency.srcSubpass = 0;
  imgui_end_dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
  imgui_end_dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  imgui_end_dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
  imgui_end_dependency.dstAccessMask = vk::AccessFlagBits::eNoneKHR;
  imgui_end_dependency.dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;

  std::array imgui_subpass_dependencies{imgui_start_dependency, imgui_end_dependency};

  vk::RenderPassCreateInfo imgui_pass_info;
  imgui_pass_info.setAttachments(imgui_color_attachment);
  imgui_pass_info.setSubpasses(imgui_subpass);
  imgui_pass_info.setDependencies(imgui_subpass_dependencies);
  auto const imgui_renderpass = device->createRenderPassUnique(imgui_pass_info);

  vk::DescriptorPoolCreateInfo descriptor_pool_info;
  descriptor_pool_info.maxSets = 64;

  std::array pool_sizes = {vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 40}};
  descriptor_pool_info.setPoolSizes(pool_sizes);
  vk::UniqueDescriptorPool imgui_dpool = device->createDescriptorPoolUnique(descriptor_pool_info);

  ImGui_ImplVulkan_InitInfo imgui_info{};
  imgui_info.Instance = vk_instance.get();
  imgui_info.PhysicalDevice = default_pd;
  imgui_info.Device = device.get();
  imgui_info.MinImageCount = swapchain_data.images.size();
  imgui_info.ImageCount = swapchain_data.images.size();
  imgui_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  imgui_info.Queue = graphics_queue;
  imgui_info.QueueFamily = queues[graphics_queue_index].family;
  imgui_info.Subpass = 0;
  imgui_info.DescriptorPool = imgui_dpool.get();
  ImGui_ImplVulkan_Init(&imgui_info, imgui_renderpass.get());

  // vk::FramebufferCreateInfo imgui_fbo_info;
  // imgui_fbo_info.width = initial_width;
  // imgui_fbo_info.height = initial_height;
  // imgui_fbo_info.layers = 1;
  // imgui_fbo_info.renderPass = imgui_renderpass.get();

  // std::vector<vk::UniqueFramebuffer> imgui_framebuffers;
  // for (int i = 0; i < swapchain_views.size(); ++i) {
  //  imgui_fbo_info.setAttachments(swapchain_views[i].get());
  //  imgui_framebuffers.push_back(device->createFramebufferUnique(imgui_fbo_info));
  //}

#pragma endregion

  vk::CommandPoolCreateInfo graphics_command_pool_info;
  graphics_command_pool_info.queueFamilyIndex = queues[graphics_queue_index].family;
  auto graphics_command_pool = device->createCommandPoolUnique(graphics_command_pool_info);

  vk::CommandPoolCreateInfo transfer_pool_info;
  transfer_pool_info.queueFamilyIndex = queues[transfer_queue_index].family;
  auto transfer_pool = device->createCommandPoolUnique(transfer_pool_info);

  vk::SamplerCreateInfo smp_info;
  auto smp = device->createSamplerUnique(smp_info);

  VmaAllocatorCreateInfo alloc_info{};
  alloc_info.device = device.get();
  alloc_info.instance = vk_instance.get();
  alloc_info.physicalDevice = default_pd;
  VmaAllocator allocator;
  vmaCreateAllocator(&alloc_info, &allocator);

  auto cleanup_allocator = at_scope_end([&] { vmaDestroyAllocator(allocator); });

#pragma region BuildRenderPipeline
  auto const vert_shader = myrt::compile_file(shaderc_vertex_shader, "../../../../../src/myrt.vk/forward.vert");
  vk::ShaderModuleCreateInfo vertex_stage_info;
  vertex_stage_info.codeSize = sizeof(std::uint32_t) * vert_shader.size();
  vertex_stage_info.pCode = vert_shader.data();
  auto vertex_stage = device->createShaderModuleUnique(vertex_stage_info);

  auto const frag_shader = myrt::compile_file(shaderc_fragment_shader, "../../../../../src/myrt.vk/forward.frag");
  vk::ShaderModuleCreateInfo frag_shader_info;
  frag_shader_info.codeSize = sizeof(std::uint32_t) * frag_shader.size();
  frag_shader_info.pCode = frag_shader.data();
  auto fragment_stage = device->createShaderModuleUnique(frag_shader_info);

  vk::UniquePipelineLayout render_layout;
  {
    vk::PushConstantRange matrix_range;
    matrix_range.offset = 0;
    matrix_range.size = 2 * 4 * 4 * sizeof(float);
    matrix_range.stageFlags = vk::ShaderStageFlagBits::eVertex;

    vk::PipelineLayoutCreateInfo info;
    info.setPushConstantRanges(matrix_range);
    render_layout = device->createPipelineLayoutUnique(info);
  }

  std::array<vk::PipelineShaderStageCreateInfo, 2> stages;
  {
    vk::ShaderModuleCreateInfo module_info;
    stages[0].module = vertex_stage.get();
    stages[0].stage = vk::ShaderStageFlagBits::eVertex;
    stages[0].pName = "main";
    stages[1].module = fragment_stage.get();
    stages[1].stage = vk::ShaderStageFlagBits::eFragment;
    stages[1].pName = "main";
  }

  vk::PipelineVertexInputStateCreateInfo vertex_input;
  vk::VertexInputAttributeDescription pos_attr(0, 0, vk::Format::eR32G32B32Sfloat, 0);
  vk::VertexInputAttributeDescription norm_attr(1, 1, vk::Format::eR32G32B32Sfloat, 0);
  vk::VertexInputAttributeDescription uv_attr(2, 2, vk::Format::eR32G32Sfloat, 0);
  vk::VertexInputAttributeDescription transform_attr0(3, 3, vk::Format::eR32G32B32A32Sfloat, 0);
  vk::VertexInputAttributeDescription transform_attr1(4, 3, vk::Format::eR32G32B32A32Sfloat, (4) * sizeof(float));
  vk::VertexInputAttributeDescription transform_attr2(5, 3, vk::Format::eR32G32B32A32Sfloat, (4 + 4) * sizeof(float));
  vk::VertexInputAttributeDescription transform_attr3(
      6, 3, vk::Format::eR32G32B32A32Sfloat, (4 + 4 + 4) * sizeof(float));
  std::array attributes = {
      pos_attr, norm_attr, uv_attr, transform_attr0, transform_attr1, transform_attr2, transform_attr3};

  vk::VertexInputBindingDescription vertex_binding;
  vertex_binding.binding = 0;
  vertex_binding.stride = 3 * sizeof(float);
  vertex_binding.inputRate = vk::VertexInputRate::eVertex;
  vk::VertexInputBindingDescription normal_binding;
  normal_binding.binding = 1;
  normal_binding.stride = 3 * sizeof(float);
  normal_binding.inputRate = vk::VertexInputRate::eVertex;
  vk::VertexInputBindingDescription uv_binding;
  uv_binding.binding = 2;
  uv_binding.stride = 2 * sizeof(float);
  uv_binding.inputRate = vk::VertexInputRate::eVertex;

  vk::VertexInputBindingDescription transform_binding;
  transform_binding.binding = 3;
  transform_binding.stride = 4 * 4 * sizeof(float);
  transform_binding.inputRate = vk::VertexInputRate::eInstance;
  std::array render_bindings = {vertex_binding, normal_binding, uv_binding, transform_binding};
  vertex_input.setVertexAttributeDescriptions(attributes);
  vertex_input.setVertexBindingDescriptions(render_bindings);

  vk::PipelineDynamicStateCreateInfo dynamic_state;
  std::array dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
  dynamic_state.setDynamicStates(dynamic_states);

  vk::PipelineColorBlendAttachmentState attstate;
  attstate.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
  attstate.blendEnable = false;
  std::array blend_attachments{attstate, attstate, attstate};

  vk::PipelineColorBlendStateCreateInfo color_blend_state;
  color_blend_state.setAttachments(blend_attachments);
  color_blend_state.logicOpEnable = false;

  vk::PipelineRasterizationStateCreateInfo rasterization;
  rasterization.cullMode = vk::CullModeFlagBits::eBack;
  rasterization.frontFace = vk::FrontFace::eCounterClockwise;
  rasterization.polygonMode = vk::PolygonMode::eFill;
  rasterization.rasterizerDiscardEnable = false;
  rasterization.lineWidth = 1.0f;

  vk::PipelineDepthStencilStateCreateInfo depth_stencil;
  depth_stencil.depthBoundsTestEnable = false;
  depth_stencil.depthTestEnable = true;
  depth_stencil.depthWriteEnable = true;
  depth_stencil.depthCompareOp = vk::CompareOp::eLess;
  depth_stencil.stencilTestEnable = false;

  vk::PipelineInputAssemblyStateCreateInfo input_assembly;
  input_assembly.topology = vk::PrimitiveTopology::eTriangleList;

  vk::PipelineMultisampleStateCreateInfo multisample;
  multisample.sampleShadingEnable = false;
  multisample.rasterizationSamples = vk::SampleCountFlagBits::e1;

  vk::PipelineViewportStateCreateInfo viewport;
  viewport.scissorCount = 1;
  viewport.viewportCount = 1;

  vk::AttachmentDescription attachment0;
  attachment0.initialLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
  attachment0.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
  attachment0.format = vk::Format::eR16G16B16A16Sfloat;
  attachment0.loadOp = vk::AttachmentLoadOp::eClear;
  attachment0.samples = vk::SampleCountFlagBits::e1;
  attachment0.storeOp = vk::AttachmentStoreOp::eStore;
  vk::AttachmentDescription attachment1 = attachment0;
  vk::AttachmentDescription attachment2 = attachment0;

  vk::AttachmentDescription depth_attachment = attachment0;
  depth_attachment.format = vk::Format::eD24UnormS8Uint;
  depth_attachment.storeOp = vk::AttachmentStoreOp::eDontCare;
  depth_attachment.initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
  depth_attachment.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

  std::array renderpass_attachments = {attachment0, attachment1, attachment2, depth_attachment};

  vk::AttachmentReference attachment0ref;
  attachment0ref.attachment = 0;
  attachment0ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
  vk::AttachmentReference attachment1ref;
  attachment1ref.attachment = 1;
  attachment1ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
  vk::AttachmentReference attachment2ref;
  attachment2ref.attachment = 2;
  attachment2ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
  vk::AttachmentReference depthref;
  depthref.attachment = 3;
  depthref.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
  std::array subpass_attachments = {attachment0ref, attachment1ref, attachment2ref};

  vk::SubpassDescription subpass;
  subpass.inputAttachmentCount = 0;
  subpass.setColorAttachments(subpass_attachments);
  subpass.setPDepthStencilAttachment(&depthref);
  subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;

  vk::SubpassDependency subpass_dependency_start;
  subpass_dependency_start.srcSubpass = VK_SUBPASS_EXTERNAL;
  subpass_dependency_start.srcStageMask = vk::PipelineStageFlagBits::eTopOfPipe;
  subpass_dependency_start.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
  subpass_dependency_start.dstSubpass = 0;
  subpass_dependency_start.dstStageMask =
      vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
  subpass_dependency_start.dstAccessMask =
      vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

  vk::SubpassDependency subpass_dependency_end;
  subpass_dependency_end.srcSubpass = 0;
  subpass_dependency_end.srcStageMask =
      vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
  subpass_dependency_end.srcAccessMask =
      vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
  subpass_dependency_end.dstSubpass = VK_SUBPASS_EXTERNAL;
  subpass_dependency_end.dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
  subpass_dependency_end.dstAccessMask = vk::AccessFlagBits::eShaderRead;

  std::array subpass_dependencies{subpass_dependency_start, subpass_dependency_end};

  vk::RenderPassCreateInfo renderpass_info;
  renderpass_info.setAttachments(renderpass_attachments);
  renderpass_info.setSubpasses(subpass);
  renderpass_info.setDependencies(subpass_dependencies);

  auto renderpass = device->createRenderPassUnique(renderpass_info);

  vk::GraphicsPipelineCreateInfo gpi;
  gpi.layout = render_layout.get();
  gpi.setStages(stages);
  gpi.pVertexInputState = &vertex_input;
  gpi.pDynamicState = &dynamic_state;
  gpi.pColorBlendState = &color_blend_state;
  gpi.pRasterizationState = &rasterization;
  gpi.pDepthStencilState = &depth_stencil;
  gpi.pInputAssemblyState = &input_assembly;
  gpi.pMultisampleState = &multisample;
  gpi.pTessellationState = nullptr;
  gpi.pViewportState = &viewport;

  gpi.renderPass = renderpass.get();
  gpi.subpass = 0;

  auto const render_pipeline = std::move(device->createGraphicsPipelineUnique(nullptr, gpi).value);
#pragma endregion

#pragma region BuildGbufferPipeline
  vk::UniqueSampler default_post_sampler;
  vk::UniqueSampler default_shadow_sampler;
  {
    vk::SamplerCreateInfo sci;
    sci.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    sci.addressModeV = vk::SamplerAddressMode::eClampToEdge;
    sci.addressModeW = vk::SamplerAddressMode::eClampToEdge;

    sci.anisotropyEnable = true;
    sci.compareEnable = false;
    sci.minFilter = vk::Filter::eLinear;
    sci.magFilter = vk::Filter::eLinear;
    sci.maxAnisotropy = 16.0f;
    sci.maxLod = 1000.0;
    sci.minLod = -1000.0;
    sci.mipmapMode = vk::SamplerMipmapMode::eLinear;
    sci.unnormalizedCoordinates = false;
    default_post_sampler = device->createSamplerUnique(sci, nullptr, sdl);

    sci.compareOp = vk::CompareOp::eLess;
    sci.compareEnable = true;
    default_shadow_sampler = device->createSamplerUnique(sci, nullptr, sdl);
  }

  vk::UniqueDescriptorSetLayout resolve_dsl;
  vk::UniquePipelineLayout resolve_pll;
  vk::UniquePipeline resolve_pipeline;
  vk::UniqueRenderPass resolve_pass;

  std::vector<vk_image> resolve_images;
  std::vector<vk::UniqueImageView> resolve_views;

  auto const rvert_shader = myrt::compile_file(shaderc_vertex_shader, "../../../../../src/myrt.vk/resolve.vert");
  vk::ShaderModuleCreateInfo rvertex_stage_info;
  rvertex_stage_info.codeSize = sizeof(std::uint32_t) * rvert_shader.size();
  rvertex_stage_info.pCode = rvert_shader.data();
  auto rvertex_stage = device->createShaderModuleUnique(rvertex_stage_info);

  auto const rfrag_shader = myrt::compile_file(shaderc_fragment_shader, "../../../../../src/myrt.vk/resolve.frag");
  vk::ShaderModuleCreateInfo rfrag_shader_info;
  rfrag_shader_info.codeSize = sizeof(std::uint32_t) * rfrag_shader.size();
  rfrag_shader_info.pCode = rfrag_shader.data();
  auto rfragment_stage = device->createShaderModuleUnique(rfrag_shader_info);
  {
    std::array<vk::PipelineShaderStageCreateInfo, 2> stages;
    {
      stages[0].module = rvertex_stage.get();
      stages[0].stage = vk::ShaderStageFlagBits::eVertex;
      stages[0].pName = "main";
      stages[1].module = rfragment_stage.get();
      stages[1].stage = vk::ShaderStageFlagBits::eFragment;
      stages[1].pName = "main";
    }

    resolve_dsl =
        make_ds_layout(device.get(), {vk::DescriptorSetLayoutBinding(0u, vk::DescriptorType::eCombinedImageSampler,
                                          vk::ShaderStageFlagBits::eFragment, default_post_sampler.get()),
                                         vk::DescriptorSetLayoutBinding(1u, vk::DescriptorType::eCombinedImageSampler,
                                             vk::ShaderStageFlagBits::eFragment, default_post_sampler.get()),
                                         vk::DescriptorSetLayoutBinding(2u, vk::DescriptorType::eCombinedImageSampler,
                                             vk::ShaderStageFlagBits::eFragment, default_post_sampler.get()),
                                         vk::DescriptorSetLayoutBinding(3u, vk::DescriptorType::eCombinedImageSampler,
                                             vk::ShaderStageFlagBits::eFragment, default_shadow_sampler.get())});

    vk::PipelineLayoutCreateInfo ppi;
    ppi.setSetLayouts(resolve_dsl.get());

    vk::PushConstantRange rvs;
    rvs.offset = 0;
    rvs.size = 2 * sizeof(rnu::mat4);
    rvs.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
    ppi.setPushConstantRanges(rvs);

    resolve_pll = device->createPipelineLayoutUnique(ppi, nullptr, sdl);

    vk::PipelineVertexInputStateCreateInfo vi;
    vk::PipelineDepthStencilStateCreateInfo ds;

    vk::AttachmentDescription rpat0;
    rpat0.initialLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    rpat0.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    rpat0.format = vk::Format::eR16G16B16A16Sfloat;
    rpat0.loadOp = vk::AttachmentLoadOp::eDontCare;
    rpat0.storeOp = vk::AttachmentStoreOp::eStore;
    rpat0.samples = vk::SampleCountFlagBits::e1;

    vk::AttachmentReference rpat0r(0, vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription rsp;
    rsp.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    rsp.setColorAttachments(rpat0r);

    std::array<vk::SubpassDependency, 2> deps = {vk::SubpassDependency()
                                                     .setSrcAccessMask(vk::AccessFlagBits::eShaderRead)
                                                     .setSrcStageMask(vk::PipelineStageFlagBits::eFragmentShader)
                                                     .setSrcSubpass(VK_SUBPASS_EXTERNAL)
                                                     .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
                                                     .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
                                                     .setDstSubpass(0)
                                                     .setDependencyFlags(vk::DependencyFlagBits::eByRegion),
        vk::SubpassDependency()
            .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setSrcSubpass(0)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
            .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
            .setDstSubpass(VK_SUBPASS_EXTERNAL)
            .setDependencyFlags(vk::DependencyFlagBits::eByRegion)

    };

    vk::RenderPassCreateInfo rpi;
    rpi.setAttachments(rpat0);
    rpi.setSubpasses(rsp);
    rpi.setDependencies(deps);
    resolve_pass = device->createRenderPassUnique(rpi, nullptr, sdl);

    vk::PipelineColorBlendAttachmentState attstate;
    attstate.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    attstate.blendEnable = false;
    std::array blend_attachments{attstate};

    vk::PipelineColorBlendStateCreateInfo color_blend_state;
    color_blend_state.setAttachments(blend_attachments);
    color_blend_state.logicOpEnable = false;

    vk::GraphicsPipelineCreateInfo gpi;
    gpi.layout = resolve_pll.get();
    gpi.setStages(stages);
    gpi.pTessellationState = nullptr;

    gpi.pVertexInputState = &vi;
    gpi.pDynamicState = &dynamic_state;
    gpi.pColorBlendState = &color_blend_state;
    gpi.pRasterizationState = &rasterization;
    gpi.pDepthStencilState = &ds;
    gpi.pInputAssemblyState = &input_assembly;
    gpi.pMultisampleState = &multisample;
    gpi.pViewportState = &viewport;

    gpi.renderPass = resolve_pass.get();
    gpi.subpass = 0;
    resolve_pipeline = device->createGraphicsPipelineUnique(nullptr, gpi, nullptr, sdl);

    {
      vk::CommandBufferAllocateInfo cmd_info;
      cmd_info.commandBufferCount = 1;
      cmd_info.commandPool = graphics_command_pool.get();
      cmd_info.level = vk::CommandBufferLevel::ePrimary;
      auto pac = std::move(device->allocateCommandBuffersUnique(cmd_info)[0]);
      pac->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
      resolve_images.clear();
      resolve_views.clear();
      for (int i = 0; i < swapchain_data.images.size(); ++i) {
        resolve_images.push_back(
            get_image(device.get(), allocator, initial_width, initial_height, vk::Format::eR16G16B16A16Sfloat,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled));

        vk::ImageViewCreateInfo attv;
        attv.viewType = vk::ImageViewType::e2D;
        attv.components = vk::ComponentMapping();
        attv.format = vk::Format::eR16G16B16A16Sfloat;
        attv.image = resolve_images[i].image.get();
        attv.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
        resolve_views.push_back(device->createImageViewUnique(attv, nullptr, sdl));

        vk::ImageMemoryBarrier2KHR img0_transition;
        img0_transition.oldLayout = vk::ImageLayout::eUndefined;
        img0_transition.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        img0_transition.srcStageMask = vk::PipelineStageFlagBits2KHR::eNone;
        img0_transition.dstStageMask = vk::PipelineStageFlagBits2KHR::eFragmentShader;
        img0_transition.image = resolve_images[i].image.get();
        img0_transition.srcQueueFamilyIndex = queues[present_queue_index].family;
        img0_transition.dstQueueFamilyIndex = queues[graphics_queue_index].family;
        img0_transition.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        img0_transition.subresourceRange.baseMipLevel = 0;
        img0_transition.subresourceRange.levelCount = 1;
        img0_transition.subresourceRange.baseArrayLayer = 0;
        img0_transition.subresourceRange.layerCount = 1;
        img0_transition.srcAccessMask = vk::AccessFlagBits2KHR::eNone;
        img0_transition.dstAccessMask = vk::AccessFlagBits2KHR::eShaderRead;

        vk::DependencyInfoKHR dp;
        dp.dependencyFlags = vk::DependencyFlagBits::eByRegion;
        dp.setImageMemoryBarriers(img0_transition);
        pac->pipelineBarrier2KHR(dp, ext);
      }
      pac->end();

      vk::SubmitInfo submit;
      submit.setCommandBuffers(pac.get());
      graphics_queue.submit(submit);
      graphics_queue.waitIdle();
    }
  }
#pragma endregion

#pragma region fxaaPipeline
  vk::UniquePipeline fxaaPipeline;
  vk::UniqueDescriptorSetLayout fxaa_dsl;
  vk::UniquePipelineLayout fxaa_pll;
  vk::UniqueRenderPass fxaa_pass;

  std::vector<vk_image> fxaa_images;
  std::vector<vk::UniqueImageView> fxaa_views;
  {
    auto const rvert_shader = myrt::compile_file(shaderc_vertex_shader, "../../../../../src/myrt.vk/screen.vert");
    vk::ShaderModuleCreateInfo rvertex_stage_info;
    rvertex_stage_info.codeSize = sizeof(std::uint32_t) * rvert_shader.size();
    rvertex_stage_info.pCode = rvert_shader.data();
    auto rvertex_stage = device->createShaderModuleUnique(rvertex_stage_info);

    auto const rfrag_shader = myrt::compile_file(shaderc_fragment_shader, "../../../../../src/myrt.vk/fxaa.frag");
    vk::ShaderModuleCreateInfo rfrag_shader_info;
    rfrag_shader_info.codeSize = sizeof(std::uint32_t) * rfrag_shader.size();
    rfrag_shader_info.pCode = rfrag_shader.data();
    auto rfragment_stage = device->createShaderModuleUnique(rfrag_shader_info);

    std::array<vk::PipelineShaderStageCreateInfo, 2> stages;
    {
      stages[0].module = rvertex_stage.get();
      stages[0].stage = vk::ShaderStageFlagBits::eVertex;
      stages[0].pName = "main";
      stages[1].module = rfragment_stage.get();
      stages[1].stage = vk::ShaderStageFlagBits::eFragment;
      stages[1].pName = "main";
    }

    fxaa_dsl = make_ds_layout(
            device.get(), {vk::DescriptorSetLayoutBinding(0u, vk::DescriptorType::eCombinedImageSampler,
                              vk::ShaderStageFlagBits::eFragment, default_post_sampler.get())});

    vk::PipelineLayoutCreateInfo ppi;
    ppi.setSetLayouts(fxaa_dsl.get());

    fxaa_pll = device->createPipelineLayoutUnique(ppi, nullptr, sdl);

    vk::PipelineVertexInputStateCreateInfo vi;
    vk::PipelineDepthStencilStateCreateInfo ds;

    vk::AttachmentDescription rpat0;
    rpat0.initialLayout = vk::ImageLayout::eTransferSrcOptimal;
    rpat0.finalLayout = vk::ImageLayout::eTransferSrcOptimal;
    rpat0.format = vk::Format::eR16G16B16A16Sfloat;
    rpat0.loadOp = vk::AttachmentLoadOp::eDontCare;
    rpat0.storeOp = vk::AttachmentStoreOp::eStore;
    rpat0.samples = vk::SampleCountFlagBits::e1;

    vk::AttachmentReference rpat0r(0, vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription rsp;
    rsp.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    rsp.setColorAttachments(rpat0r);

    std::array<vk::SubpassDependency, 2> deps = {vk::SubpassDependency()
                                                     .setSrcAccessMask(vk::AccessFlagBits::eTransferRead)
                                                     .setSrcStageMask(vk::PipelineStageFlagBits::eTransfer)
                                                     .setSrcSubpass(VK_SUBPASS_EXTERNAL)
                                                     .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
                                                     .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
                                                     .setDstSubpass(0)
                                                     .setDependencyFlags(vk::DependencyFlagBits::eByRegion),
        vk::SubpassDependency()
            .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setSrcSubpass(0)
            .setDstAccessMask(vk::AccessFlagBits::eTransferRead)
            .setDstStageMask(vk::PipelineStageFlagBits::eTransfer)
            .setDstSubpass(VK_SUBPASS_EXTERNAL)
            .setDependencyFlags(vk::DependencyFlagBits::eByRegion)

    };

    vk::RenderPassCreateInfo rpi;
    rpi.setAttachments(rpat0);
    rpi.setSubpasses(rsp);
    rpi.setDependencies(deps);
    fxaa_pass = device->createRenderPassUnique(rpi, nullptr, sdl);

    vk::PipelineColorBlendAttachmentState attstate;
    attstate.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    attstate.blendEnable = false;
    std::array blend_attachments{attstate};

    vk::PipelineColorBlendStateCreateInfo color_blend_state;
    color_blend_state.setAttachments(blend_attachments);
    color_blend_state.logicOpEnable = false;

    vk::GraphicsPipelineCreateInfo gpi;
    gpi.layout = fxaa_pll.get();
    gpi.setStages(stages);
    gpi.pTessellationState = nullptr;

    gpi.pVertexInputState = &vi;
    gpi.pDynamicState = &dynamic_state;
    gpi.pColorBlendState = &color_blend_state;
    gpi.pRasterizationState = &rasterization;
    gpi.pDepthStencilState = &ds;
    gpi.pInputAssemblyState = &input_assembly;
    gpi.pMultisampleState = &multisample;
    gpi.pViewportState = &viewport;

    gpi.renderPass = resolve_pass.get();
    gpi.subpass = 0;
    fxaaPipeline = device->createGraphicsPipelineUnique(nullptr, gpi, nullptr, sdl);

    {
      vk::CommandBufferAllocateInfo cmd_info;
      cmd_info.commandBufferCount = 1;
      cmd_info.commandPool = graphics_command_pool.get();
      cmd_info.level = vk::CommandBufferLevel::ePrimary;
      auto pac = std::move(device->allocateCommandBuffersUnique(cmd_info)[0]);
      pac->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
      fxaa_images.clear();
      fxaa_views.clear();
      for (int i = 0; i < swapchain_data.images.size(); ++i) {
        fxaa_images.push_back(
            get_image(device.get(), allocator, initial_width, initial_height, vk::Format::eR16G16B16A16Sfloat,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc));

        vk::ImageViewCreateInfo attv;
        attv.viewType = vk::ImageViewType::e2D;
        attv.components = vk::ComponentMapping();
        attv.format = vk::Format::eR16G16B16A16Sfloat;
        attv.image = fxaa_images[i].image.get();
        attv.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
        fxaa_views.push_back(device->createImageViewUnique(attv, nullptr, sdl));

        vk::ImageMemoryBarrier2KHR img0_transition;
        img0_transition.oldLayout = vk::ImageLayout::eUndefined;
        img0_transition.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        img0_transition.srcStageMask = vk::PipelineStageFlagBits2KHR::eNone;
        img0_transition.dstStageMask = vk::PipelineStageFlagBits2KHR::eTransfer;
        img0_transition.image = fxaa_images[i].image.get();
        img0_transition.srcQueueFamilyIndex = queues[present_queue_index].family;
        img0_transition.dstQueueFamilyIndex = queues[graphics_queue_index].family;
        img0_transition.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        img0_transition.subresourceRange.baseMipLevel = 0;
        img0_transition.subresourceRange.levelCount = 1;
        img0_transition.subresourceRange.baseArrayLayer = 0;
        img0_transition.subresourceRange.layerCount = 1;
        img0_transition.srcAccessMask = vk::AccessFlagBits2KHR::eNone;
        img0_transition.dstAccessMask = vk::AccessFlagBits2KHR::eTransferRead;

        vk::DependencyInfoKHR dp;
        dp.dependencyFlags = vk::DependencyFlagBits::eByRegion;
        dp.setImageMemoryBarriers(img0_transition);
        pac->pipelineBarrier2KHR(dp, ext);
      }
      pac->end();

      vk::SubmitInfo submit;
      submit.setCommandBuffers(pac.get());
      graphics_queue.submit(submit);
      graphics_queue.waitIdle();
    }
  }

  std::vector<vk::UniqueDescriptorSet> fxaa_descriptor_sets;
  {
    vk::DescriptorSetAllocateInfo dsal;
    dsal.descriptorPool = imgui_dpool.get();
    dsal.setSetLayouts(fxaa_dsl.get());

    for (int i = 0; i < swapchain_data.images.size(); ++i) {
      fxaa_descriptor_sets.push_back(std::move(device->allocateDescriptorSetsUnique(dsal, sdl)[0]));
      vk::DescriptorImageInfo dinf;
      dinf.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
      dinf.imageView = resolve_views[i].get();

      vk::WriteDescriptorSet wds;
      wds.descriptorCount = 1;
      wds.descriptorType = vk::DescriptorType::eCombinedImageSampler;
      wds.dstSet = fxaa_descriptor_sets[i].get();
      wds.dstBinding = 0;
      wds.pImageInfo = &dinf;

      device->updateDescriptorSets({wds}, nullptr);
    }
  }

#pragma endregion

  std::vector<vk::UniqueCommandPool> frame_command_pools;

  vk::CommandPoolCreateInfo graphics_cmd_pool_info;
  graphics_cmd_pool_info.queueFamilyIndex = queues[graphics_queue_index].family;
  for (int i = 0; i < swapchain_data.images.size(); ++i) {
    frame_command_pools.push_back(device->createCommandPoolUnique(graphics_cmd_pool_info, nullptr, sdl));
  }

  std::vector<vk_image> attachment0img;
  std::vector<vk_image> attachment1img;
  std::vector<vk_image> attachment2img;
  std::vector<vk_image> attachmentdimg;
  std::vector<std::array<vk::UniqueImageView, 4>> attachment_views_unique;
  std::vector<std::array<vk::ImageView, 4>> attachment_views;
  std::vector<vk::UniqueImageView> depth_attachment_views;

  for (int i = 0; i < swapchain_data.images.size(); ++i) {
    attachment0img.push_back(get_image(device.get(), allocator, initial_width, initial_height,
        vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled));
    attachment1img.push_back(get_image(device.get(), allocator, initial_width, initial_height,
        vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled));
    attachment2img.push_back(get_image(device.get(), allocator, initial_width, initial_height,
        vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled));
    attachmentdimg.push_back(
        get_image(device.get(), allocator, initial_width, initial_height, vk::Format::eD24UnormS8Uint,
            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled));
  }

  vk::ImageViewCreateInfo img0vinfo_base;
  img0vinfo_base.components = vk::ComponentMapping{};
  img0vinfo_base.viewType = vk::ImageViewType::e2D;

  vk::DescriptorSetAllocateInfo dsal;
  dsal.descriptorPool = imgui_dpool.get();
  dsal.setSetLayouts(resolve_dsl.get());
  std::vector<vk::UniqueDescriptorSet> resolve_descriptor_sets;

  for (int i = 0; i < swapchain_data.images.size(); ++i) {
    resolve_descriptor_sets.push_back(std::move(device->allocateDescriptorSetsUnique(dsal, sdl)[0]));
    img0vinfo_base.format = vk::Format::eR16G16B16A16Sfloat;
    img0vinfo_base.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
    auto& arr = attachment_views_unique.emplace_back();
    img0vinfo_base.image = attachment0img[i].image.get();
    arr[0] = device->createImageViewUnique(img0vinfo_base, nullptr, sdl);
    img0vinfo_base.image = attachment1img[i].image.get();
    arr[1] = device->createImageViewUnique(img0vinfo_base, nullptr, sdl);
    img0vinfo_base.image = attachment2img[i].image.get();
    arr[2] = device->createImageViewUnique(img0vinfo_base, nullptr, sdl);
    img0vinfo_base.image = attachmentdimg[i].image.get();
    img0vinfo_base.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
    img0vinfo_base.format = vk::Format::eD24UnormS8Uint;
    arr[3] = device->createImageViewUnique(img0vinfo_base, nullptr, sdl);
    img0vinfo_base.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    depth_attachment_views.push_back(device->createImageViewUnique(img0vinfo_base, nullptr, sdl));

    auto& nu = attachment_views.emplace_back();
    for (int i = 0; i < size(nu); ++i) nu[i] = arr[i].get();

    vk::DescriptorImageInfo dinf;
    dinf.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    dinf.imageView = arr[0].get();

    vk::WriteDescriptorSet wds;
    wds.descriptorCount = 1;
    wds.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    wds.dstSet = resolve_descriptor_sets[i].get();
    wds.dstBinding = 0;
    wds.pImageInfo = &dinf;

    vk::DescriptorImageInfo dinf2;
    dinf2.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    dinf2.imageView = arr[1].get();
    auto wds2 = wds;
    wds2.dstBinding = 1;
    wds2.pImageInfo = &dinf2;

    vk::DescriptorImageInfo dinf3;
    dinf3.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    dinf3.imageView = arr[2].get();
    auto wds3 = wds;
    wds3.dstBinding = 2;
    wds3.pImageInfo = &dinf3;

    vk::DescriptorImageInfo dinfsh;
    dinfsh.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    dinfsh.imageView = depth_attachment_views[i].get();
    auto wdssh = wds;
    wdssh.dstBinding = 3;
    wdssh.pImageInfo = &dinfsh;

    device->updateDescriptorSets({wds, wds2, wds3, wdssh}, nullptr);
  }
  {
    vk::CommandBufferAllocateInfo cmd_info;
    cmd_info.commandBufferCount = 1;
    cmd_info.commandPool = graphics_command_pool.get();
    cmd_info.level = vk::CommandBufferLevel::ePrimary;
    auto pac = std::move(device->allocateCommandBuffersUnique(cmd_info)[0]);
    pac->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    for (int i = 0; i < swapchain_data.images.size(); ++i) {
      vk::ImageMemoryBarrier2KHR img0_transition;
      img0_transition.oldLayout = vk::ImageLayout::eUndefined;
      img0_transition.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
      img0_transition.srcStageMask = vk::PipelineStageFlagBits2KHR::eNone;
      img0_transition.dstStageMask = vk::PipelineStageFlagBits2KHR::eFragmentShader;
      img0_transition.image = attachment0img[i].image.get();
      img0_transition.srcQueueFamilyIndex = queues[present_queue_index].family;
      img0_transition.dstQueueFamilyIndex = queues[graphics_queue_index].family;
      img0_transition.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
      img0_transition.subresourceRange.baseMipLevel = 0;
      img0_transition.subresourceRange.levelCount = 1;
      img0_transition.subresourceRange.baseArrayLayer = 0;
      img0_transition.subresourceRange.layerCount = 1;
      img0_transition.srcAccessMask = vk::AccessFlagBits2KHR::eNone;
      img0_transition.dstAccessMask = vk::AccessFlagBits2KHR::eShaderRead;
      vk::ImageMemoryBarrier2KHR img1_transition = img0_transition;
      img1_transition.image = attachment1img[i].image.get();
      vk::ImageMemoryBarrier2KHR img2_transition = img0_transition;
      img2_transition.image = attachment2img[i].image.get();
      vk::ImageMemoryBarrier2KHR imgd_transition = img0_transition;
      imgd_transition.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
      imgd_transition.image = attachmentdimg[i].image.get();
      imgd_transition.dstStageMask = vk::PipelineStageFlagBits2KHR::eEarlyFragmentTests;
      imgd_transition.dstAccessMask = vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite;
      imgd_transition.newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

      vk::ImageMemoryBarrier2KHR imgsc_transition = img0_transition;
      imgsc_transition.image = swapchain_data.images[i];
      imgsc_transition.newLayout = vk::ImageLayout::ePresentSrcKHR;
      imgsc_transition.dstAccessMask = vk::AccessFlagBits2KHR::eNone;
      imgsc_transition.srcQueueFamilyIndex = queues[present_queue_index].family;
      imgsc_transition.dstQueueFamilyIndex = queues[present_queue_index].family;

      std::array imgbar{img0_transition, img1_transition, img2_transition, imgd_transition, imgsc_transition};

      vk::DependencyInfoKHR dep;
      dep.dependencyFlags = vk::DependencyFlagBits::eByRegion;
      dep.setImageMemoryBarriers(imgbar);

      pac->pipelineBarrier2KHR(dep, ext);

      ImGui_ImplVulkan_CreateFontsTexture(pac.get());
    }
    pac->end();

    vk::SubmitInfo submit;
    submit.setCommandBuffers(pac.get());
    graphics_queue.submit(submit);
    graphics_queue.waitIdle();
  }

  std::vector<vk::UniqueFence> render_fences;
  for (int i = 0; i < swapchain_data.images.size(); ++i) {
    render_fences.push_back(device->createFenceUnique({vk::FenceCreateFlagBits::eSignaled}));
  }

  auto image_available_sem = device->createSemaphoreUnique({});
  auto render_finished_sem = device->createSemaphoreUnique({});

  struct matrix_constants {
    rnu::mat4 view;
    rnu::mat4 proj;
  } matrices;
  matrices.proj =
      rnu::cameraf::projection(rnu::radians(70), float(initial_width) / float(initial_height), 0.01f, 1000.f, true);

  rnu::cameraf camera(rnu::vec3(0.0f, 0.0f, 3.f));

  auto obj = myrt::obj::load_obj("../../../../../res/bunny.obj");
  auto bunny = myrt::obj::triangulate(obj[0]);

  std::vector<rnu::mat4> tfs{};

  for (int x = -4; x <= 4; ++x) {
    for (int y = -4; y <= 4; ++y) {
      tfs.push_back(rnu::translation(rnu::vec3(1.5 * x, 0, 1.5 * y)));
    }
  }

  VkBuffer staging;
  VmaAllocation staging_alloc;

  auto const bytes = [](auto const& coll) { return sizeof(coll[0]) * std::size(coll); };
  {
    vk::BufferCreateInfo bci;
    bci.sharingMode = vk::SharingMode::eExclusive;
    bci.usage = vk::BufferUsageFlagBits::eTransferSrc;
    VmaAllocationCreateInfo alloc{};
    alloc.preferredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    bci.size = bytes(bunny[0].positions) + bytes(bunny[0].normals) + bytes(bunny[0].texcoords) +
               bytes(bunny[0].indices) + bytes(tfs);

    vmaCreateBuffer(allocator, (VkBufferCreateInfo*)&bci, &alloc, &staging, &staging_alloc, nullptr);

    std::byte* buf_mem = static_cast<std::byte*>(
        device->mapMemory(staging_alloc->GetMemory(), staging_alloc->GetOffset(), staging_alloc->GetSize()));

    memcpy(buf_mem, data(bunny[0].positions), bytes(bunny[0].positions));
    buf_mem += bytes(bunny[0].positions);
    memcpy(buf_mem, data(bunny[0].normals), bytes(bunny[0].normals));
    buf_mem += bytes(bunny[0].normals);
    memcpy(buf_mem, data(bunny[0].texcoords), bytes(bunny[0].texcoords));
    buf_mem += bytes(bunny[0].texcoords);
    memcpy(buf_mem, data(bunny[0].indices), bytes(bunny[0].indices));
    buf_mem += bytes(bunny[0].indices);
    memcpy(buf_mem, std::data(tfs), bytes(tfs));
    buf_mem += bytes(tfs);
    vk::MappedMemoryRange r(staging_alloc->GetMemory(), staging_alloc->GetOffset(), staging_alloc->GetSize());
    device->flushMappedMemoryRanges(r);
    device->unmapMemory(staging_alloc->GetMemory());
  }

  VkBuffer vbo;
  VmaAllocation vbo_alloc;
  VkBuffer nbo;
  VmaAllocation nbo_alloc;
  VkBuffer ubo;
  VmaAllocation ubo_alloc;
  VkBuffer ibo;
  VmaAllocation ibo_alloc;
  VkBuffer tbo;
  VmaAllocation tbo_alloc;

  auto cleanup_buffers = at_scope_end([&] {
    vmaDestroyBuffer(allocator, vbo, vbo_alloc);
    vmaDestroyBuffer(allocator, nbo, nbo_alloc);
    vmaDestroyBuffer(allocator, ubo, ubo_alloc);
    vmaDestroyBuffer(allocator, tbo, tbo_alloc);
    vmaDestroyBuffer(allocator, ibo, ibo_alloc);
  });

  vk::BufferCreateInfo bci;
  bci.sharingMode = vk::SharingMode::eExclusive;
  bci.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;

  VmaAllocationCreateInfo alloc{};
  alloc.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  bci.size = bytes(bunny[0].positions);
  vmaCreateBuffer(allocator, (VkBufferCreateInfo*)&bci, &alloc, &vbo, &vbo_alloc, nullptr);
  bci.size = bytes(bunny[0].normals);
  vmaCreateBuffer(allocator, (VkBufferCreateInfo*)&bci, &alloc, &nbo, &nbo_alloc, nullptr);
  bci.size = bytes(bunny[0].texcoords);
  vmaCreateBuffer(allocator, (VkBufferCreateInfo*)&bci, &alloc, &ubo, &ubo_alloc, nullptr);
  bci.size = bytes(tfs);
  vmaCreateBuffer(allocator, (VkBufferCreateInfo*)&bci, &alloc, &tbo, &tbo_alloc, nullptr);
  bci.usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
  bci.size = bytes(bunny[0].indices);
  vmaCreateBuffer(allocator, (VkBufferCreateInfo*)&bci, &alloc, &ibo, &ibo_alloc, nullptr);

  {
    vk::CommandBufferAllocateInfo alloc(transfer_pool.get(), vk::CommandBufferLevel::ePrimary, 1);
    auto c = std::move(device->allocateCommandBuffersUnique(alloc)[0]);
    c->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    {
      {
        vk::BufferMemoryBarrier2KHR targetBarrier;
        targetBarrier.srcStageMask = vk::PipelineStageFlagBits2KHR::eVertexInput;
        targetBarrier.srcQueueFamilyIndex = queues[graphics_queue_index].family;
        targetBarrier.srcAccessMask = vk::AccessFlagBits2KHR::eVertexAttributeRead;

        targetBarrier.dstStageMask = vk::PipelineStageFlagBits2KHR::eTransfer;
        targetBarrier.dstQueueFamilyIndex = queues[transfer_queue_index].family;
        targetBarrier.dstAccessMask = vk::AccessFlagBits2KHR::eTransferWrite;

        targetBarrier.size = VK_WHOLE_SIZE;
        targetBarrier.offset = 0;

        auto const copy_for_buffer = [&](vk::Buffer buffer) {
          auto copy = targetBarrier;
          copy.buffer = buffer;
          return copy;
        };
        auto vbob = copy_for_buffer(vbo);
        auto nbob = copy_for_buffer(nbo);
        auto ubob = copy_for_buffer(ubo);
        auto tbob = copy_for_buffer(tbo);

        vk::BufferMemoryBarrier2KHR indexBarrier = targetBarrier;
        indexBarrier.srcStageMask = vk::PipelineStageFlagBits2KHR::eIndexInput;
        indexBarrier.srcAccessMask = vk::AccessFlagBits2KHR::eIndexRead;
        indexBarrier.buffer = ibo;

        std::array buffer_barriers{vbob, nbob, ubob, tbob, indexBarrier};

        vk::DependencyInfoKHR dep;
        dep.setBufferMemoryBarriers(buffer_barriers);
        dep.dependencyFlags = vk::DependencyFlagBits::eByRegion;
        c->pipelineBarrier2KHR(dep, ext);
      }

      vk::BufferCopy region(0, 0, bytes(bunny[0].positions));
      c->copyBuffer(staging, vbo, region, ext);

      region.srcOffset += region.size;
      region.size = bytes(bunny[0].normals);
      c->copyBuffer(staging, nbo, region, ext);

      region.srcOffset += region.size;
      region.size = bytes(bunny[0].texcoords);
      c->copyBuffer(staging, ubo, region, ext);

      region.srcOffset += region.size;
      region.size = bytes(bunny[0].indices);
      c->copyBuffer(staging, ibo, region, ext);

      region.srcOffset += region.size;
      region.size = bytes(tfs);
      c->copyBuffer(staging, tbo, region, ext);

      {
        vk::BufferMemoryBarrier2KHR targetBarrier;
        targetBarrier.srcStageMask = vk::PipelineStageFlagBits2KHR::eTransfer;
        targetBarrier.srcQueueFamilyIndex = queues[transfer_queue_index].family;
        targetBarrier.srcAccessMask = vk::AccessFlagBits2KHR::eTransferWrite;

        targetBarrier.dstStageMask = vk::PipelineStageFlagBits2KHR::eVertexInput;
        targetBarrier.dstQueueFamilyIndex = queues[graphics_queue_index].family;
        targetBarrier.dstAccessMask = vk::AccessFlagBits2KHR::eVertexAttributeRead;

        targetBarrier.size = VK_WHOLE_SIZE;
        targetBarrier.offset = 0;

        auto const copy_for_buffer = [&](vk::Buffer buffer) {
          auto copy = targetBarrier;
          copy.buffer = buffer;
          return copy;
        };
        auto vbob = copy_for_buffer(vbo);
        auto nbob = copy_for_buffer(nbo);
        auto ubob = copy_for_buffer(ubo);
        auto tbob = copy_for_buffer(tbo);

        vk::BufferMemoryBarrier2KHR indexBarrier = targetBarrier;
        indexBarrier.dstStageMask = vk::PipelineStageFlagBits2KHR::eIndexInput;
        indexBarrier.dstAccessMask = vk::AccessFlagBits2KHR::eIndexRead;
        indexBarrier.buffer = ibo;

        std::array buffer_barriers{vbob, nbob, ubob, tbob, indexBarrier};

        vk::DependencyInfoKHR dep;
        dep.setBufferMemoryBarriers(buffer_barriers);
        dep.dependencyFlags = vk::DependencyFlagBits::eByRegion;
        c->pipelineBarrier2KHR(dep, ext);
      }
    }

    c->end();

    vk::CommandBufferSubmitInfoKHR copy_cmd(c.get());
    vk::SubmitInfo2KHR submit;
    submit.setCommandBufferInfos(copy_cmd);
    transfer_queue.submit2KHR(submit, {}, ext);
    transfer_queue.waitIdle();
  }
  vmaDestroyBuffer(allocator, staging, staging_alloc);

  auto current_time = std::chrono::high_resolution_clock::now();

  double dt_smooth = 0.0;

  std::vector<float> delta_times(500);

  bool rebuild_swapchain = false;

  std::vector<vk::UniqueCommandBuffer> command_buffers_in_flight(swapchain_data.images.size());

  while (!glfwWindowShouldClose(window.get())) {
    if (rebuild_swapchain) {
#pragma region rebuildSwapchain
      device->waitIdle();
      swapchain_data.reset();

      glfwGetWindowSize(window.get(), &initial_width, &initial_height);
      ImGui::GetIO().DisplaySize = ImVec2(initial_width, initial_height);

      swapchain_data = create_swapchain(device.get(), default_pd, surface.get(), initial_width, initial_height,
          {queues[present_queue_index].family, queues[present_queue_index].family});

      ImGui_ImplVulkan_SetMinImageCount(swapchain_data.images.size());

      attachment0img.clear();
      attachment1img.clear();
      attachment2img.clear();
      attachmentdimg.clear();
      attachment_views_unique.clear();
      attachment_views.clear();
      depth_attachment_views.clear();

      for (int i = 0; i < swapchain_data.images.size(); ++i) {
        attachment0img.push_back(
            get_image(device.get(), allocator, initial_width, initial_height, vk::Format::eR16G16B16A16Sfloat,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled));
        attachment1img.push_back(
            get_image(device.get(), allocator, initial_width, initial_height, vk::Format::eR16G16B16A16Sfloat,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled));
        attachment2img.push_back(
            get_image(device.get(), allocator, initial_width, initial_height, vk::Format::eR16G16B16A16Sfloat,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled));
        attachmentdimg.push_back(
            get_image(device.get(), allocator, initial_width, initial_height, vk::Format::eD24UnormS8Uint,
                vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled));
      }
      vk::ImageViewCreateInfo img0vinfo_base;
      img0vinfo_base.components = vk::ComponentMapping{};
      img0vinfo_base.viewType = vk::ImageViewType::e2D;

      {
        vk::CommandBufferAllocateInfo cmd_info;
        cmd_info.commandBufferCount = 1;
        cmd_info.commandPool = graphics_command_pool.get();
        cmd_info.level = vk::CommandBufferLevel::ePrimary;
        auto pac = std::move(device->allocateCommandBuffersUnique(cmd_info)[0]);
        pac->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        resolve_images.clear();
        resolve_views.clear();
        for (int i = 0; i < swapchain_data.images.size(); ++i) {
          resolve_images.push_back(
              get_image(device.get(), allocator, initial_width, initial_height, vk::Format::eR16G16B16A16Sfloat,
                  vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled));

          vk::ImageViewCreateInfo attv;
          attv.viewType = vk::ImageViewType::e2D;
          attv.components = vk::ComponentMapping();
          attv.format = vk::Format::eR16G16B16A16Sfloat;
          attv.image = resolve_images[i].image.get();
          attv.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
          resolve_views.push_back(device->createImageViewUnique(attv, nullptr, sdl));

          vk::ImageMemoryBarrier2KHR img0_transition;
          img0_transition.oldLayout = vk::ImageLayout::eUndefined;
          img0_transition.newLayout = vk::ImageLayout::eTransferSrcOptimal;
          img0_transition.srcStageMask = vk::PipelineStageFlagBits2KHR::eNone;
          img0_transition.dstStageMask = vk::PipelineStageFlagBits2KHR::eTransfer;
          img0_transition.image = resolve_images[i].image.get();
          img0_transition.srcQueueFamilyIndex = queues[present_queue_index].family;
          img0_transition.dstQueueFamilyIndex = queues[graphics_queue_index].family;
          img0_transition.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
          img0_transition.subresourceRange.baseMipLevel = 0;
          img0_transition.subresourceRange.levelCount = 1;
          img0_transition.subresourceRange.baseArrayLayer = 0;
          img0_transition.subresourceRange.layerCount = 1;
          img0_transition.srcAccessMask = vk::AccessFlagBits2KHR::eNone;
          img0_transition.dstAccessMask = vk::AccessFlagBits2KHR::eTransferRead;

          vk::DependencyInfoKHR dp;
          dp.dependencyFlags = vk::DependencyFlagBits::eByRegion;
          dp.setImageMemoryBarriers(img0_transition);
          pac->pipelineBarrier2KHR(dp, ext);
        }
        pac->end();

        vk::SubmitInfo submit;
        submit.setCommandBuffers(pac.get());
        graphics_queue.submit(submit);
        graphics_queue.waitIdle();
      }

      fxaa_descriptor_sets.clear();
      {
        vk::DescriptorSetAllocateInfo dsal;
        dsal.descriptorPool = imgui_dpool.get();
        dsal.setSetLayouts(fxaa_dsl.get());

        for (int i = 0; i < swapchain_data.images.size(); ++i) {
          fxaa_descriptor_sets.push_back(std::move(device->allocateDescriptorSetsUnique(dsal, sdl)[0]));
          vk::DescriptorImageInfo dinf;
          dinf.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
          dinf.imageView = resolve_views[i].get();

          vk::WriteDescriptorSet wds;
          wds.descriptorCount = 1;
          wds.descriptorType = vk::DescriptorType::eCombinedImageSampler;
          wds.dstSet = fxaa_descriptor_sets[i].get();
          wds.dstBinding = 0;
          wds.pImageInfo = &dinf;

          device->updateDescriptorSets({wds}, nullptr);
        }
      }

      {
        vk::CommandBufferAllocateInfo cmd_info;
        cmd_info.commandBufferCount = 1;
        cmd_info.commandPool = graphics_command_pool.get();
        cmd_info.level = vk::CommandBufferLevel::ePrimary;
        auto pac = std::move(device->allocateCommandBuffersUnique(cmd_info)[0]);
        pac->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        fxaa_images.clear();
        fxaa_views.clear();
        for (int i = 0; i < swapchain_data.images.size(); ++i) {
          fxaa_images.push_back(
              get_image(device.get(), allocator, initial_width, initial_height, vk::Format::eR16G16B16A16Sfloat,
                  vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc));

          vk::ImageViewCreateInfo attv;
          attv.viewType = vk::ImageViewType::e2D;
          attv.components = vk::ComponentMapping();
          attv.format = vk::Format::eR16G16B16A16Sfloat;
          attv.image = fxaa_images[i].image.get();
          attv.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
          fxaa_views.push_back(device->createImageViewUnique(attv, nullptr, sdl));

          vk::ImageMemoryBarrier2KHR img0_transition;
          img0_transition.oldLayout = vk::ImageLayout::eUndefined;
          img0_transition.newLayout = vk::ImageLayout::eTransferSrcOptimal;
          img0_transition.srcStageMask = vk::PipelineStageFlagBits2KHR::eNone;
          img0_transition.dstStageMask = vk::PipelineStageFlagBits2KHR::eTransfer;
          img0_transition.image = resolve_images[i].image.get();
          img0_transition.srcQueueFamilyIndex = queues[present_queue_index].family;
          img0_transition.dstQueueFamilyIndex = queues[graphics_queue_index].family;
          img0_transition.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
          img0_transition.subresourceRange.baseMipLevel = 0;
          img0_transition.subresourceRange.levelCount = 1;
          img0_transition.subresourceRange.baseArrayLayer = 0;
          img0_transition.subresourceRange.layerCount = 1;
          img0_transition.srcAccessMask = vk::AccessFlagBits2KHR::eNone;
          img0_transition.dstAccessMask = vk::AccessFlagBits2KHR::eTransferRead;

          vk::DependencyInfoKHR dp;
          dp.dependencyFlags = vk::DependencyFlagBits::eByRegion;
          dp.setImageMemoryBarriers(img0_transition);
          pac->pipelineBarrier2KHR(dp, ext);
        }
        pac->end();

        vk::SubmitInfo submit;
        submit.setCommandBuffers(pac.get());
        graphics_queue.submit(submit);
        graphics_queue.waitIdle();
      }

      for (int i = 0; i < swapchain_data.images.size(); ++i) {
        img0vinfo_base.format = vk::Format::eR16G16B16A16Sfloat;
        img0vinfo_base.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
        auto& arr = attachment_views_unique.emplace_back();
        img0vinfo_base.image = attachment0img[i].image.get();
        arr[0] = device->createImageViewUnique(img0vinfo_base);
        img0vinfo_base.image = attachment1img[i].image.get();
        arr[1] = device->createImageViewUnique(img0vinfo_base);
        img0vinfo_base.image = attachment2img[i].image.get();
        arr[2] = device->createImageViewUnique(img0vinfo_base, nullptr, sdl);
        img0vinfo_base.image = attachmentdimg[i].image.get();
        img0vinfo_base.subresourceRange.aspectMask =
            vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
        img0vinfo_base.format = vk::Format::eD24UnormS8Uint;
        arr[3] = device->createImageViewUnique(img0vinfo_base, nullptr, sdl);
        img0vinfo_base.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        depth_attachment_views.push_back(device->createImageViewUnique(img0vinfo_base, nullptr, sdl));

        auto& nu = attachment_views.emplace_back();
        for (int i = 0; i < size(nu); ++i) nu[i] = arr[i].get();

        vk::DescriptorImageInfo dinf;
        dinf.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        dinf.imageView = attachment_views[i][0];

        vk::WriteDescriptorSet wds;
        wds.descriptorCount = 1;
        wds.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        wds.dstSet = resolve_descriptor_sets[i].get();
        wds.dstBinding = 0;
        wds.pImageInfo = &dinf;

        vk::DescriptorImageInfo dinf2;
        dinf2.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        dinf2.imageView = attachment_views[i][1];
        auto wds2 = wds;
        wds2.dstBinding = 1;
        wds2.pImageInfo = &dinf2;

        vk::DescriptorImageInfo dinf3;
        dinf3.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        dinf3.imageView = arr[2].get();
        auto wds3 = wds;
        wds3.dstBinding = 2;
        wds3.pImageInfo = &dinf3;

        vk::DescriptorImageInfo dinfsh;
        dinfsh.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        dinfsh.imageView = depth_attachment_views[i].get();
        auto wdssh = wds;
        wdssh.dstBinding = 3;
        wdssh.pImageInfo = &dinfsh;

        device->updateDescriptorSets({wds, wds2, wds3, wdssh}, nullptr);
      }
      {
        vk::CommandBufferAllocateInfo cmd_info;
        cmd_info.commandBufferCount = 1;
        cmd_info.commandPool = graphics_command_pool.get();
        cmd_info.level = vk::CommandBufferLevel::ePrimary;
        auto pac = std::move(device->allocateCommandBuffersUnique(cmd_info)[0]);
        pac->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        for (int i = 0; i < swapchain_data.images.size(); ++i) {
          vk::ImageMemoryBarrier2KHR img0_transition;
          img0_transition.oldLayout = vk::ImageLayout::eUndefined;
          img0_transition.srcQueueFamilyIndex = queues[present_queue_index].family;
          img0_transition.srcStageMask = vk::PipelineStageFlagBits2KHR::eNone;
          img0_transition.srcAccessMask = vk::AccessFlagBits2KHR::eNone;

          img0_transition.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
          img0_transition.dstQueueFamilyIndex = queues[graphics_queue_index].family;
          img0_transition.dstStageMask = vk::PipelineStageFlagBits2KHR::eFragmentShader;
          img0_transition.dstAccessMask = vk::AccessFlagBits2KHR::eShaderRead;

          img0_transition.image = attachment0img[i].image.get();
          img0_transition.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

          vk::ImageMemoryBarrier2KHR img1_transition = img0_transition;
          img1_transition.image = attachment1img[i].image.get();
          vk::ImageMemoryBarrier2KHR img2_transition = img0_transition;
          img2_transition.image = attachment2img[i].image.get();
          vk::ImageMemoryBarrier2KHR imgd_transition = img0_transition;
          imgd_transition.subresourceRange.aspectMask =
              vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
          imgd_transition.image = attachmentdimg[i].image.get();
          imgd_transition.dstStageMask = vk::PipelineStageFlagBits2KHR::eEarlyFragmentTests;
          imgd_transition.dstAccessMask = vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite;
          imgd_transition.newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

          vk::ImageMemoryBarrier2KHR imgsc_transition = img0_transition;
          imgsc_transition.image = swapchain_data.images[i];
          imgsc_transition.newLayout = vk::ImageLayout::ePresentSrcKHR;
          imgsc_transition.dstAccessMask = vk::AccessFlagBits2KHR::eNone;
          imgsc_transition.srcQueueFamilyIndex = queues[present_queue_index].family;
          imgsc_transition.dstQueueFamilyIndex = queues[present_queue_index].family;

          std::array imgbar{img0_transition, img1_transition, img2_transition, imgd_transition, imgsc_transition};

          vk::DependencyInfoKHR dep;
          dep.dependencyFlags = vk::DependencyFlagBits::eByRegion;
          dep.setImageMemoryBarriers(imgbar);

          pac->pipelineBarrier2KHR(dep, ext);

          ImGui_ImplVulkan_CreateFontsTexture(pac.get());
        }
        pac->end();

        vk::SubmitInfo submit;
        submit.setCommandBuffers(pac.get());
        graphics_queue.submit(submit);
        graphics_queue.waitIdle();
      }
      rebuild_swapchain = false;

#pragma endregion
    }

    get_framebuffer.new_frame();
    ImGui::NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplVulkan_NewFrame();

    auto const delta_time = std::chrono::duration_cast<std::chrono::duration<double>>(
        std::chrono::high_resolution_clock::now() - current_time);
    current_time = std::chrono::high_resolution_clock::now();

    auto const [result, current_image] = device->acquireNextImageKHR(swapchain_data.swapchain.get(),
        std::numeric_limits<unsigned long long>::max(), image_available_sem.get(), nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR) {
      rebuild_swapchain = true;
      continue;
    } else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
      throw std::runtime_error("failed to acquire swap chain image!");
    }

    device->waitForFences(render_fences[current_image].get(), true, std::numeric_limits<uint64_t>::max());
    device->resetFences(render_fences[current_image].get());
    if (!command_buffers_in_flight[current_image]) {
      vk::CommandBufferAllocateInfo render_cmd_info;
      render_cmd_info.commandBufferCount = 1;
      render_cmd_info.commandPool = frame_command_pools[current_image].get();
      render_cmd_info.level = vk::CommandBufferLevel::ePrimary;
      command_buffers_in_flight[current_image] = std::move(device->allocateCommandBuffersUnique(render_cmd_info)[0]);
    } else {
      device->resetCommandPool(frame_command_pools[current_image].get(), {});
    }
    auto& c = command_buffers_in_flight[current_image];

    c->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    {
      std::array clear_values{vk::ClearValue(vk::ClearColorValue(std::array{0, 0, 0, 0})),
          vk::ClearValue(vk::ClearColorValue(std::array{0, 0, 0, 0})),
          vk::ClearValue(vk::ClearColorValue(std::array{0, 0, 0, 0})),
          vk::ClearValue(vk::ClearDepthStencilValue(1.f, 0))};

      vk::RenderPassBeginInfo rpbegin;
      rpbegin.renderPass = renderpass.get();
      rpbegin.renderArea = vk::Rect2D({0, 0}, {uint32_t(initial_width), uint32_t(initial_height)});
      rpbegin.setClearValues(clear_values);

      auto const main_fb =
          get_framebuffer(initial_width, initial_height, rpbegin.renderPass, attachment_views[current_image]);
      rpbegin.framebuffer = main_fb;
      c->beginRenderPass(rpbegin, vk::SubpassContents::eInline);
      c->bindPipeline(vk::PipelineBindPoint::eGraphics, render_pipeline.get());
      c->setViewport(0, vk::Viewport(0, 0, initial_width, initial_height, 0, 1));
      c->setScissor(0, vk::Rect2D({0, 0}, {uint32_t(initial_width), uint32_t(initial_height)}));

      if (glfwGetWindowAttrib(window.get(), GLFW_FOCUSED)) {
        if (!ImGui::GetIO().WantCaptureKeyboard) {
          camera.axis(float(delta_time.count()) * (1.f + 5 * glfwGetKey(window.get(), GLFW_KEY_LEFT_SHIFT)),
              glfwGetKey(window.get(), GLFW_KEY_W), glfwGetKey(window.get(), GLFW_KEY_S),
              glfwGetKey(window.get(), GLFW_KEY_A), glfwGetKey(window.get(), GLFW_KEY_D),
              glfwGetKey(window.get(), GLFW_KEY_E), glfwGetKey(window.get(), GLFW_KEY_Q));
        }

        if (!ImGui::GetIO().WantCaptureMouse) {
          double xpos, ypos;
          glfwGetCursorPos(window.get(), &xpos, &ypos);

          camera.mouse(
              float(xpos), float(ypos), glfwGetMouseButton(window.get(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
        }
      }

      matrices.proj =
          rnu::cameraf::projection(rnu::radians(70), float(initial_width) / float(initial_height), 0.01f, 1000.f, true);
      matrices.view = rnu::mat4(1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1) * camera.matrix(false);

      static int avg = 20;
      double factor = 1.0 / avg;
      dt_smooth = ((1.0 - factor) * dt_smooth) + (factor * delta_time.count());

      std::rotate(begin(delta_times), std::next(begin(delta_times)), end(delta_times));
      delta_times.back() = delta_time.count();

      if (ImGui::Begin("Test")) {
        ImGui::DragInt("Average frames", &avg, 0.01f, 1, 1000, "N = %d");
        ImGui::LabelText("Framerate (avg. over N)", "%.3f", 1.0 / dt_smooth);
        ImGui::LabelText("Frametime (avg. over N)", "%.3f ms", 1000 * dt_smooth);

        ImGui::PlotHistogram(
            "Frametimes", delta_times.data(), delta_times.size(), 0, nullptr, 0.f, 0.016f, ImVec2(0, 120));

        ImGui::Separator();
        ImGui::End();
      }

      c->pushConstants(render_layout.get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(matrices), &matrices);

      c->bindVertexBuffers(0, {vbo, nbo, ubo, tbo}, {0ull, 0ull, 0ull, 0ull});
      c->bindIndexBuffer(ibo, 0, vk::IndexType::eUint32);
      c->drawIndexed(std::size(bunny[0].indices), std::size(tfs), 0, 0, 0);

      c->endRenderPass();

      {
        vk::RenderPassBeginInfo rpbegin2;
        rpbegin2.renderPass = resolve_pass.get();
        rpbegin2.renderArea = vk::Rect2D({0, 0}, {uint32_t(initial_width), uint32_t(initial_height)});
        auto const main_fb2 =
            get_framebuffer(initial_width, initial_height, rpbegin2.renderPass, resolve_views[current_image].get());
        rpbegin2.framebuffer = main_fb2;
        c->beginRenderPass(rpbegin2, vk::SubpassContents::eInline);
        c->bindPipeline(vk::PipelineBindPoint::eGraphics, resolve_pipeline.get());
        c->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, resolve_pll.get(), 0,
            resolve_descriptor_sets[current_image].get(), nullptr);

        c->pushConstants(resolve_pll.get(), vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex, 0,
            sizeof(matrices), &matrices);

        c->setViewport(0, vk::Viewport(0, 0, initial_width, initial_height, 0, 1));
        c->setScissor(0, vk::Rect2D({0, 0}, {uint32_t(initial_width), uint32_t(initial_height)}));
        c->draw(3, 1, 0, 0);
        c->endRenderPass();
      }

      {
        vk::RenderPassBeginInfo rpbegin2;
        rpbegin2.renderPass = fxaa_pass.get();
        rpbegin2.renderArea = vk::Rect2D({0, 0}, {uint32_t(initial_width), uint32_t(initial_height)});
        auto const main_fb2 =
            get_framebuffer(initial_width, initial_height, rpbegin2.renderPass, fxaa_views[current_image].get());
        rpbegin2.framebuffer = main_fb2;
        c->beginRenderPass(rpbegin2, vk::SubpassContents::eInline);
        c->bindPipeline(vk::PipelineBindPoint::eGraphics, fxaaPipeline.get());
        c->bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, fxaa_pll.get(), 0, fxaa_descriptor_sets[current_image].get(), nullptr);
        c->setViewport(0, vk::Viewport(0, 0, initial_width, initial_height, 0, 1));
        c->setScissor(0, vk::Rect2D({0, 0}, {uint32_t(initial_width), uint32_t(initial_height)}));
        c->draw(3, 1, 0, 0);
        c->endRenderPass();
      }

      // Set a Barrier
      // to blitting
      {
        vk::ImageMemoryBarrier2KHR istr;
        istr.image = swapchain_data.images[current_image];
        istr.oldLayout = vk::ImageLayout::ePresentSrcKHR;
        istr.srcAccessMask = vk::AccessFlagBits2KHR::eColorAttachmentRead;
        istr.srcQueueFamilyIndex = queues[present_queue_index].family;
        istr.srcStageMask = vk::PipelineStageFlagBits2KHR::eColorAttachmentOutput;

        istr.newLayout = vk::ImageLayout::eTransferDstOptimal;
        istr.dstAccessMask = vk::AccessFlagBits2KHR::eTransferWrite;
        istr.dstQueueFamilyIndex = queues[graphics_queue_index].family;
        istr.dstStageMask = vk::PipelineStageFlagBits2KHR::eTransfer;

        istr.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

        std::array transitions{istr};
        vk::DependencyInfoKHR dep;
        dep.setImageMemoryBarriers(transitions);
        dep.dependencyFlags = vk::DependencyFlagBits::eByRegion;
        c->pipelineBarrier2KHR(dep, ext);
      }

      vk::ImageBlit blit_region;
      blit_region.srcSubresource = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
      blit_region.setSrcOffsets({vk::Offset3D(0, 0, 0), vk::Offset3D(initial_width, initial_height, 1)});
      blit_region.dstSubresource = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
      blit_region.setDstOffsets({vk::Offset3D(0, 0, 0), vk::Offset3D(initial_width, initial_height, 1)});

      c->blitImage(fxaa_images[current_image].image.get(), vk::ImageLayout::eTransferSrcOptimal,
          swapchain_data.images[current_image], vk::ImageLayout::eTransferDstOptimal, blit_region,
          vk::Filter::eNearest);

      // now transition back to the default layouts
      // BARRIER IS DONE BY IMGUI PASS
      {
        /*vk::ImageMemoryBarrier2KHR i0tr;
        i0tr.image = resolve_images[current_image].image.get();

        i0tr.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        i0tr.srcAccessMask = vk::AccessFlagBits2KHR::eTransferRead;
        i0tr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        i0tr.srcStageMask = vk::PipelineStageFlagBits2KHR::eTransfer;

        i0tr.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
        i0tr.dstAccessMask = vk::AccessFlagBits2KHR::eColorAttachmentWrite;
        i0tr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        i0tr.dstStageMask = vk::PipelineStageFlagBits2KHR::eColorAttachmentOutput;

        i0tr.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

        std::array transitions{i0tr};
        vk::DependencyInfoKHR dep;
        dep.setImageMemoryBarriers(transitions);
        dep.dependencyFlags = vk::DependencyFlagBits::eByRegion;
        c->pipelineBarrier2KHR(dep, ext);*/

        vk::ImageMemoryBarrier2KHR imgd_transition;
        imgd_transition.image = attachmentdimg[current_image].image.get();
        imgd_transition.oldLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imgd_transition.srcAccessMask = vk::AccessFlagBits2KHR::eShaderRead;
        imgd_transition.srcQueueFamilyIndex = queues[graphics_queue_index].family;
        imgd_transition.srcStageMask = vk::PipelineStageFlagBits2KHR::eFragmentShader;

        imgd_transition.newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        imgd_transition.dstAccessMask =
            vk::AccessFlagBits2KHR::eDepthStencilAttachmentRead | vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite;
        imgd_transition.dstQueueFamilyIndex = queues[graphics_queue_index].family;
        imgd_transition.dstStageMask = vk::PipelineStageFlagBits2KHR::eEarlyFragmentTests;

        imgd_transition.subresourceRange =
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil, 0, 1, 0, 1);

        vk::ImageMemoryBarrier2KHR istr;
        istr.image = swapchain_data.images[current_image];
        istr.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        istr.srcAccessMask = vk::AccessFlagBits2KHR::eTransferWrite;
        istr.srcQueueFamilyIndex = queues[graphics_queue_index].family;
        istr.srcStageMask = vk::PipelineStageFlagBits2KHR::eTransfer;

        istr.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
        istr.dstAccessMask =
            vk::AccessFlagBits2KHR::eColorAttachmentRead | vk::AccessFlagBits2KHR::eColorAttachmentWrite;
        istr.dstQueueFamilyIndex = queues[graphics_queue_index].family;
        istr.dstStageMask = vk::PipelineStageFlagBits2KHR::eColorAttachmentOutput;

        istr.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

        std::array transitions{istr, imgd_transition};
        vk::DependencyInfoKHR dep;
        dep.setImageMemoryBarriers(transitions);
        dep.dependencyFlags = vk::DependencyFlagBits::eByRegion;
        c->pipelineBarrier2KHR(dep, ext);
      }

      vk::RenderPassBeginInfo imgui_rp;
      imgui_rp.renderPass = imgui_renderpass.get();
      imgui_rp.renderArea = vk::Rect2D({0, 0}, {uint32_t(initial_width), uint32_t(initial_height)});
      imgui_rp.framebuffer = get_framebuffer(
          initial_width, initial_height, imgui_rp.renderPass, swapchain_data.views[current_image].get());
      c->beginRenderPass(imgui_rp, vk::SubpassContents::eInline);
      ImGui::Render();
      ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *c);
      c->endRenderPass();
    }
    c->end();

    vk::SubmitInfo submit;
    submit.setWaitSemaphores(image_available_sem.get());
    vk::PipelineStageFlags flags[] = {
        vk::PipelineStageFlagBits::eBottomOfPipe,
    };
    submit.pWaitDstStageMask = flags;
    submit.pCommandBuffers = &*c;
    submit.commandBufferCount = 1;
    submit.setSignalSemaphores(render_finished_sem.get());
    graphics_queue.submit(submit, render_fences[current_image].get());

    vk::PresentInfoKHR present;
    present.pImageIndices = &current_image;
    present.setWaitSemaphores(render_finished_sem.get());
    present.setSwapchains(swapchain_data.swapchain.get());
    auto const present_result = present_queue.presentKHR(&present);

    if (present_result == vk::Result::eErrorOutOfDateKHR || present_result == vk::Result::eSuboptimalKHR) {
      rebuild_swapchain = true;
      continue;
    } else if (present_result != vk::Result::eSuccess) {
      throw std::runtime_error("failed to present swap chain image!");
    }

    glfwPollEvents();
  }

  device->waitIdle();
  ImGui_ImplGlfw_Shutdown();
  ImGui_ImplVulkan_Shutdown();
}