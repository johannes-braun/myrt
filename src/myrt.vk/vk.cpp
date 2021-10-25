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

#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "../obj.hpp"

#include <GLFW/glfw3.h>

template<typename Fun>
struct call_at_scope_end {
  call_at_scope_end(Fun&& f) : _f(std::forward<Fun>(f)){}
  ~call_at_scope_end(){ _f(); }
  
private:
  Fun _f;
};

template<typename Fun>
auto at_scope_end(Fun&& f) {
  return call_at_scope_end<Fun>(std::forward<Fun>(f));
}

constexpr bool debug_layers = false;
constexpr int initial_width = 1920;
constexpr int initial_height = 1200;

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
  auto img = device.createImageUnique(img1);

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

  auto found_device_extensions = filter_extensions(desired_device_extensions, device_extensions);
  std::unordered_set<std::string_view> desired_device_layers;
  if constexpr (debug_layers) {
    desired_device_layers.emplace("VK_LAYER_KHRONOS_validation");
  }
  auto found_device_layers = filter_layers(desired_device_layers, device_layers);

  vk::PhysicalDeviceSynchronization2FeaturesKHR sync2;
  sync2.synchronization2 = true;

  vk::DeviceCreateInfo device_info;
  device_info.ppEnabledExtensionNames = found_device_extensions.data();
  device_info.enabledExtensionCount = found_device_extensions.size();
  device_info.ppEnabledLayerNames = found_device_layers.data();
  device_info.enabledLayerCount = found_device_layers.size();
  device_info.pNext = &sync2; // TODO: make more explicit...
  device_info.queueCreateInfoCount = queue_infos.size();
  device_info.pQueueCreateInfos = queue_infos.data();

  auto device = default_pd.createDeviceUnique(device_info);
  vk::DispatchLoaderDynamic ext(vk_instance.get(), vkGetInstanceProcAddr, device.get(), vkGetDeviceProcAddr);

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

#pragma region CreateSwapchain
  auto const& surface_caps = default_pd.getSurfaceCapabilitiesKHR(surface.get());
  auto const& formats = default_pd.getSurfaceFormatsKHR(surface.get());
  auto const& presentModes = default_pd.getSurfacePresentModesKHR(surface.get());

  auto format = formats[1];
  auto present_mode = vk::PresentModeKHR::eMailbox;
  vk::SwapchainCreateInfoKHR swapchain_info;
  swapchain_info.surface = surface.get();
  swapchain_info.imageColorSpace = format.colorSpace;
  swapchain_info.imageArrayLayers = 1;
  swapchain_info.imageExtent = vk::Extent2D(initial_width, initial_height);
  swapchain_info.imageFormat = format.format;
  swapchain_info.imageSharingMode = vk::SharingMode::eExclusive;
  swapchain_info.clipped = true;
  swapchain_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  swapchain_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
  swapchain_info.minImageCount = std::clamp<std::uint32_t>(12, surface_caps.minImageCount, surface_caps.maxImageCount);

  std::uint32_t swapchain_queues[] = {queues[present_queue_index].family, queues[present_queue_index].family};

  if (swapchain_queues[0] != swapchain_queues[1]) {
    swapchain_info.imageSharingMode = vk::SharingMode::eConcurrent;
    swapchain_info.queueFamilyIndexCount = std::size(swapchain_queues);
    swapchain_info.pQueueFamilyIndices = swapchain_queues;
  }
  swapchain_info.presentMode = present_mode;
  swapchain_info.preTransform = surface_caps.currentTransform;
  auto swapchain = device->createSwapchainKHRUnique(swapchain_info);
  auto swapchain_images = device->getSwapchainImagesKHR(swapchain.get());

  std::vector<vk::UniqueImageView> swapchain_views;
  for (auto const& image : swapchain_images) {
    vk::ImageViewCreateInfo info;
    info.image = image;
    info.components = vk::ComponentMapping();
    info.format = format.format;
    info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.viewType = vk::ImageViewType::e2D;
    swapchain_views.push_back(device->createImageViewUnique(info));
  }
