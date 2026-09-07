#include "NativeVulkanHost.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include "render/vulkan/FrameCapture.hpp"
#include "scene/GalleryScene.hpp"
#include "first_room_vert.hpp"
#include "first_room_frag.hpp"

namespace paths {
namespace {
void check(VkResult result, const char* operation) {
  if (result != VK_SUCCESS)
    throw std::runtime_error(std::string(operation) + " (VkResult " +
                             std::to_string(result) + ")");
}
void backendCheck(VkResult result) { check(result, "ImGui Vulkan backend"); }
bool hasExtension(const std::vector<VkExtensionProperties>& list, const char* name) {
  return std::any_of(list.begin(), list.end(), [name](const auto& entry) {
    return std::strcmp(entry.extensionName, name) == 0;
  });
}
constexpr VkFormat kColorFormat = VK_FORMAT_R8G8B8A8_UNORM;
constexpr auto kInfinite = std::numeric_limits<std::uint64_t>::max();
}

struct NativeVulkanHost::Impl {
  NativeLaunchConfig config;
  SDL_Window* window = nullptr;
  bool sdlReady = false, sdlBackendReady = false;
  ImGuiContext* context = nullptr;
  VkInstance instance = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkPhysicalDevice physical = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkQueue queue = VK_NULL_HANDLE;
  std::uint32_t family = 0;
  VkSwapchainKHR swapchain = VK_NULL_HANDLE;
  VkFormat swapFormat = VK_FORMAT_UNDEFINED;
  VkExtent2D extent{};
  std::vector<VkImage> swapImages;
  std::vector<VkSemaphore> presentReady;
  VkSemaphore acquired = VK_NULL_HANDLE;
  bool resizeNeeded = false;
  VkCommandPool pool = VK_NULL_HANDLE;
  VkCommandBuffer command = VK_NULL_HANDLE;
  VkFence fence = VK_NULL_HANDLE;
  VkRenderPass pass = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
  VkFramebuffer framebuffer = VK_NULL_HANDLE;
  iggy3d::vulkan::VulkanMemoryAllocator allocator;
  iggy3d::vulkan::VulkanImageAllocation target;
  iggy3d::vulkan::VulkanImageAllocation depthTarget;
  VkImageView depthView = VK_NULL_HANDLE;
  VkPipelineLayout sceneLayout = VK_NULL_HANDLE;
  VkPipeline scenePipeline = VK_NULL_HANDLE;
  iggy3d::vulkan::VulkanBufferAllocation sceneVertices, sceneIndices;
  void* vertexMapping = nullptr;
  void* indexMapping = nullptr;
  iggy3d::vulkan::FrameCapture readback;
  bool uiRecorded = false;
  bool sceneRecorded = false;
  std::size_t sceneIndexCount = 0;
  std::uint64_t frameNumber = 0;

  explicit Impl(const NativeLaunchConfig& value) : config(value) {}
  ~Impl() {
    if (device) vkDeviceWaitIdle(device);
    if (context) {
      ImGui::SetCurrentContext(context);
      if (ImGui::GetIO().BackendRendererUserData) ImGui_ImplVulkan_Shutdown();
      if (sdlBackendReady) ImGui_ImplSDL3_Shutdown();
      ImGui::DestroyContext(context);
    }
    destroyTarget();
    if (device) {
      if (vertexMapping) vkUnmapMemory(device, sceneVertices.allocation.memory);
      if (indexMapping) vkUnmapMemory(device, sceneIndices.allocation.memory);
      if (sceneVertices.buffer) static_cast<void>(allocator.destroyBuffer(sceneVertices));
      if (sceneIndices.buffer) static_cast<void>(allocator.destroyBuffer(sceneIndices));
      if (scenePipeline) vkDestroyPipeline(device, scenePipeline, nullptr);
      if (sceneLayout) vkDestroyPipelineLayout(device, sceneLayout, nullptr);
      for (const auto semaphore : presentReady) vkDestroySemaphore(device, semaphore, nullptr);
      if (acquired) vkDestroySemaphore(device, acquired, nullptr);
      if (swapchain) vkDestroySwapchainKHR(device, swapchain, nullptr);
      if (fence) vkDestroyFence(device, fence, nullptr);
      if (pool) vkDestroyCommandPool(device, pool, nullptr);
      if (pass) vkDestroyRenderPass(device, pass, nullptr);
      static_cast<void>(allocator.destroy());
      vkDestroyDevice(device, nullptr);
    }
    if (surface) vkDestroySurfaceKHR(instance, surface, nullptr);
    if (instance) vkDestroyInstance(instance, nullptr);
    if (window) SDL_DestroyWindow(window);
    if (sdlReady) SDL_Quit();
  }

