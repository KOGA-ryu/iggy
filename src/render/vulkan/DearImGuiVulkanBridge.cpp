#include "render/vulkan/DearImGuiVulkanBridge.hpp"

#if defined(IGGY3D_HAS_VULKAN) && defined(IGGY3D_HAS_SDL3)

#include <SDL3/SDL.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

#include "render/vulkan/Swapchain.hpp"

namespace iggy3d::vulkan {

namespace {

constexpr std::uint32_t kUiDescriptorPoolSize = 16;

std::uint32_t clampedMinImageCount(const SwapchainInfo& info) {
  // ImGui_ImplVulkan asserts MinImageCount >= 2. requestedMinImageCount is
  // capabilities.minImageCount + 1 (>= 2 in practice); clamp defensively.
  return info.requestedMinImageCount < 2U ? 2U : info.requestedMinImageCount;
}

}  // namespace

DearImGuiVulkanBridge::~DearImGuiVulkanBridge() {
  shutdown();
}

bool DearImGuiVulkanBridge::initialize(
    const DearImGuiVulkanBridgeCreateInfo& createInfo) {
  if (initialized_) {
    // Idempotent: VulkanBackend::resize can re-run initializePacket5Modules.
    return true;
  }
  createInfo_ = createInfo;
  if (!createInfo_.enabled) {
    return false;
  }
  if (createInfo_.nativeWindow == nullptr || createInfo_.swapchain == nullptr ||
      createInfo_.device == VK_NULL_HANDLE ||
      createInfo_.deviceFunctions.cmdBeginRendering == nullptr ||
      createInfo_.deviceFunctions.cmdEndRendering == nullptr) {
    return false;
  }
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  // Layout persistence arrives with the desktop prefs slice (plan DD-13);
  // until then never write imgui.ini into the CWD.
  io.IniFilename = nullptr;
  if (!ImGui_ImplSDL3_InitForVulkan(
          static_cast<SDL_Window*>(createInfo_.nativeWindow))) {
    ImGui::DestroyContext();
    return false;
  }
  if (!initVulkanBackendObjects()) {
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    return false;
  }
  initialized_ = true;
  return true;
}

bool DearImGuiVulkanBridge::initVulkanBackendObjects() {
  const SwapchainInfo& swapchainInfo = createInfo_.swapchain->info();
  pipelineColorFormat_ = swapchainInfo.colorFormat;
  cachedGeneration_ = swapchainInfo.generation;
  const std::uint32_t minImageCount = clampedMinImageCount(swapchainInfo);

  ImGui_ImplVulkan_InitInfo initInfo{};
  initInfo.ApiVersion = createInfo_.apiVersion;
  initInfo.Instance = createInfo_.instance;
  initInfo.PhysicalDevice = createInfo_.physicalDevice;
  initInfo.Device = createInfo_.device;
  initInfo.QueueFamily = createInfo_.graphicsQueueFamily;
  initInfo.Queue = createInfo_.graphicsQueue;
  initInfo.DescriptorPoolSize = kUiDescriptorPoolSize;
  initInfo.MinImageCount = minImageCount;
  initInfo.ImageCount = swapchainInfo.imageCount < minImageCount
                            ? minImageCount
                            : swapchainInfo.imageCount;
  initInfo.UseDynamicRendering = true;
  VkPipelineRenderingCreateInfoKHR renderingCreateInfo{};
  renderingCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
  renderingCreateInfo.colorAttachmentCount = 1;
  renderingCreateInfo.pColorAttachmentFormats = &pipelineColorFormat_;
  initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = renderingCreateInfo;
  return ImGui_ImplVulkan_Init(&initInfo);
}

void DearImGuiVulkanBridge::shutdown() {
  if (!initialized_) {
    return;
  }
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
  initialized_ = false;
  frameActive_ = false;
  recordedLastFrame_ = false;
}

bool DearImGuiVulkanBridge::enabled() const {
  return initialized_;
}

bool DearImGuiVulkanBridge::frameActive() const {
  return frameActive_;
}

bool DearImGuiVulkanBridge::recordedLastFrame() const {
  return recordedLastFrame_;
}

void DearImGuiVulkanBridge::processEvent(const SDL_Event& event) {
  if (!initialized_) {
    return;
  }
  static_cast<void>(ImGui_ImplSDL3_ProcessEvent(&event));
}

bool DearImGuiVulkanBridge::beginFrame() {
  if (!initialized_) {
    return false;
  }
  if (rebuildPending_) {
    // Color format changed across a swapchain recreate: the UI pipeline is
    // baked to the old format, so rebuild the Vulkan backend objects here,
    // outside command recording, with the device idle.
    static_cast<void>(vkDeviceWaitIdle(createInfo_.device));
    ImGui_ImplVulkan_Shutdown();
    if (!initVulkanBackendObjects()) {
      // Leave the shell dormant rather than half-initialized; ImGui context
      // and the SDL3 backend stay alive for a later successful rebuild.
      return false;
    }
    rebuildPending_ = false;
    minImageCountUpdatePending_ = false;
  } else if (minImageCountUpdatePending_) {
    ImGui_ImplVulkan_SetMinImageCount(
        clampedMinImageCount(createInfo_.swapchain->info()));
    minImageCountUpdatePending_ = false;
  }
  recordedLastFrame_ = false;
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
  frameActive_ = true;
  return true;
}

ExternalUiRecordHook DearImGuiVulkanBridge::recordHook() {
  if (!createInfo_.enabled) {
    return {};
  }
  return {&DearImGuiVulkanBridge::recordThunk, this};
}

bool DearImGuiVulkanBridge::recordThunk(void* user,
                                        const ExternalUiRecordTarget& target) {
  return static_cast<DearImGuiVulkanBridge*>(user)->record(target);
}

bool DearImGuiVulkanBridge::record(const ExternalUiRecordTarget& target) {
  if (!initialized_ || !frameActive_) {
    return false;
  }
  frameActive_ = false;
  const SwapchainInfo& swapchainInfo = createInfo_.swapchain->info();
  if (swapchainInfo.generation != cachedGeneration_) {
    // Covers the in-frame recreate path (RenderLoop recreates without
    // notifying VulkanBackend) as well as app-driven resizes.
    cachedGeneration_ = swapchainInfo.generation;
    if (swapchainInfo.colorFormat != pipelineColorFormat_) {
      rebuildPending_ = true;
      return false;
    }
    minImageCountUpdatePending_ = true;
  }
  if (rebuildPending_) {
    return false;
  }
  ImDrawData* drawData = ImGui::GetDrawData();
  if (drawData == nullptr || drawData->DisplaySize.x <= 0.0F ||
      drawData->DisplaySize.y <= 0.0F) {
    return false;
  }

  // The frame's rendering block just wrote this image as a color attachment;
  // our LOAD_OP_LOAD block writes it again — write-after-write hazard.
  VkImageMemoryBarrier uiBarrier{};
  uiBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  uiBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  uiBarrier.dstAccessMask =
      VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  uiBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  uiBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  uiBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  uiBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  uiBarrier.image = target.colorImage;
  uiBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  uiBarrier.subresourceRange.levelCount = 1;
  uiBarrier.subresourceRange.layerCount = 1;
  vkCmdPipelineBarrier(target.commandBuffer,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0,
                       nullptr, 0, nullptr, 1, &uiBarrier);

  VkRenderingAttachmentInfo colorAttachment{};
  colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  colorAttachment.imageView = target.colorImageView;
  colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  VkRenderingInfo renderingInfo{};
  renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderingInfo.renderArea.offset = {0, 0};
  renderingInfo.renderArea.extent = target.extent;
  renderingInfo.layerCount = 1;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachments = &colorAttachment;

  createInfo_.deviceFunctions.cmdBeginRendering(target.commandBuffer,
                                                &renderingInfo);
  ImGui_ImplVulkan_RenderDrawData(drawData, target.commandBuffer);
  createInfo_.deviceFunctions.cmdEndRendering(target.commandBuffer);
  recordedLastFrame_ = true;
  return true;
}

}  // namespace iggy3d::vulkan