#pragma endregion

  vk::CommandPoolCreateInfo graphics_command_pool_info;
  graphics_command_pool_info.queueFamilyIndex = queues[graphics_queue_index].family;
  graphics_command_pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
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

  auto cleanup_allocator = at_scope_end([&] {
    vmaDestroyAllocator(allocator);
    });

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
  std::array blend_attachments{attstate, attstate};

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
  attachment0.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
  attachment0.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
  attachment0.format = vk::Format::eR16G16B16A16Sfloat;
  attachment0.loadOp = vk::AttachmentLoadOp::eClear;
  attachment0.samples = vk::SampleCountFlagBits::e1;
  attachment0.storeOp = vk::AttachmentStoreOp::eStore;
  vk::AttachmentDescription attachment1 = attachment0;
  attachment1.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

  vk::AttachmentDescription depth_attachment = attachment0;
  depth_attachment.format = vk::Format::eD24UnormS8Uint;
  depth_attachment.storeOp = vk::AttachmentStoreOp::eDontCare;
  depth_attachment.initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
  depth_attachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

  std::array renderpass_attachments = {attachment0, attachment1, depth_attachment};

  vk::AttachmentReference attachment0ref;
  attachment0ref.attachment = 0;
  attachment0ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
  vk::AttachmentReference attachment1ref;
  attachment1ref.attachment = 1;
  attachment1ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
  vk::AttachmentReference depthref;
  depthref.attachment = 2;
  depthref.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
  std::array subpass_attachments = {attachment0ref, attachment1ref};

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
  subpass_dependency_end.dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
  subpass_dependency_end.dstAccessMask = vk::AccessFlagBits::eNoneKHR;

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

  std::vector<vk_image> attachment0img;
  for (int i = 0; i < swapchain_images.size(); ++i)
    attachment0img.push_back(get_image(device.get(), allocator, initial_width, initial_height, vk::Format::eR16G16B16A16Sfloat,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
            vk::ImageUsageFlagBits::eTransferSrc));

  std::vector<vk_image> attachment1img;
  for (int i = 0; i < swapchain_images.size(); ++i)
    attachment1img.push_back(get_image(device.get(), allocator, initial_width, initial_height, vk::Format::eR16G16B16A16Sfloat,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled));

  std::vector<vk_image> attachmentdimg;
  for (int i = 0; i < swapchain_images.size(); ++i)
    attachmentdimg.push_back(get_image(device.get(), allocator, initial_width, initial_height, vk::Format::eD24UnormS8Uint,
        vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled));

  std::vector<std::array<vk::UniqueImageView, 3>> attachment_views_unique;
  std::vector<std::array<vk::ImageView, 3>> attachment_views;

  vk::ImageViewCreateInfo img0vinfo_base;
  img0vinfo_base.components = vk::ComponentMapping{};
  img0vinfo_base.viewType = vk::ImageViewType::e2D;
  for (int i = 0; i < swapchain_images.size(); ++i) {
    img0vinfo_base.format = vk::Format::eR16G16B16A16Sfloat;
    img0vinfo_base.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
    auto& arr = attachment_views_unique.emplace_back();
    img0vinfo_base.image = attachment0img[i].image.get();
    arr[0] = device->createImageViewUnique(img0vinfo_base);
    img0vinfo_base.image = attachment1img[i].image.get();
    arr[1] = device->createImageViewUnique(img0vinfo_base);
    img0vinfo_base.image = attachmentdimg[i].image.get();
    img0vinfo_base.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
    img0vinfo_base.format = vk::Format::eD24UnormS8Uint;
    arr[2] = device->createImageViewUnique(img0vinfo_base);

    auto& nu = attachment_views.emplace_back();
    for (int i = 0; i < size(nu); ++i) nu[i] = arr[i].get();
  }

  vk::FramebufferCreateInfo fb_info;
  fb_info.width = initial_width;
  fb_info.height = initial_height;
  fb_info.renderPass = renderpass.get();
  fb_info.layers = 1;

  std::vector<vk::UniqueFramebuffer> framebuffers;
  for (int i = 0; i < swapchain_images.size(); ++i) {
    fb_info.setAttachments(attachment_views[i]);
    framebuffers.push_back(device->createFramebufferUnique(fb_info));
  }

  {
    vk::CommandBufferAllocateInfo cmd_info;
    cmd_info.commandBufferCount = 1;
    cmd_info.commandPool = graphics_command_pool.get();
    cmd_info.level = vk::CommandBufferLevel::ePrimary;
    auto pac = std::move(device->allocateCommandBuffersUnique(cmd_info)[0]);
    pac->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    for (int i = 0; i < swapchain_images.size(); ++i) {
      vk::ImageMemoryBarrier2KHR img0_transition;
      img0_transition.oldLayout = vk::ImageLayout::eUndefined;
      img0_transition.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
      img0_transition.srcStageMask = vk::PipelineStageFlagBits2KHR::eNone;
      img0_transition.dstStageMask = vk::PipelineStageFlagBits2KHR::eColorAttachmentOutput;
      img0_transition.image = attachment0img[i].image.get();
      img0_transition.srcQueueFamilyIndex = queues[present_queue_index].family;
      img0_transition.dstQueueFamilyIndex = queues[graphics_queue_index].family;
      img0_transition.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
      img0_transition.subresourceRange.baseMipLevel = 0;
      img0_transition.subresourceRange.levelCount = 1;
      img0_transition.subresourceRange.baseArrayLayer = 0;
      img0_transition.subresourceRange.layerCount = 1;
      img0_transition.srcAccessMask = vk::AccessFlagBits2KHR::eNone;
      img0_transition.dstAccessMask = vk::AccessFlagBits2KHR::eColorAttachmentWrite;
      vk::ImageMemoryBarrier2KHR img1_transition = img0_transition;
      img1_transition.image = attachment1img[i].image.get();
      vk::ImageMemoryBarrier2KHR imgd_transition = img0_transition;
      imgd_transition.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
      imgd_transition.image = attachmentdimg[i].image.get();
      imgd_transition.dstStageMask = vk::PipelineStageFlagBits2KHR::eEarlyFragmentTests;
      imgd_transition.dstAccessMask = vk::AccessFlagBits2KHR::eDepthStencilAttachmentWrite;
      imgd_transition.newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

      vk::ImageMemoryBarrier2KHR imgsc_transition = img0_transition;
      imgsc_transition.image = swapchain_images[i];
      imgsc_transition.newLayout = vk::ImageLayout::ePresentSrcKHR;
      imgsc_transition.dstAccessMask = vk::AccessFlagBits2KHR::eNone;
      imgsc_transition.srcQueueFamilyIndex = queues[present_queue_index].family;
      imgsc_transition.dstQueueFamilyIndex = queues[present_queue_index].family;

      std::array imgbar{img0_transition, img1_transition, imgd_transition, imgsc_transition};

      vk::DependencyInfoKHR dep;
      dep.dependencyFlags = vk::DependencyFlagBits::eByRegion;
      dep.setImageMemoryBarriers(imgbar);

      pac->pipelineBarrier2KHR(dep, ext);
    }
    pac->end();

    vk::SubmitInfo submit;
    submit.setCommandBuffers(pac.get());
    graphics_queue.submit(submit);
    graphics_queue.waitIdle();
  }

  vk::CommandBufferAllocateInfo render_cmd_info;
  render_cmd_info.commandBufferCount = swapchain_images.size();
  render_cmd_info.commandPool = graphics_command_pool.get();
  render_cmd_info.level = vk::CommandBufferLevel::ePrimary;
  auto const render_cmds = device->allocateCommandBuffersUnique(render_cmd_info);

  std::vector<vk::UniqueFence> render_fences;
  for (int i = 0; i < swapchain_images.size(); ++i) {
    render_fences.push_back(device->createFenceUnique({vk::FenceCreateFlagBits::eSignaled}));
  }

  auto image_available_sem = device->createSemaphoreUnique({});
  auto render_finished_sem = device->createSemaphoreUnique({});

  struct matrix_constants {
    rnu::mat4 view;
    rnu::mat4 proj;
  } matrices;
  matrices.proj = rnu::cameraf::projection(rnu::radians(70), float(initial_width) / float(initial_height), 0.01f, 1000.f, true);

  rnu::cameraf camera(rnu::vec3(0.0f, 0.0f, 3.f));

  auto obj = myrt::obj::load_obj("../../../../../res/bunny.obj");
  auto bunny = myrt::obj::triangulate(obj[0]);

  rnu::mat4 tfs[]{
      rnu::translation(rnu::vec3(-2.4, 0, 0)),
      rnu::translation(rnu::vec3(2.4, 0, 0)),
      rnu::translation(rnu::vec3(0, 0, 0)),
  };

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

  while (!glfwWindowShouldClose(window.get())) {
    auto const delta_time = std::chrono::duration_cast<std::chrono::duration<double>>(
        std::chrono::high_resolution_clock::now() - current_time);
    current_time = std::chrono::high_resolution_clock::now();

    uint32_t const current_image = device->acquireNextImageKHR(
        swapchain.get(), std::numeric_limits<unsigned long long>::max(), image_available_sem.get(), nullptr);

    device->waitForFences(render_fences[current_image].get(), true, std::numeric_limits<uint64_t>::max());
    device->resetFences(render_fences[current_image].get());

    auto& c = render_cmds[current_image];
    c->reset();
    c->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
    {
      std::array clear_values{vk::ClearValue(vk::ClearColorValue(std::array{0.2f, 0.2f, 0.2f, 1.f})),
          vk::ClearValue(vk::ClearColorValue(std::array{0.2f, 0.2f, 0.2f, 1.f})),
          vk::ClearValue(vk::ClearDepthStencilValue(1.f, 0))};

      vk::RenderPassBeginInfo rpbegin;
      rpbegin.renderPass = renderpass.get();
      rpbegin.renderArea = vk::Rect2D({0, 0}, {initial_width, initial_height});
      rpbegin.setClearValues(clear_values);
      rpbegin.framebuffer = framebuffers[current_image].get();
      c->bindPipeline(vk::PipelineBindPoint::eGraphics, render_pipeline.get());
      c->setViewport(0, vk::Viewport(0, 0, initial_width, initial_height, 0, 1));
      c->setScissor(0, vk::Rect2D({0, 0}, {initial_width, initial_height}));
      c->beginRenderPass(rpbegin, vk::SubpassContents::eInline);

      if (glfwGetWindowAttrib(window.get(), GLFW_FOCUSED)) {
        camera.axis(float(delta_time.count()) * (1.f + 5 * glfwGetKey(window.get(), GLFW_KEY_LEFT_SHIFT)),
            glfwGetKey(window.get(), GLFW_KEY_W), glfwGetKey(window.get(), GLFW_KEY_S),
            glfwGetKey(window.get(), GLFW_KEY_A), glfwGetKey(window.get(), GLFW_KEY_D),
            glfwGetKey(window.get(), GLFW_KEY_E), glfwGetKey(window.get(), GLFW_KEY_Q));

        double xpos, ypos;
        glfwGetCursorPos(window.get(), &xpos, &ypos);

        camera.mouse(float(xpos), float(ypos), glfwGetMouseButton(window.get(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
      }
      matrices.view = rnu::mat4(1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1) * camera.matrix(false);

      c->pushConstants(render_layout.get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(matrices), &matrices);

      c->bindVertexBuffers(0, {vbo, nbo, ubo, tbo}, {0ull, 0ull, 0ull, 0ull});
      c->bindIndexBuffer(ibo, 0, vk::IndexType::eUint32);
      c->drawIndexed(std::size(bunny[0].indices), std::size(tfs), 0, 0, 0);

      c->endRenderPass();

      // Set a Barrier:
      // From color attachment write
      // to blitting
      {
        vk::ImageMemoryBarrier2KHR i0tr;
        i0tr.image = attachment0img[current_image].image.get();

        i0tr.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
        i0tr.srcAccessMask = vk::AccessFlagBits2KHR::eColorAttachmentWrite;
        i0tr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        i0tr.srcStageMask = vk::PipelineStageFlagBits2KHR::eColorAttachmentOutput;

        i0tr.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        i0tr.dstAccessMask = vk::AccessFlagBits2KHR::eTransferRead;
        i0tr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        i0tr.dstStageMask = vk::PipelineStageFlagBits2KHR::eTransfer;

        i0tr.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

        vk::ImageMemoryBarrier2KHR istr;
        istr.image = swapchain_images[current_image];
        istr.oldLayout = vk::ImageLayout::ePresentSrcKHR;
        istr.srcAccessMask = vk::AccessFlagBits2KHR::eColorAttachmentRead;
        istr.srcQueueFamilyIndex = queues[present_queue_index].family;
        istr.srcStageMask = vk::PipelineStageFlagBits2KHR::eColorAttachmentOutput;

        istr.newLayout = vk::ImageLayout::eTransferDstOptimal;
        istr.dstAccessMask = vk::AccessFlagBits2KHR::eTransferWrite;
        istr.dstQueueFamilyIndex = queues[graphics_queue_index].family;
        istr.dstStageMask = vk::PipelineStageFlagBits2KHR::eTransfer;

        istr.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

        std::array transitions{i0tr, istr};
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

      c->blitImage(attachment0img[current_image].image.get(), vk::ImageLayout::eTransferSrcOptimal,
          swapchain_images[current_image], vk::ImageLayout::eTransferDstOptimal, blit_region, vk::Filter::eNearest);

      // now transition back to the default layouts
      {
        vk::ImageMemoryBarrier2KHR i0tr;
        i0tr.image = attachment0img[current_image].image.get();

        i0tr.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        i0tr.srcAccessMask = vk::AccessFlagBits2KHR::eTransferRead;
        i0tr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        i0tr.srcStageMask = vk::PipelineStageFlagBits2KHR::eTransfer;

        i0tr.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
        i0tr.dstAccessMask = vk::AccessFlagBits2KHR::eColorAttachmentWrite;
        i0tr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        i0tr.dstStageMask = vk::PipelineStageFlagBits2KHR::eColorAttachmentOutput;

        i0tr.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

        vk::ImageMemoryBarrier2KHR istr;
        istr.image = swapchain_images[current_image];
        istr.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        istr.srcAccessMask = vk::AccessFlagBits2KHR::eTransferWrite;
        istr.srcQueueFamilyIndex = queues[graphics_queue_index].family;
        istr.srcStageMask = vk::PipelineStageFlagBits2KHR::eTransfer;

        istr.newLayout = vk::ImageLayout::ePresentSrcKHR;
        istr.dstAccessMask = vk::AccessFlagBits2KHR::eNone;
        istr.dstQueueFamilyIndex = queues[present_queue_index].family;
        istr.dstStageMask = vk::PipelineStageFlagBits2KHR::eBottomOfPipe;

        istr.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

        std::array transitions{i0tr, istr};
        vk::DependencyInfoKHR dep;
        dep.setImageMemoryBarriers(transitions);
        dep.dependencyFlags = vk::DependencyFlagBits::eByRegion;
        c->pipelineBarrier2KHR(dep, ext);
      }
    }
    c->end();

    vk::SubmitInfo submit;
    submit.setWaitSemaphores(image_available_sem.get());
    vk::PipelineStageFlags flags[] = {
        vk::PipelineStageFlagBits::eBottomOfPipe,
    };
    submit.pWaitDstStageMask = flags;
    submit.pCommandBuffers = &c.get();
    submit.commandBufferCount = 1;
    submit.setSignalSemaphores(render_finished_sem.get());
    graphics_queue.submit(submit, render_fences[current_image].get());

    vk::PresentInfoKHR present;
    present.pImageIndices = &current_image;
    present.setWaitSemaphores(render_finished_sem.get());
    present.setSwapchains(swapchain.get());
    present_queue.presentKHR(present);

    glfwSetWindowTitle(window.get(), std::to_string(1 / delta_time.count()).c_str());
    glfwPollEvents();
  }

  device->waitIdle();
}