  void destroyTarget() {
    readback.destroy();
    if (device && framebuffer) vkDestroyFramebuffer(device, framebuffer, nullptr);
    if (device && view) vkDestroyImageView(device, view, nullptr);
    if (device && depthView) vkDestroyImageView(device, depthView, nullptr);
    framebuffer = VK_NULL_HANDLE;
    view = VK_NULL_HANDLE;
    depthView = VK_NULL_HANDLE;
    if (target.image) static_cast<void>(allocator.destroyImage(target));
    target = {};
    if (depthTarget.image) static_cast<void>(allocator.destroyImage(depthTarget));
    depthTarget = {};
    uiRecorded = false;
    sceneRecorded = false;
  }

  void createScenePipeline() {
    // Port of FirstRoomPipeline's vertex-colour path, using this host's render
    // pass and sole device. Geometry shaders remain byte-for-byte source copies.
    VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT, 0, 64};
    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout.pushConstantRangeCount=1; layout.pPushConstantRanges=&push;
    check(vkCreatePipelineLayout(device,&layout,nullptr,&sceneLayout),"scene layout");
    VkShaderModule vert=VK_NULL_HANDLE, frag=VK_NULL_HANDLE;
    const auto shader=[&](const auto& words,VkShaderModule& module) {
      VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
      info.codeSize=sizeof(words); info.pCode=words;
      check(vkCreateShaderModule(device,&info,nullptr,&module),"scene shader");
    };
    try {
      shader(first_room_vert,vert); shader(first_room_frag,frag);
      std::array<VkPipelineShaderStageCreateInfo,2> stages{};
      stages[0]={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_VERTEX_BIT,vert,"main",nullptr};
      stages[1]={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,nullptr,0,VK_SHADER_STAGE_FRAGMENT_BIT,frag,"main",nullptr};
      const VkVertexInputBindingDescription binding{0,sizeof(SceneVertex),VK_VERTEX_INPUT_RATE_VERTEX};
      const std::array<VkVertexInputAttributeDescription,2> attributes{{
        {0,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(SceneVertex,position)},
        {2,0,VK_FORMAT_R32G32B32_SFLOAT,offsetof(SceneVertex,color)}}};
      VkPipelineVertexInputStateCreateInfo input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
      input.vertexBindingDescriptionCount=1; input.pVertexBindingDescriptions=&binding;
      input.vertexAttributeDescriptionCount=2; input.pVertexAttributeDescriptions=attributes.data();
      VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
      assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
      viewport.viewportCount=1;viewport.scissorCount=1;
      VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
      raster.polygonMode=VK_POLYGON_MODE_FILL;raster.cullMode=VK_CULL_MODE_NONE;raster.lineWidth=1;
      VkPipelineMultisampleStateCreateInfo sampling{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
      sampling.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
      VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
      depth.depthTestEnable=VK_TRUE;depth.depthWriteEnable=VK_TRUE;depth.depthCompareOp=VK_COMPARE_OP_LESS;
      VkPipelineColorBlendAttachmentState color{};
      color.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
      VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
      blend.attachmentCount=1;blend.pAttachments=&color;
      const std::array<VkDynamicState,2> states{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
      VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
      dynamic.dynamicStateCount=2;dynamic.pDynamicStates=states.data();
      VkGraphicsPipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
      pipeline.stageCount=2;pipeline.pStages=stages.data();pipeline.pVertexInputState=&input;
      pipeline.pInputAssemblyState=&assembly;pipeline.pViewportState=&viewport;pipeline.pRasterizationState=&raster;
      pipeline.pMultisampleState=&sampling;pipeline.pDepthStencilState=&depth;
      pipeline.pColorBlendState=&blend;pipeline.pDynamicState=&dynamic;pipeline.layout=sceneLayout;pipeline.renderPass=pass;
      check(vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&pipeline,nullptr,&scenePipeline),"scene pipeline");
    } catch (...) {
      if(vert)vkDestroyShaderModule(device,vert,nullptr);
      if(frag)vkDestroyShaderModule(device,frag,nullptr);
      throw;
    }
    vkDestroyShaderModule(device,vert,nullptr);vkDestroyShaderModule(device,frag,nullptr);
    const auto allocate=[&](const char* name,VkDeviceSize bytes,VkBufferUsageFlags usage,
                            auto& destination,void*& mapping) {
      const auto result=allocator.createBuffer(name,bytes,usage,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
      if(result.outcome!=iggy3d::RenderOutcome::Ok)throw std::runtime_error(iggy3d::formatRenderReceipt(result.receipt));
      destination=result.buffer;
      check(vkMapMemory(device,destination.allocation.memory,0,bytes,0,&mapping),"map scene buffer");
    };
    allocate("paths_scene_vertices",kSceneVertexCapacity*sizeof(SceneVertex),VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,sceneVertices,vertexMapping);
    allocate("paths_scene_indices",kSceneIndexCapacity*sizeof(std::uint16_t),VK_BUFFER_USAGE_INDEX_BUFFER_BIT,sceneIndices,indexMapping);
  }

  void recordScene(const SceneFrame& scene) {
    if(!config.enableScene || !scenePipeline)throw std::runtime_error("Scene rendering was not enabled");
    if(scene.vertices.size()>kSceneVertexCapacity || scene.indices.size()>kSceneIndexCapacity)
      throw std::runtime_error("Scene exceeds Vulkan buffer capacity");
    if(scene.vertices.empty() || scene.indices.empty())return;
    if(!iggy3d::isFinite(scene.clipFromWorld) || scene.indices.size()%3!=0 ||
       std::any_of(scene.indices.begin(),scene.indices.end(),[&](auto i){return i>=scene.vertices.size();}))
      throw std::runtime_error("Invalid scene matrix or triangle indices");
    const auto scale=ImGui::GetIO().DisplayFramebufferScale;
    const auto& v=scene.viewport;
    const float x=v.x*scale.x,y=v.y*scale.y,w=v.width*scale.x,h=v.height*scale.y;
    if(!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(w) || !std::isfinite(h) ||
       x<0 || y<0 || w<1 || h<1 || x+w>extent.width+0.5F || y+h>extent.height+0.5F)
      throw std::runtime_error("Scene viewport outside render target");
    std::memcpy(vertexMapping,scene.vertices.data(),scene.vertices.size()*sizeof(SceneVertex));
    std::memcpy(indexMapping,scene.indices.data(),scene.indices.size()*sizeof(std::uint16_t));
    vkCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,scenePipeline);
    const VkViewport viewport{x,y,w,h,0,1};
    const VkRect2D scissor{{static_cast<std::int32_t>(x),static_cast<std::int32_t>(y)},
      {static_cast<std::uint32_t>(w),static_cast<std::uint32_t>(h)}};
    vkCmdSetViewport(command,0,1,&viewport);vkCmdSetScissor(command,0,1,&scissor);
    std::array<float,16> matrix{};
    for(std::size_t row=0;row<4;++row)for(std::size_t col=0;col<4;++col)
      matrix[col*4+row]=scene.clipFromWorld.m[row*4+col];
    vkCmdPushConstants(command,sceneLayout,VK_SHADER_STAGE_VERTEX_BIT,0,64,matrix.data());
    const VkDeviceSize offset=0;
    vkCmdBindVertexBuffers(command,0,1,&sceneVertices.buffer,&offset);
    vkCmdBindIndexBuffer(command,sceneIndices.buffer,0,VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(command,static_cast<std::uint32_t>(scene.indices.size()),1,0,0,0);
    sceneRecorded=true;sceneIndexCount=scene.indices.size();
  }

  void initialize() {
#ifdef PATHS_DEFAULT_VULKAN_ICD
    if (!std::getenv("VK_DRIVER_FILES") && !std::getenv("VK_ICD_FILENAMES"))
      setenv("VK_DRIVER_FILES", PATHS_DEFAULT_VULKAN_ICD, 0);
#endif
    std::vector<const char*> extensions;
    if (!config.offscreen) {
      if (!SDL_Init(SDL_INIT_VIDEO)) throw std::runtime_error(SDL_GetError());
      sdlReady = true;
      window = SDL_CreateWindow("Paths", static_cast<int>(config.width),
          static_cast<int>(config.height), SDL_WINDOW_VULKAN |
          SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
      if (!window) throw std::runtime_error(SDL_GetError());
      if(config.enableScene && !SDL_SetWindowMinimumSize(window,800,600))
        throw std::runtime_error(SDL_GetError());
      std::uint32_t count = 0;
      const char* const* required = SDL_Vulkan_GetInstanceExtensions(&count);
      if (!required) throw std::runtime_error(SDL_GetError());
      extensions.assign(required, required + count);
    }
    std::uint32_t count = 0;
    check(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr), "instance extensions");
    std::vector<VkExtensionProperties> available(count);
    check(vkEnumerateInstanceExtensionProperties(nullptr, &count, available.data()), "instance extensions");
    VkInstanceCreateFlags flags = 0;
    if (hasExtension(available, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
      extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
      flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }
    for (const char* name : extensions)
      if (!hasExtension(available, name))
        throw std::runtime_error(std::string("Required Vulkan extension unavailable: ") + name);
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "Paths";
    app.apiVersion = VK_API_VERSION_1_1;
    VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    info.flags = flags;
    info.pApplicationInfo = &app;
    info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
    info.ppEnabledExtensionNames = extensions.data();
    check(vkCreateInstance(&info, nullptr, &instance), "create Vulkan instance");
    if (window && !SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface))
      throw std::runtime_error(SDL_GetError());
    check(vkEnumeratePhysicalDevices(instance, &count, nullptr), "enumerate Vulkan devices");
    std::vector<VkPhysicalDevice> devices(count);
    check(vkEnumeratePhysicalDevices(instance, &count, devices.data()), "enumerate Vulkan devices");
    std::vector<const char*> deviceExtensions;
    for (const auto candidate : devices) {
      VkPhysicalDeviceProperties properties{};
      vkGetPhysicalDeviceProperties(candidate, &properties);
      if (properties.apiVersion < VK_API_VERSION_1_1) continue;
      std::uint32_t extensionCount = 0;
      check(vkEnumerateDeviceExtensionProperties(candidate, nullptr, &extensionCount, nullptr), "device extensions");
      std::vector<VkExtensionProperties> candidateExtensions(extensionCount);
      check(vkEnumerateDeviceExtensionProperties(candidate, nullptr, &extensionCount, candidateExtensions.data()), "device extensions");
      if (surface && !hasExtension(candidateExtensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) continue;
      std::uint32_t queueCount = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queueCount, nullptr);
      std::vector<VkQueueFamilyProperties> queues(queueCount);
      vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queueCount, queues.data());
      for (std::uint32_t index = 0; index < queueCount; ++index) {
        if (!(queues[index].queueFlags & VK_QUEUE_GRAPHICS_BIT)) continue;
        VkBool32 supportsPresent = VK_TRUE;
        if (surface) check(vkGetPhysicalDeviceSurfaceSupportKHR(candidate, index, surface, &supportsPresent), "surface support");
        if (!supportsPresent) continue;
        physical = candidate;
        family = index;
        if (surface) deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        if (hasExtension(candidateExtensions, "VK_KHR_portability_subset"))
          deviceExtensions.push_back("VK_KHR_portability_subset");
        break;
      }
      if (physical) break;
    }
    if (!physical) throw std::runtime_error("No compatible Vulkan 1.1 graphics device/queue available");
    VkFormatProperties formatProperties{};
    vkGetPhysicalDeviceFormatProperties(physical, kColorFormat, &formatProperties);
    const auto requiredFeatures = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
    if ((formatProperties.optimalTilingFeatures & requiredFeatures) != requiredFeatures)
      throw std::runtime_error("Vulkan device cannot render/capture an RGBA8 target");
    float priority = 1;
    VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueInfo.queueFamilyIndex = family;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;
    VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledExtensionCount = static_cast<std::uint32_t>(deviceExtensions.size());
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
    check(vkCreateDevice(physical, &deviceInfo, nullptr, &device), "create Vulkan device");
    vkGetDeviceQueue(device, family, 0, &queue);
    static_cast<void>(allocator.create({physical, device, VK_API_VERSION_1_1}));

    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = family;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    check(vkCreateCommandPool(device, &poolInfo, nullptr, &pool), "create command pool");
    VkCommandBufferAllocateInfo commandInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    commandInfo.commandPool = pool;
    commandInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandInfo.commandBufferCount = 1;
    check(vkAllocateCommandBuffers(device, &commandInfo, &command), "allocate command buffer");
    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    check(vkCreateFence(device, &fenceInfo, nullptr, &fence), "create frame fence");
    std::array<VkAttachmentDescription,2> attachments{};
    auto& attachment=attachments[0];
    attachment.format = kColorFormat;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    VkAttachmentReference reference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depthReference{1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    if(config.enableScene) {
      VkFormatProperties depthProperties{};
      vkGetPhysicalDeviceFormatProperties(physical,VK_FORMAT_D32_SFLOAT,&depthProperties);
      if(!(depthProperties.optimalTilingFeatures&VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
        throw std::runtime_error("Scene requires D32 depth attachment support");
      attachments[1]=attachment;
      attachments[1].format=VK_FORMAT_D32_SFLOAT;
      attachments[1].storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
      attachments[1].finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &reference;
    if(config.enableScene)subpass.pDepthStencilAttachment=&depthReference;
    std::array<VkSubpassDependency, 2> dependencies{};
    dependencies[0] = {VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_TRANSFER_READ_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0};
    dependencies[1] = {0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_ACCESS_TRANSFER_READ_BIT, 0};
    if(config.enableScene) {
      dependencies[0].srcStageMask|=VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      dependencies[0].srcAccessMask|=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      dependencies[0].dstStageMask|=VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      dependencies[0].dstAccessMask|=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    VkRenderPassCreateInfo passInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    passInfo.attachmentCount = config.enableScene ? 2U:1U;
    passInfo.pAttachments = attachments.data();
    passInfo.subpassCount = 1;
    passInfo.pSubpasses = &subpass;
    passInfo.dependencyCount = static_cast<std::uint32_t>(dependencies.size());
    passInfo.pDependencies = dependencies.data();
    check(vkCreateRenderPass(device, &passInfo, nullptr, &pass), "create UI render pass");
    if(config.enableScene)createScenePipeline();
    if (surface) {
      VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
      check(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &acquired), "create acquire semaphore");
      recreateSwapchain();
    } else {
      extent = {config.width, config.height};
    }
    createTarget();
    IMGUI_CHECKVERSION();
    context = ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();
    if (window) {
      if (!ImGui_ImplSDL3_InitForVulkan(window)) throw std::runtime_error("SDL ImGui backend initialization failed");
      sdlBackendReady = true;
    }
    ImGui_ImplVulkan_InitInfo imgui{};
    imgui.ApiVersion = VK_API_VERSION_1_1;
    imgui.Instance = instance;
    imgui.PhysicalDevice = physical;
    imgui.Device = device;
    imgui.QueueFamily = family;
    imgui.Queue = queue;
    imgui.DescriptorPoolSize = 128;
    imgui.MinImageCount = 2;
    imgui.ImageCount = 2;
    imgui.PipelineInfoMain.RenderPass = pass;
    imgui.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    imgui.CheckVkResultFn = backendCheck;
    if (!ImGui_ImplVulkan_Init(&imgui)) throw std::runtime_error("Vulkan ImGui backend initialization failed");
  }

  void recreateSwapchain() {
    check(vkDeviceWaitIdle(device), "wait before swapchain recreation");
    VkSurfaceCapabilitiesKHR capabilities{};
    check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &capabilities), "surface capabilities");
    if (!(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
      throw std::runtime_error("Surface cannot accept the Paths UI transfer");
    std::uint32_t count = 0;
    check(vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &count, nullptr), "surface formats");
    std::vector<VkSurfaceFormatKHR> formats(count);
    check(vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &count, formats.data()), "surface formats");
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
      formats[0].format = kColorFormat;
    const auto selected = std::find_if(formats.begin(), formats.end(), [](const auto& format) {
      return (format.format == kColorFormat || format.format == VK_FORMAT_B8G8R8A8_UNORM) &&
             format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    });
    if (selected == formats.end()) throw std::runtime_error("Surface lacks a supported RGBA/BGRA UNORM format");
    swapFormat = selected->format;
    if (swapFormat != kColorFormat) {
      VkFormatProperties source{}, destination{};
      vkGetPhysicalDeviceFormatProperties(physical, kColorFormat, &source);
      vkGetPhysicalDeviceFormatProperties(physical, swapFormat, &destination);
      if (!(source.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) ||
          !(destination.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT))
        throw std::runtime_error("Surface channel conversion requires unsupported Vulkan blitting");
    }
    int width = 0, height = 0;
    if (!SDL_GetWindowSizeInPixels(window, &width, &height)) throw std::runtime_error(SDL_GetError());
    extent = capabilities.currentExtent;
    if (extent.width == std::numeric_limits<std::uint32_t>::max()) {
      extent.width = std::clamp(static_cast<std::uint32_t>(std::max(1, width)), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
      extent.height = std::clamp(static_cast<std::uint32_t>(std::max(1, height)), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }
    VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    info.surface = surface;
    info.minImageCount = std::max(2U, capabilities.minImageCount);
    if (capabilities.maxImageCount) info.minImageCount = std::min(info.minImageCount, capabilities.maxImageCount);
    info.imageFormat = selected->format;
    info.imageColorSpace = selected->colorSpace;
    info.imageExtent = extent;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.preTransform = capabilities.currentTransform;
    for (const auto alpha : {VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR}) {
      if (capabilities.supportedCompositeAlpha & alpha) { info.compositeAlpha = alpha; break; }
    }
    info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    info.clipped = VK_TRUE;
    info.oldSwapchain = swapchain;
    VkSwapchainKHR next = VK_NULL_HANDLE;
    check(vkCreateSwapchainKHR(device, &info, nullptr, &next), "create swapchain");
    for (const auto semaphore : presentReady) vkDestroySemaphore(device, semaphore, nullptr);
    presentReady.clear();
    if (swapchain) vkDestroySwapchainKHR(device, swapchain, nullptr);
    swapchain = next;
    check(vkGetSwapchainImagesKHR(device, swapchain, &count, nullptr), "swapchain images");
    swapImages.resize(count);
    check(vkGetSwapchainImagesKHR(device, swapchain, &count, swapImages.data()), "swapchain images");
    presentReady.resize(count, VK_NULL_HANDLE);
    VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    for (auto& semaphore : presentReady)
      check(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore), "create present semaphore");
    resizeNeeded = false;
  }

  void createTarget() {
    auto result = allocator.createImage("paths_ui", {extent.width, extent.height, 1}, kColorFormat,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (result.outcome != iggy3d::RenderOutcome::Ok)
      throw std::runtime_error(iggy3d::formatRenderReceipt(result.receipt));
    target = result.image;
    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = target.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = kColorFormat;
    viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    check(vkCreateImageView(device, &viewInfo, nullptr, &view), "create UI image view");
    if(config.enableScene) {
      const auto depth=allocator.createImage("paths_depth",{extent.width,extent.height,1},VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
      if(depth.outcome!=iggy3d::RenderOutcome::Ok)throw std::runtime_error(iggy3d::formatRenderReceipt(depth.receipt));
      depthTarget=depth.image;
      viewInfo.image=depthTarget.image;viewInfo.format=VK_FORMAT_D32_SFLOAT;
      viewInfo.subresourceRange.aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT;
      check(vkCreateImageView(device,&viewInfo,nullptr,&depthView),"create depth image view");
    }
    const std::array<VkImageView,2> views{view,depthView};
    VkFramebufferCreateInfo framebufferInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    framebufferInfo.renderPass = pass;
    framebufferInfo.attachmentCount = config.enableScene ? 2U:1U;
    framebufferInfo.pAttachments = views.data();
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    framebufferInfo.layers = 1;
    check(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer), "create UI framebuffer");
    const auto receipt = readback.create({physical, device, extent, kColorFormat});
    if (!readback.ready()) throw std::runtime_error(iggy3d::formatRenderReceipt(receipt));
  }

  NativeFrameResult frame(const std::function<void(const SDL_Event&)>& onEvent,
                          const std::function<void()>& draw, const SceneFrame* scene) {
    ImGui::SetCurrentContext(context);
    if (window) {
      SDL_Event event;
      while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        onEvent(event);
        if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
          return {FrameStatus::Closed, {}};
        if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) resizeNeeded = true;
      }
      int width = 0, height = 0;
      if (!SDL_GetWindowSizeInPixels(window, &width, &height)) throw std::runtime_error(SDL_GetError());
      if (width <= 0 || height <= 0 || (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)) {
        SDL_Delay(16);
        return {FrameStatus::Skipped, {}};
      }
      if (resizeNeeded) {
        recreateSwapchain();
        destroyTarget();
        createTarget();
      }
    }
    check(vkWaitForFences(device, 1, &fence, VK_TRUE, kInfinite), "wait for UI frame");
    std::uint32_t imageIndex = 0;
    if (surface) {
      const auto result = vkAcquireNextImageKHR(device, swapchain, kInfinite, acquired, VK_NULL_HANDLE, &imageIndex);
      if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        resizeNeeded = true;
        return {FrameStatus::Skipped, {}};
      }
      if (result == VK_SUBOPTIMAL_KHR) resizeNeeded = true;
      else check(result, "acquire swapchain image");
    }
    ImGui_ImplVulkan_NewFrame();
    if (window) ImGui_ImplSDL3_NewFrame();
    else {
      ImGui::GetIO().DisplaySize = {static_cast<float>(extent.width), static_cast<float>(extent.height)};
      ImGui::GetIO().DisplayFramebufferScale = {1, 1};
      ImGui::GetIO().DeltaTime = 1.0F / 60.0F;
    }
    ImGui::NewFrame();
    draw();
    ImGui::Render();
    check(vkResetCommandBuffer(command, 0), "reset UI command buffer");
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    check(vkBeginCommandBuffer(command, &begin), "begin UI command buffer");
    std::array<VkClearValue,2> clear{};
    clear[0].color={{0.035F,0.055F,0.075F,1}};
    clear[1].depthStencil={1,0};
    VkRenderPassBeginInfo render{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    render.renderPass = pass;
    render.framebuffer = framebuffer;
    render.renderArea.extent = extent;
    render.clearValueCount = config.enableScene ? 2U:1U;
    render.pClearValues = clear.data();
    vkCmdBeginRenderPass(command, &render, VK_SUBPASS_CONTENTS_INLINE);
    sceneRecorded=false;sceneIndexCount=0;
    if(scene)recordScene(*scene);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command);
    uiRecorded = ImGui::GetDrawData()->TotalVtxCount > 0;
    vkCmdEndRenderPass(command);
    VkBufferImageCopy copy{};
    copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copy.imageExtent = {extent.width, extent.height, 1};
    vkCmdCopyImageToBuffer(command, target.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           readback.buffer(), 1, &copy);
    VkBufferMemoryBarrier hostRead{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    hostRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    hostRead.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    hostRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    hostRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    hostRead.buffer = readback.buffer();
    hostRead.size = VK_WHOLE_SIZE;
    vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                          0, 0, nullptr, 1, &hostRead, 0, nullptr);
    if (surface) {
      VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
      barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.image = swapImages[imageIndex];
      barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
      vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                            0, 0, nullptr, 0, nullptr, 1, &barrier);
      if (swapFormat == kColorFormat) {
        VkImageCopy imageCopy{};
        imageCopy.srcSubresource = copy.imageSubresource;
        imageCopy.dstSubresource = copy.imageSubresource;
        imageCopy.extent = copy.imageExtent;
        vkCmdCopyImage(command, target.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                         barrier.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
      } else {
        VkImageBlit blit{};
        blit.srcSubresource = copy.imageSubresource;
        blit.dstSubresource = copy.imageSubresource;
        blit.srcOffsets[1] = {static_cast<std::int32_t>(extent.width), static_cast<std::int32_t>(extent.height), 1};
        blit.dstOffsets[1] = blit.srcOffsets[1];
        vkCmdBlitImage(command, target.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                         barrier.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_NEAREST);
      }
      barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask = 0;
      barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
      vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                            0, 0, nullptr, 0, nullptr, 1, &barrier);
    }
    check(vkEndCommandBuffer(command), "end UI command buffer");
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &command;
    if (surface) {
      submit.waitSemaphoreCount = 1;
      submit.pWaitSemaphores = &acquired;
      submit.pWaitDstStageMask = &waitStage;
      submit.signalSemaphoreCount = 1;
      submit.pSignalSemaphores = &presentReady[imageIndex];
    }
    check(vkResetFences(device, 1, &fence), "reset frame fence");
    check(vkQueueSubmit(queue, 1, &submit, fence), "submit UI frame");
    if (surface) {
      VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
      present.waitSemaphoreCount = 1;
      present.pWaitSemaphores = &presentReady[imageIndex];
      present.swapchainCount = 1;
      present.pSwapchains = &swapchain;
      present.pImageIndices = &imageIndex;
      const auto result = vkQueuePresentKHR(queue, &present);
      if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) resizeNeeded = true;
      else check(result, "present UI frame");
    }
    ++frameNumber;
    return {FrameStatus::Rendered, {}};
  }
};