#else  // !(IGGY3D_HAS_VULKAN && IGGY3D_HAS_SDL3)

namespace iggy3d::vulkan {

DearImGuiVulkanBridge::~DearImGuiVulkanBridge() = default;

bool DearImGuiVulkanBridge::initialize(
    const DearImGuiVulkanBridgeCreateInfo& createInfo) {
  createInfo_ = createInfo;
  return false;
}

void DearImGuiVulkanBridge::shutdown() {}

bool DearImGuiVulkanBridge::enabled() const {
  return false;
}

bool DearImGuiVulkanBridge::frameActive() const {
  return false;
}

bool DearImGuiVulkanBridge::recordedLastFrame() const {
  return false;
}

void DearImGuiVulkanBridge::processEvent(const SDL_Event& event) {
  static_cast<void>(event);
}

bool DearImGuiVulkanBridge::beginFrame() {
  return false;
}

ExternalUiRecordHook DearImGuiVulkanBridge::recordHook() {
  return {};
}

bool DearImGuiVulkanBridge::recordThunk(void*, const ExternalUiRecordTarget&) {
  return false;
}

bool DearImGuiVulkanBridge::record(const ExternalUiRecordTarget&) {
  return false;
}

bool DearImGuiVulkanBridge::initVulkanBackendObjects() {
  return false;
}

}  // namespace iggy3d::vulkan

#endif