CapturePaths capturePaths(const std::filesystem::path& screenshot) {
  CapturePaths result{screenshot, screenshot, screenshot, screenshot};
  result.rawPath.replace_extension(".rgba");
  result.metaPath.replace_extension(".meta.kv");
  result.hashPath.replace_extension(".sha256");
  return result;
}
NativeVulkanHost::NativeVulkanHost(const NativeLaunchConfig& config)
    : impl_(std::make_unique<Impl>(config)) { impl_->initialize(); }
NativeVulkanHost::~NativeVulkanHost() = default;
NativeFrameResult NativeVulkanHost::frame(
    const std::function<void(const SDL_Event&)>& onEvent, const std::function<void()>& draw,
    const SceneFrame* scene) {
  try { return impl_->frame(onEvent, draw, scene); }
  catch (const std::exception& error) { return {FrameStatus::Failed, error.what()}; }
}
bool NativeVulkanHost::capture(const CapturePaths& paths, std::string& error) {
  try {
    if (!impl_->uiRecorded || !impl_->frameNumber) throw std::runtime_error("No rendered UI frame is available for capture");
    check(vkWaitForFences(impl_->device, 1, &impl_->fence, VK_TRUE, kInfinite), "wait for capture");
    const auto pixels = impl_->readback.readMappedRgba();
    bool visible = false;
    for (std::size_t i = 4; i + 3 < pixels.rgba.size(); i += 4)
      if (pixels.rgba[i + 3] && (pixels.rgba[i] != pixels.rgba[0] ||
          pixels.rgba[i + 1] != pixels.rgba[1] || pixels.rgba[i + 2] != pixels.rgba[2])) { visible = true; break; }
    if (!visible) throw std::runtime_error("Capture has no nonuniform UI pixels");
    const auto result = iggy3d::vulkan::writePacket7CaptureArtifacts(pixels,
        {paths.screenshotPath, paths.rawPath, paths.metaPath, paths.hashPath, true});
    if (!result.written) throw std::runtime_error(iggy3d::formatRenderReceipt(result.receipt));
    std::ofstream receipt(paths.metaPath, std::ios::app);
    receipt << "product=paths\nhost=native_vulkan\nhelper_origin=iggy3d\nui_recorded=true\nnonuniform_rgb=true\noffscreen="
            << (impl_->config.offscreen ? "true" : "false") << '\n';
    receipt << "scene_recorded=" << (impl_->sceneRecorded ? "true":"false")
            << "\nscene_index_count=" << impl_->sceneIndexCount
            << "\ndepth_enabled=" << (impl_->config.enableScene ? "true":"false") << '\n';
    receipt.flush();
    if (!receipt) throw std::runtime_error("Cannot write Paths capture receipt");
    return true;
  } catch (const std::exception& failure) { error = failure.what(); return false; }
}
}  // namespace paths
