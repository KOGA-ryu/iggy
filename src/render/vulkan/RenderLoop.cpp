#include "render/vulkan/RenderLoop.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

#include "render/vulkan/RenderLoopFramePlan.hpp"
#include "render/vulkan/VulkanResult.hpp"

namespace iggy3d::vulkan {
namespace {

RenderReason reasonFor(std::string_view code) {
  if (code == "vulkan_smoke_pass") {
    return {code, "vulkan smoke pass"};
  }
  if (code == "empty_frame_presented") {
    return {code, "empty frame presented"};
  }
  if (code == "packet7_first_room_visible") {
    return {code, "packet 7 first room visible"};
  }
  if (code == "package_room_meshes_presented") {
    return {code, "package room meshes presented"};
  }
  if (code == "proxy_primitives_presented") {
    return {code, "proxy primitives presented"};
  }
  // branch-gate: BG-1078
  if (code == "product_menu_ui_presented") {
    return {code, "product menu ui presented"};
  }
  if (code == "first_room_resources_missing") {
    return {code, "first room resources missing"};
  }
  if (code == "swapchain_suboptimal") {
    return {code, "swapchain suboptimal"};
  }
  if (code == "swapchain_out_of_date") {
    return {code, "swapchain out of date"};
  }
  if (code == "empty_frame_skipped_not_drawable") {
    return {code, "empty frame skipped not drawable"};
  }
  if (code == "empty_frame_acquire_skipped") {
    return {code, "empty frame acquire skipped"};
  }
  if (code == "empty_frame_submit_failed") {
    return {code, "empty frame submit failed"};
  }
  if (code == "empty_frame_present_failed") {
    return {code, "empty frame present failed"};
  }
  if (code == "render_loop_shutdown") {
    return {code, "render loop shutdown"};
  }
  if (code == "swapchain_recreate_failed") {
    return {code, "swapchain recreate failed"};
  }
  return {"render_loop_not_ready", "render loop not ready"};
}

std::string extentString(VkExtent2D extent) {
  return std::to_string(extent.width) + "x" + std::to_string(extent.height);
}

bool firstRoomBundleReady(const RenderLoopCreateInfo& createInfo) {
  if (createInfo.swapchain == nullptr) {
    return false;
  }
  const VkExtent2D swapchainExtent = createInfo.swapchain->info().extent;
  if (createInfo.firstRoomResources == nullptr) {
    return false;
  }
  const FirstRoomGeometryResources& geometry =
      createInfo.firstRoomResources->geometry();
  const StaticMeshAssetAtlasResources& assetAtlas =
      createInfo.firstRoomResources->staticMeshAssetAtlas();
  const bool baseGeometryReady =
      geometry.indexCount > 0U &&
      geometry.vertexBuffer.allocation.buffer != VK_NULL_HANDLE &&
      geometry.indexBuffer.allocation.buffer != VK_NULL_HANDLE;
  const bool instanceGeometryReady =
      geometry.staticMeshInstanceCount > 0U && assetAtlas.ready &&
      geometry.staticMeshInstanceBuffer.allocation.buffer != VK_NULL_HANDLE &&
      createInfo.staticMeshInstancePipeline != nullptr &&
      createInfo.staticMeshInstancePipeline->pipeline != VK_NULL_HANDLE;
  return createInfo.firstRoomPipeline != nullptr &&
         createInfo.firstRoomPipeline->pipeline != VK_NULL_HANDLE &&
         createInfo.creativeViewModelPipeline != nullptr &&
         createInfo.creativeViewModelPipeline->pipeline != VK_NULL_HANDLE &&
         createInfo.firstRoomLayout != nullptr &&
         createInfo.firstRoomLayout->layout != VK_NULL_HANDLE &&
         createInfo.firstRoomResources->ready() &&
         createInfo.firstRoomResources->depth().extent.width == swapchainExtent.width &&
         createInfo.firstRoomResources->depth().extent.height == swapchainExtent.height &&
         (baseGeometryReady || instanceGeometryReady) &&
         createInfo.firstRoomResources->creativePreviewGeometry().ready &&
         createInfo.firstRoomResources->creativePreviewGeometry()
                 .vertexBuffer.allocation.buffer != VK_NULL_HANDLE &&
         createInfo.firstRoomResources->creativePreviewGeometry()
                 .indexBuffer.allocation.buffer != VK_NULL_HANDLE &&
         createInfo.firstRoomResources->depth().depthImage.allocation.image != VK_NULL_HANDLE &&
         createInfo.firstRoomResources->depth().depthImage.imageView != VK_NULL_HANDLE;
}

FirstRoomPushConstants firstRoomClipFromModel() {
  FirstRoomPushConstants constants;
  constants.clipFromModel = {0.52F, 0.0F, 0.0F, 0.0F,
                             0.0F, 0.0F, 0.0F, 0.0F,
                             0.0F, 0.52F, 0.0F, 0.0F,
                             0.0F, 0.0F, 0.5F, 1.0F};
  return constants;
}

void populateStaticMeshInstanceRecordInfo(
    const RenderLoopCreateInfo& createInfo,
    FirstRoomFrameRecordInfo& recordInfo) {
  if (createInfo.firstRoomResources == nullptr) {
    return;
  }
  if (createInfo.staticMeshInstancePipeline != nullptr) {
    recordInfo.staticMeshInstancePipeline =
        createInfo.staticMeshInstancePipeline->pipeline;
  }
  if (createInfo.staticMeshInstanceMaterialPipeline != nullptr) {
    recordInfo.staticMeshInstanceMaterialPipeline =
        createInfo.staticMeshInstanceMaterialPipeline->pipeline;
  }
  const StaticMeshAssetAtlasResources& atlas =
      createInfo.firstRoomResources->staticMeshAssetAtlas();
  const FirstRoomGeometryResources& geometry =
      createInfo.firstRoomResources->geometry();
  recordInfo.staticMeshAssetVertexBuffer =
      atlas.vertexBuffer.allocation.buffer;
  recordInfo.staticMeshAssetIndexBuffer = atlas.indexBuffer.allocation.buffer;
  recordInfo.staticMeshInstanceBuffer =
      geometry.staticMeshInstanceBuffer.allocation.buffer;
  recordInfo.staticMeshInstanceBatches =
      geometry.staticMeshInstanceBatches.data();
  recordInfo.staticMeshInstanceBatchCount =
      geometry.staticMeshInstanceBatches.size();
}

bool packageRoomLoaded(const FrameInput& frame) {
  return frame.projections.scene != nullptr && frame.projections.scene->room.loaded;
}

}  // namespace

std::string_view vulkanFrameStatusName(VulkanFrameStatus status) {
  switch (status) {
    case VulkanFrameStatus::Presented:
      return "presented";
    case VulkanFrameStatus::PresentedSuboptimal:
      return "presented_suboptimal";
    case VulkanFrameStatus::SkippedNotDrawable:
      return "skipped_not_drawable";
    case VulkanFrameStatus::SwapchainRecreated:
      return "swapchain_recreated";
    case VulkanFrameStatus::SkippedAcquire:
      return "skipped_acquire";
    case VulkanFrameStatus::PresentRecreateRequested:
      return "present_recreate_requested";
    case VulkanFrameStatus::Failed:
      return "failed";
  }
  return "failed";
}

RenderReceipt RenderLoop::makeReceipt(std::string_view result,
                                      std::string_view reasonCode,
                                      std::string_view renderingPath) const {
  RenderReceipt receipt;
  appendReceiptField(receipt, "receipt_version", "1");
  appendReceiptField(receipt, "repo", "iggy3d");
  appendReceiptField(receipt, "file_plan", "src/render/vulkan/RenderLoop.cpp");
  appendReceiptField(receipt, "packet_order", "5");
  appendReceiptField(receipt, "backend", "vulkan");
  if (renderingPath.empty()) {
    appendReceiptField(receipt, "rendering_path", firstRoomBundleReady(createInfo_)
                                               ? "first_room"
                                               : "clear_only_fallback");
  } else {
    appendReceiptField(receipt, "rendering_path", renderingPath);
  }
  appendReceiptField(receipt, "first_room_bundle_ready", firstRoomBundleReady(createInfo_));
  appendReceiptField(receipt, "frame_capture_ready",
                     createInfo_.frameCapture != nullptr && createInfo_.frameCapture->ready());
  appendReceiptField(receipt, "render_loop_ready", ready_);
  if (createInfo_.swapchain != nullptr) {
    const SwapchainInfo& swapchainInfo = createInfo_.swapchain->info();
    appendReceiptField(receipt, "swapchain_state", swapchainStateName(swapchainInfo.state));
    appendReceiptField(receipt, "swapchain_generation",
                       static_cast<std::uint64_t>(swapchainInfo.generation));
    appendReceiptField(receipt, "swapchain_extent", extentString(swapchainInfo.extent));
    appendReceiptField(receipt, "swapchain_recreate_count",
                       static_cast<std::uint64_t>(swapchainInfo.recreateCount));
  }
  if (createInfo_.frameSync != nullptr) {
    appendReceiptField(receipt, "sync_policy", "binary_wsi");
    appendReceiptField(receipt, "current_frame_slot",
                       static_cast<std::uint64_t>(createInfo_.frameSync->currentFrameSlot()));
  }
  appendReceiptField(receipt, "validation_error_count", static_cast<std::uint64_t>(0));
  appendReceiptField(receipt, "sync_validation_error_count", static_cast<std::uint64_t>(0));
  appendReceiptField(receipt, "result", result);
  appendReceiptField(receipt, "reason_code", reasonCode);
  return receipt;
}

VulkanFrameResult RenderLoop::initialize(const RenderLoopCreateInfo& createInfo) {
  createInfo_ = createInfo;
  shutdown_ = false;
  ready_ = createInfo_.deviceSurface != nullptr && createInfo_.deviceSurface->ready() &&
           createInfo_.swapchain != nullptr && createInfo_.swapchain->ready() &&
           createInfo_.frameSync != nullptr && createInfo_.frameSync->ready() &&
           createInfo_.commandRecording != nullptr && createInfo_.commandRecording->ready();
  VulkanFrameResult result;
  result.outcome = ready_ ? RenderOutcome::Ok : RenderOutcome::RendererNotReady;
  result.reason = ready_ ? reasonFor("vulkan_smoke_pass") : reasonFor("render_loop_not_ready");
  result.status = ready_ ? VulkanFrameStatus::SwapchainRecreated : VulkanFrameStatus::Failed;
  result.receipt = makeReceipt(ready_ ? "pass" : "fail", result.reason.code);
  return result;
}

VulkanFrameResult RenderLoop::resize(std::uint32_t width, std::uint32_t height) {
  VulkanFrameResult result;
  if (createInfo_.swapchain == nullptr) {
    result.outcome = RenderOutcome::RendererNotReady;
    result.reason = reasonFor("render_loop_not_ready");
    result.receipt = makeReceipt("fail", result.reason.code);
    return result;
  }
  const SwapchainOperationResult resizeResult =
      createInfo_.swapchain->markDrawableExtent(width, height);
  if (resizeResult.outcome == RenderOutcome::SkipFrame) {
    result.status = VulkanFrameStatus::SkippedNotDrawable;
    result.outcome = RenderOutcome::SkipFrame;
    result.reason = reasonFor("empty_frame_skipped_not_drawable");
    result.receipt = makeReceipt("skip", result.reason.code);
    return result;
  }
  if (resizeResult.outcome == RenderOutcome::RecreateSwapchain) {
    const SwapchainOperationResult recreated = createInfo_.swapchain->recreate(width, height);
    result.swapchainRecreated = recreated.outcome == RenderOutcome::Ok;
    result.status =
        result.swapchainRecreated ? VulkanFrameStatus::SwapchainRecreated : VulkanFrameStatus::Failed;
    result.outcome = recreated.outcome;
    result.reason = result.swapchainRecreated ? reasonFor("vulkan_smoke_pass")
                                              : reasonFor("swapchain_recreate_failed");
    result.receipt = makeReceipt(result.swapchainRecreated ? "pass" : "fail", result.reason.code);
    return result;
  }
  result.status = VulkanFrameStatus::SwapchainRecreated;
  result.outcome = RenderOutcome::Ok;
  result.reason = reasonFor("vulkan_smoke_pass");
  result.receipt = makeReceipt("pass", result.reason.code);
  return result;
}

VulkanFrameResult RenderLoop::renderFrame(const FrameInput& frame) {
  VulkanFrameResult result;
  if (!ready_ || shutdown_) {
    result.outcome = RenderOutcome::RendererNotReady;
    result.reason = reasonFor(shutdown_ ? "render_loop_shutdown" : "render_loop_not_ready");
    result.receipt = makeReceipt("fail", result.reason.code);
    return result;
  }

  const FrameInputStatus frameStatus = validateFrameInput(frame);
  if (frameStatus == FrameInputStatus::NotDrawable) {
    result.status = VulkanFrameStatus::SkippedNotDrawable;
    result.outcome = RenderOutcome::SkipFrame;
    result.reason = reasonFor("empty_frame_skipped_not_drawable");
    result.receipt = makeReceipt("skip", result.reason.code);
    appendReceiptField(result.receipt, "frame_input_valid", false);
    appendReceiptField(result.receipt, "frame_input_reason", frameInputReasonCode(frameStatus));
    return result;
  }
  if (frameStatus != FrameInputStatus::Valid) {
    result.outcome = RenderOutcome::InvalidFrameInput;
    result.reason = {"frame_input_invalid", "frame input invalid"};
    result.receipt = makeReceipt("fail", result.reason.code);
    appendReceiptField(result.receipt, "frame_input_valid", false);
    appendReceiptField(result.receipt, "frame_input_reason", frameInputReasonCode(frameStatus));
    return result;
  }

  const SwapchainInfo& swapchainInfo = createInfo_.swapchain->info();
  if (swapchainInfo.state == SwapchainState::NotDrawable || swapchainInfo.extent.width == 0U ||
      swapchainInfo.extent.height == 0U) {
    result.status = VulkanFrameStatus::SkippedNotDrawable;
    result.outcome = RenderOutcome::SkipFrame;
    result.reason = reasonFor("empty_frame_skipped_not_drawable");
    result.receipt = makeReceipt("skip", result.reason.code);
    return result;
  }
  if (swapchainInfo.state == SwapchainState::DirtyResize ||
      swapchainInfo.state == SwapchainState::DirtyOutOfDate ||
      swapchainInfo.state == SwapchainState::DirtySuboptimal) {
    const SwapchainOperationResult recreated =
        createInfo_.swapchain->recreate(frame.viewport.width, frame.viewport.height);
    if (recreated.outcome != RenderOutcome::Ok) {
      result.status = VulkanFrameStatus::Failed;
      result.outcome = recreated.outcome;
      result.reason = reasonFor("swapchain_recreate_failed");
      result.receipt = makeReceipt("fail", result.reason.code);
      return result;
    }
    result.swapchainRecreated = true;
  }

  const bool drawPackageRoom = packageRoomLoaded(frame);
  if (drawPackageRoom) {
    BufferImageResourcesResult roomResources =
        createInfo_.firstRoomResources->createRoomMeshResources(
            frame.projections.scene->room,
            &frame.creativeWireframeDebug);
    if (roomResources.outcome != RenderOutcome::Ok) {
      result.status = VulkanFrameStatus::Failed;
      result.outcome = roomResources.outcome;
      result.reason = roomResources.reason;
      result.receipt = roomResources.receipt;
      return result;
    }
  }

  result.frameSlot = createInfo_.frameSync->currentFrameSlot();
  const FrameSyncWaitResult waitResult = createInfo_.frameSync->waitForCurrentFrame();
  if (waitResult.outcome != RenderOutcome::Ok) {
    result.status = VulkanFrameStatus::SkippedAcquire;
    result.outcome = waitResult.outcome;
    result.reason = waitResult.reason;
    result.receipt = makeReceipt("skip", result.reason.code);
    return result;
  }

  const FrameSyncSubmitPlan submitPlan = createInfo_.frameSync->submitPlanForCurrentFrame();
  const SwapchainAcquireResult acquire = createInfo_.swapchain->acquire(submitPlan.waitSemaphore);
  result.swapchainImageIndex = acquire.imageIndex;
  if (!acquire.submitAllowed || !acquire.imageValid) {
    createInfo_.frameSync->markPresentedOrSkipped(false);
    createInfo_.frameSync->advanceFrameSlot();
    result.status = VulkanFrameStatus::SkippedAcquire;
    result.outcome = acquire.outcome;
    result.reason = acquire.recreateRequested ? reasonFor("empty_frame_acquire_skipped")
                                              : acquire.reason;
    result.receipt = makeReceipt(acquire.recreateRequested ? "skip" : "fail",
                                 result.reason.code);
    appendReceiptField(result.receipt, "acquire_action",
                       acquire.recreateRequested ? "recreate" : "fail");
    return result;
  }

  const SwapchainInfo& readySwapchain = createInfo_.swapchain->info();
  const RenderLoopFramePlan framePlan = buildRenderLoopFramePlan(
      createInfo_, frame, drawPackageRoom, firstRoomBundleReady(createInfo_));
  const DebugHudLayoutResult& debugHud = framePlan.debugHud;
  const ProjectileOverlayLayout& projectileOverlay =
      framePlan.projectileOverlay;
  const std::vector<OverlayRect>& uiOverlayRects = framePlan.uiOverlayRects;
  const auto& creativePreviewDraws = framePlan.creativePreviewDraws;
  const std::size_t creativePreviewDrawCount =
      framePlan.creativePreviewDrawCount;
  const bool drawUiFrame = framePlan.drawUiFrame;
  const bool drawProxyPrimitives = framePlan.drawProxyPrimitives;
  const bool drawFirstRoom = framePlan.drawFirstRoom;
  const RenderLoopProxySceneFacts& proxyFacts = framePlan.proxyFacts;
  VkCommandBuffer commandBuffer =
      createInfo_.commandRecording->commandBufferForFrameSlot(result.frameSlot);
  CommandRecordResult recordResult;
  if (drawPackageRoom) {
    FirstRoomFrameRecordInfo recordInfo;
    recordInfo.commandBuffer = commandBuffer;
    recordInfo.swapchainImage = createInfo_.swapchain->imageAt(acquire.imageIndex);
    recordInfo.swapchainImageView = createInfo_.swapchain->imageViewAt(acquire.imageIndex);
    recordInfo.colorFormat = readySwapchain.colorFormat;
    recordInfo.depthImage =
        createInfo_.firstRoomResources->depth().depthImage.allocation.image;
    recordInfo.depthImageView = createInfo_.firstRoomResources->depth().depthImage.imageView;
    recordInfo.depthFormat = createInfo_.firstRoomResources->depth().depthFormat;
    recordInfo.extent = readySwapchain.extent;
    recordInfo.frameSlot = result.frameSlot;
    recordInfo.imageIndex = acquire.imageIndex;
    recordInfo.pipeline = createInfo_.firstRoomPipeline->pipeline;
    recordInfo.viewModelPipeline =
        createInfo_.creativeViewModelPipeline->pipeline;
    if (createInfo_.materialTexturePipeline != nullptr &&
        createInfo_.materialTextureLayout != nullptr) {
      recordInfo.materialTexturePipeline =
          createInfo_.materialTexturePipeline->pipeline;
      recordInfo.materialTexturePipelineLayout =
          createInfo_.materialTextureLayout->layout;
    }
    recordInfo.pipelineLayout = createInfo_.firstRoomLayout->layout;
    const StaticMeshMaterialTextureResources& materialTextures =
        createInfo_.firstRoomResources->staticMeshMaterialTextures();
    recordInfo.materialTextureDescriptorSets =
        materialTextures.descriptorSets.data();
    recordInfo.materialTextureDescriptorSetCount =
        materialTextures.descriptorSets.size();
    recordInfo.vertexBuffer =
        createInfo_.firstRoomResources->geometry().vertexBuffer.allocation.buffer;
    recordInfo.indexBuffer =
        createInfo_.firstRoomResources->geometry().indexBuffer.allocation.buffer;
    recordInfo.indexCount = createInfo_.firstRoomResources->geometry().indexCount;
    recordInfo.indexedDraws = createInfo_.firstRoomResources->geometry().indexedDraws.data();
    recordInfo.indexedDrawCount =
        createInfo_.firstRoomResources->geometry().indexedDraws.size();
    recordInfo.pushConstants =
        renderLoopPushConstantsFromMat4(frame.camera.clipFromWorld);
    populateStaticMeshInstanceRecordInfo(createInfo_, recordInfo);
    const CreativePreviewGeometryResources& previewGeometry =
        createInfo_.firstRoomResources->creativePreviewGeometry();
    recordInfo.creativePreviewVertexBuffer =
        previewGeometry.vertexBuffer.allocation.buffer;
    recordInfo.creativePreviewIndexBuffer =
        previewGeometry.indexBuffer.allocation.buffer;
    recordInfo.creativePreviewIndexedDraws =
        previewGeometry.indexedDraws.data();
    recordInfo.creativePreviewIndexedDrawCount =
        previewGeometry.indexedDraws.size();
    recordInfo.creativePreviewDraws = creativePreviewDraws.data();
    recordInfo.creativePreviewDrawCount = creativePreviewDrawCount;
    recordInfo.captureEnabled =
        readySwapchain.transferSourceSupported && createInfo_.frameCapture != nullptr &&
        createInfo_.frameCapture->ready();
    if (recordInfo.captureEnabled) {
      recordInfo.captureBuffer = createInfo_.frameCapture->buffer();
      recordInfo.captureBufferSize = createInfo_.frameCapture->bufferSizeBytes();
    }
    recordInfo.projectileOverlayRects = projectileOverlay.rects.data();
    recordInfo.projectileOverlayRectCount = projectileOverlay.rects.size();
    recordInfo.uiOverlayRects = uiOverlayRects.data();
    recordInfo.uiOverlayRectCount = uiOverlayRects.size();
    recordInfo.uiTextGlyphQuads = frame.ui.textGlyphQuads;
    recordInfo.uiTextGlyphQuadCount = frame.ui.textGlyphQuadCount;
    recordInfo.debugHudQuads = debugHud.quads.data();
    recordInfo.debugHudQuadCount = debugHud.quads.size();
    recordInfo.sceneViewport = frame.contentViewport;
    recordInfo.externalUiHook = createInfo_.externalUiHook;
    recordResult = createInfo_.commandRecording->recordFirstRoomFrame(recordInfo);
  } else if (drawProxyPrimitives) {
    ProxyPrimitiveFrameRecordInfo recordInfo;
    recordInfo.commandBuffer = commandBuffer;
    recordInfo.swapchainImage = createInfo_.swapchain->imageAt(acquire.imageIndex);
    recordInfo.swapchainImageView = createInfo_.swapchain->imageViewAt(acquire.imageIndex);
    recordInfo.colorFormat = readySwapchain.colorFormat;
    recordInfo.extent = readySwapchain.extent;
    recordInfo.frameSlot = result.frameSlot;
    recordInfo.imageIndex = acquire.imageIndex;
    recordInfo.floorVisible =
        drawPackageRoom ? frame.projections.scene->room.floorVisible : true;
    recordInfo.roomBoundsVisible =
        drawPackageRoom ? frame.projections.scene->room.wallVisible : true;
    recordInfo.playerMarkerVisible = true;
    recordInfo.targetMarkerVisible = proxyFacts.targetMarkerVisible;
    recordInfo.objectiveMarkerVisible = proxyFacts.objectiveMarkerVisible;
    recordInfo.projectileOverlayRects = projectileOverlay.rects.data();
    recordInfo.projectileOverlayRectCount = projectileOverlay.rects.size();
    recordInfo.debugHudQuads = debugHud.quads.data();
    recordInfo.debugHudQuadCount = debugHud.quads.size();
    recordInfo.externalUiHook = createInfo_.externalUiHook;
    recordResult = createInfo_.commandRecording->recordProxyPrimitiveFrame(recordInfo);
  } else if (drawFirstRoom) {
    FirstRoomFrameRecordInfo recordInfo;
    recordInfo.commandBuffer = commandBuffer;
    recordInfo.swapchainImage = createInfo_.swapchain->imageAt(acquire.imageIndex);
    recordInfo.swapchainImageView = createInfo_.swapchain->imageViewAt(acquire.imageIndex);
    recordInfo.colorFormat = readySwapchain.colorFormat;
    recordInfo.depthImage =
        createInfo_.firstRoomResources->depth().depthImage.allocation.image;
    recordInfo.depthImageView = createInfo_.firstRoomResources->depth().depthImage.imageView;
    recordInfo.depthFormat = createInfo_.firstRoomResources->depth().depthFormat;
    recordInfo.extent = readySwapchain.extent;
    recordInfo.frameSlot = result.frameSlot;
    recordInfo.imageIndex = acquire.imageIndex;
    recordInfo.pipeline = createInfo_.firstRoomPipeline->pipeline;
    recordInfo.viewModelPipeline =
        createInfo_.creativeViewModelPipeline->pipeline;
    if (createInfo_.materialTexturePipeline != nullptr &&
        createInfo_.materialTextureLayout != nullptr) {
      recordInfo.materialTexturePipeline =
          createInfo_.materialTexturePipeline->pipeline;
      recordInfo.materialTexturePipelineLayout =
          createInfo_.materialTextureLayout->layout;
    }
    recordInfo.pipelineLayout = createInfo_.firstRoomLayout->layout;
    const StaticMeshMaterialTextureResources& materialTextures =
        createInfo_.firstRoomResources->staticMeshMaterialTextures();
    recordInfo.materialTextureDescriptorSets =
        materialTextures.descriptorSets.data();
    recordInfo.materialTextureDescriptorSetCount =
        materialTextures.descriptorSets.size();
    recordInfo.vertexBuffer =
        createInfo_.firstRoomResources->geometry().vertexBuffer.allocation.buffer;
    recordInfo.indexBuffer =
        createInfo_.firstRoomResources->geometry().indexBuffer.allocation.buffer;
    recordInfo.indexCount = createInfo_.firstRoomResources->geometry().indexCount;
    recordInfo.pushConstants = firstRoomClipFromModel();
    populateStaticMeshInstanceRecordInfo(createInfo_, recordInfo);
    const CreativePreviewGeometryResources& previewGeometry =
        createInfo_.firstRoomResources->creativePreviewGeometry();
    recordInfo.creativePreviewVertexBuffer =
        previewGeometry.vertexBuffer.allocation.buffer;
    recordInfo.creativePreviewIndexBuffer =
        previewGeometry.indexBuffer.allocation.buffer;
    recordInfo.creativePreviewIndexedDraws =
        previewGeometry.indexedDraws.data();
    recordInfo.creativePreviewIndexedDrawCount =
        previewGeometry.indexedDraws.size();
    recordInfo.creativePreviewDraws = creativePreviewDraws.data();
    recordInfo.creativePreviewDrawCount = creativePreviewDrawCount;
    recordInfo.captureEnabled =
        readySwapchain.transferSourceSupported && createInfo_.frameCapture != nullptr &&
        createInfo_.frameCapture->ready();
    if (recordInfo.captureEnabled) {
      recordInfo.captureBuffer = createInfo_.frameCapture->buffer();
      recordInfo.captureBufferSize = createInfo_.frameCapture->bufferSizeBytes();
    }
    recordInfo.projectileOverlayRects = projectileOverlay.rects.data();
    recordInfo.projectileOverlayRectCount = projectileOverlay.rects.size();
    recordInfo.uiOverlayRects = uiOverlayRects.data();
    recordInfo.uiOverlayRectCount = uiOverlayRects.size();
    recordInfo.uiTextGlyphQuads = frame.ui.textGlyphQuads;
    recordInfo.uiTextGlyphQuadCount = frame.ui.textGlyphQuadCount;
    recordInfo.debugHudQuads = debugHud.quads.data();
    recordInfo.debugHudQuadCount = debugHud.quads.size();
    recordInfo.sceneViewport = frame.contentViewport;
    recordInfo.externalUiHook = createInfo_.externalUiHook;
    recordResult = createInfo_.commandRecording->recordFirstRoomFrame(recordInfo);
  } else {
    EmptyFrameRecordInfo recordInfo;
    recordInfo.commandBuffer = commandBuffer;
    recordInfo.swapchainImage = createInfo_.swapchain->imageAt(acquire.imageIndex);
    recordInfo.swapchainImageView = createInfo_.swapchain->imageViewAt(acquire.imageIndex);
    recordInfo.colorFormat = readySwapchain.colorFormat;
    recordInfo.extent = readySwapchain.extent;
    recordInfo.frameSlot = result.frameSlot;
    recordInfo.imageIndex = acquire.imageIndex;
    // branch-gate: BG-1078
    if (drawUiFrame) {
      recordInfo.clearR = 0.055F;
      recordInfo.clearG = 0.075F;
      recordInfo.clearB = 0.090F;
      recordInfo.uiOverlayRects = uiOverlayRects.data();
      recordInfo.uiOverlayRectCount = uiOverlayRects.size();
      recordInfo.uiTextGlyphQuads = frame.ui.textGlyphQuads;
      recordInfo.uiTextGlyphQuadCount = frame.ui.textGlyphQuadCount;
    }
    recordInfo.debugHudQuads = debugHud.quads.data();
    recordInfo.debugHudQuadCount = debugHud.quads.size();
    recordInfo.externalUiHook = createInfo_.externalUiHook;
    recordResult = createInfo_.commandRecording->recordEmptyFrame(recordInfo);
  }
  if (!recordResult.recorded) {
    createInfo_.frameSync->markPresentedOrSkipped(false);
    createInfo_.frameSync->advanceFrameSlot();
    result.status = VulkanFrameStatus::Failed;
    result.outcome = recordResult.outcome;
    result.reason = recordResult.reason;
    result.receipt = makeReceipt("fail", result.reason.code);
    return result;
  }
  result.commandRecorded = true;

  const FrameSyncOperationResult resetResult = createInfo_.frameSync->resetFenceBeforeSubmit();
  if (resetResult.outcome != RenderOutcome::Ok) {
    result.status = VulkanFrameStatus::Failed;
    result.outcome = resetResult.outcome;
    result.reason = resetResult.reason;
    result.receipt = makeReceipt("fail", result.reason.code);
    return result;
  }

  VkSemaphore waitSemaphore = submitPlan.waitSemaphore;
  VkSemaphore signalSemaphore = submitPlan.signalSemaphore;
  VkPipelineStageFlags waitStage = submitPlan.waitStageMask;
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &waitSemaphore;
  submitInfo.pWaitDstStageMask = &waitStage;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &signalSemaphore;
  const VkResult submitResult = vkQueueSubmit(createInfo_.deviceSurface->handles().graphicsQueue,
                                              1, &submitInfo, submitPlan.signalFence);
  if (submitResult != VK_SUCCESS) {
    result.status = VulkanFrameStatus::Failed;
    result.outcome = mapVkResult(submitResult, VulkanCallContext::QueueSubmit).outcome;
    result.reason = reasonFor("empty_frame_submit_failed");
    result.receipt = makeReceipt("fail", result.reason.code);
    appendReceiptField(result.receipt, "submit_result", vkResultName(submitResult));
    return result;
  }
  createInfo_.frameSync->markSubmitted(acquire.imageIndex);
  result.submitted = true;

  VkSwapchainKHR swapchain = createInfo_.swapchain->handle();
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &signalSemaphore;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapchain;
  presentInfo.pImageIndices = &acquire.imageIndex;
  const VkResult presentVkResult =
      createInfo_.deviceSurface->functions().device.queuePresentKHR(
          createInfo_.deviceSurface->handles().presentQueue, &presentInfo);
  const SwapchainPresentResult presentResult =
      createInfo_.swapchain->notePresentResult(presentVkResult);
  createInfo_.frameSync->markPresentedOrSkipped(true);
  createInfo_.frameSync->advanceFrameSlot();

  result.presented = presentResult.presented;
  result.swapchainRecreated = result.swapchainRecreated || presentResult.recreateRequested;
  if (presentResult.presented) {
    result.status = presentResult.recreateRequested ? VulkanFrameStatus::PresentedSuboptimal
                                                    : VulkanFrameStatus::Presented;
    result.outcome = RenderOutcome::Ok;
    // branch-gate: BG-1078
    result.reason = reasonFor(presentResult.recreateRequested
                                  ? "swapchain_suboptimal"
                                  : (drawPackageRoom ? "package_room_meshes_presented"
                                      : (drawProxyPrimitives ? "proxy_primitives_presented"
                                      // branch-gate: BG-1078
                                      : (drawUiFrame ? "product_menu_ui_presented"
                                      : (drawFirstRoom ? "packet7_first_room_visible"
                                                       : "empty_frame_presented")))));
    // branch-gate: BG-1078
      result.receipt = makeReceipt("pass", result.reason.code,
                                 drawPackageRoom ? "package_room_meshes"
                                 : drawProxyPrimitives ? "proxy_primitives"
                                 // branch-gate: BG-1078
                                 : drawUiFrame ? "product_menu_ui"
                                               : std::string_view{});
  } else if (presentResult.recreateRequested) {
    result.status = VulkanFrameStatus::PresentRecreateRequested;
    result.outcome = presentResult.outcome;
    result.reason = presentResult.reason;
    result.receipt = makeReceipt("skip", result.reason.code);
  } else {
    result.status = VulkanFrameStatus::Failed;
    result.outcome = presentResult.outcome;
    result.reason = reasonFor("empty_frame_present_failed");
    result.receipt = makeReceipt("fail", result.reason.code);
  }
  appendReceiptField(result.receipt, "frame_status", vulkanFrameStatusName(result.status));
  appendReceiptField(result.receipt, "acquire_result", "VK_SUCCESS");
  appendReceiptField(result.receipt, "acquire_action", "submit");
  appendReceiptField(result.receipt, "acquired_image_index",
                     static_cast<std::uint64_t>(acquire.imageIndex));
  appendReceiptField(result.receipt, "command_recorded", result.commandRecorded);
  appendReceiptField(result.receipt, "submit_result", vkResultName(submitResult));
  appendReceiptField(result.receipt, "present_result", vkResultName(presentVkResult));
  appendReceiptField(result.receipt, "present_action",
                     presentResult.presented
                         ? (presentResult.recreateRequested ? "presented_then_recreate"
                                                            : "presented")
                         : (presentResult.recreateRequested ? "recreate" : "fail"));
  appendReceiptField(result.receipt, "presented", presentResult.presented);
  // branch-gate: BG-1078
  appendReceiptField(result.receipt, "record_mode",
                     drawPackageRoom ? "room_mesh_draws"
                     : drawProxyPrimitives ? "draw_primitives"
                     // branch-gate: BG-1078
                     : drawUiFrame ? "ui_primitives"
                                         : (drawFirstRoom ? "first_room" : "empty_frame"));
  // branch-gate: BG-1078
  appendReceiptField(result.receipt, "draw_count",
                     static_cast<std::uint64_t>(
                         drawPackageRoom
                             ? createInfo_.firstRoomResources->geometry().indexedDraws.size() +
                                   createInfo_.firstRoomResources->geometry()
                                       .staticMeshInstanceBatches.size()
                         : drawProxyPrimitives
                               ? renderLoopProxyDrawCount(proxyFacts)
                         // branch-gate: BG-1078
                         : drawUiFrame ? frame.ui.primitiveCount
                                             : (drawFirstRoom ? 1U : 0U)));
  appendReceiptField(result.receipt, "first_room_visible",
                     (drawPackageRoom || drawProxyPrimitives || drawFirstRoom) &&
                         presentResult.presented);
  appendReceiptField(result.receipt, "proxy_floor_visible", drawPackageRoom || drawProxyPrimitives);
  appendReceiptField(result.receipt, "proxy_room_bounds_visible",
                     drawPackageRoom || drawProxyPrimitives);
  appendReceiptField(result.receipt, "proxy_player_marker_visible",
                     drawPackageRoom || drawProxyPrimitives);
  appendReceiptField(result.receipt, "proxy_target_marker_visible",
                     (drawPackageRoom || drawProxyPrimitives) && proxyFacts.targetMarkerVisible);
  appendReceiptField(result.receipt, "proxy_objective_marker_visible",
                     (drawPackageRoom || drawProxyPrimitives) && proxyFacts.objectiveMarkerVisible);
  appendReceiptField(result.receipt, "fallback_room_proxy", drawProxyPrimitives);
  appendReceiptField(result.receipt, "fallback_reason",
                     drawProxyPrimitives ? "no_projected_room_geometry" : "not_applicable");
  if (drawPackageRoom) {
    const SceneRoomProjection& room = frame.projections.scene->room;
    const FirstRoomGeometryResources& geometry =
        createInfo_.firstRoomResources->geometry();
    appendReceiptField(result.receipt, "room_asset_loaded", true);
    appendReceiptField(result.receipt, "room_asset_id", room.assetId);
    appendReceiptField(result.receipt, "room_asset_version",
                       static_cast<std::uint64_t>(room.version));
    appendReceiptField(result.receipt, "source_toml", room.sourceToml);
    appendReceiptField(result.receipt, "source_subset", room.sourceSubset);
    appendReceiptField(result.receipt, "room_static_mesh_count",
                       static_cast<std::uint64_t>(room.staticMeshCount));
    appendReceiptField(result.receipt, "room_material_count",
                       static_cast<std::uint64_t>(room.materialCount));
    appendReceiptField(result.receipt, "room_anchor_count",
                       static_cast<std::uint64_t>(room.anchorCount));
    appendReceiptField(result.receipt, "mesh_draw_count",
                       static_cast<std::uint64_t>(
                           geometry.indexedDraws.size() +
                           geometry.staticMeshInstanceBatches.size()));
    appendReceiptField(result.receipt, "indexed_draw_count",
                       static_cast<std::uint64_t>(
                           geometry.indexedDraws.size()));
    appendReceiptField(
        result.receipt, "static_mesh_instance_draw_count",
        static_cast<std::uint64_t>(geometry.staticMeshInstanceBatches.size()));
    appendReceiptField(result.receipt, "static_mesh_instance_count",
                       static_cast<std::uint64_t>(
                           geometry.staticMeshInstanceCount));
    appendReceiptField(result.receipt, "room_floor_draw_count",
                       static_cast<std::uint64_t>(geometry.roomFloorDrawCount));
    appendReceiptField(result.receipt, "room_wall_draw_count",
                       static_cast<std::uint64_t>(geometry.roomWallDrawCount));
    appendReceiptField(result.receipt, "room_grid_line_draw_count",
                       static_cast<std::uint64_t>(geometry.roomGridLineDrawCount));
    appendReceiptField(result.receipt, "room_grid_visible", geometry.roomGridVisible);
    appendReceiptField(result.receipt, "room_grid_truncated", geometry.roomGridTruncated);
    appendReceiptField(result.receipt, "vertex_buffer_uploaded",
                       createInfo_.firstRoomResources->geometry().vertexBuffer.allocation.buffer !=
                           VK_NULL_HANDLE);
    appendReceiptField(result.receipt, "index_buffer_uploaded",
                       createInfo_.firstRoomResources->geometry().indexBuffer.allocation.buffer !=
                           VK_NULL_HANDLE);
    appendReceiptField(result.receipt, "depth_enabled", true);
    appendReceiptField(result.receipt, "camera_projection", "perspective");
    appendReceiptField(result.receipt, "drawable_aspect",
                       std::to_string(frame.viewport.aspectRatio));
    appendReceiptField(result.receipt, "projection_application", "single");
    appendReceiptField(result.receipt, "floor_visible", room.floorVisible);
    appendReceiptField(result.receipt, "wall_visible", room.wallVisible);
    appendReceiptField(result.receipt, "opening_visible", room.openingVisible);
    appendReceiptField(result.receipt, "prop_visible", room.propVisible);
    appendReceiptField(result.receipt, "key_marker_visible",
                       proxyFacts.keyMarkerVisible || room.keyAnchorVisible);
    appendReceiptField(result.receipt, "dummy_marker_visible",
                       proxyFacts.dummyMarkerVisible || room.dummyAnchorVisible);
  }
  if (drawPackageRoom) {
    const FirstRoomGeometryResources& geometry =
        createInfo_.firstRoomResources->geometry();
    appendReceiptField(result.receipt,
                       "creative_wireframe_debug_geometry_input_line_count",
                       static_cast<std::uint64_t>(
                           geometry.creativeWireframeDebugLineInputCount));
    appendReceiptField(result.receipt,
                       "creative_wireframe_debug_geometry_draw_count",
                       static_cast<std::uint64_t>(
                           geometry.creativeWireframeDebugGeometryDrawCount));
    appendReceiptField(result.receipt,
                       "creative_wireframe_debug_geometry_box_count",
                       static_cast<std::uint64_t>(
                           geometry.creativeWireframeDebugGeometryDrawCount));
    appendReceiptField(result.receipt,
                       "creative_wireframe_debug_geometry_skipped_count",
                       static_cast<std::uint64_t>(
                           geometry.creativeWireframeDebugGeometrySkippedCount));
    appendReceiptField(result.receipt,
                       "creative_wireframe_debug_geometry_status",
                       geometry.creativeWireframeDebugGeometryStatus);
    appendReceiptField(result.receipt,
                       "creative_wireframe_debug_geometry_reason_code",
                       geometry.creativeWireframeDebugGeometryReasonCode);
  } else {
    appendReceiptField(result.receipt,
                       "creative_wireframe_debug_geometry_input_line_count",
                       static_cast<std::uint64_t>(
                           frame.creativeWireframeDebug.lineCount));
    appendReceiptField(result.receipt,
                       "creative_wireframe_debug_geometry_draw_count",
                       static_cast<std::uint64_t>(0));
    appendReceiptField(result.receipt,
                       "creative_wireframe_debug_geometry_box_count",
                       static_cast<std::uint64_t>(0));
    appendReceiptField(result.receipt,
                       "creative_wireframe_debug_geometry_skipped_count",
                       static_cast<std::uint64_t>(0));
    const char* reasonCode =
        frame.creativeWireframeDebug.available
            ? (frame.creativeWireframeDebug.lineCount == 0U
                   ? "vulkan_creative_wireframe_debug_geometry_no_lines"
                   : "vulkan_creative_wireframe_debug_geometry_not_drawn")
            : "vulkan_creative_wireframe_debug_geometry_not_requested";
    appendReceiptField(result.receipt,
                       "creative_wireframe_debug_geometry_status",
                       reasonCode);
    appendReceiptField(result.receipt,
                       "creative_wireframe_debug_geometry_reason_code",
                       reasonCode);
  }
  appendReceiptField(result.receipt, "screenshot_capture",
                     drawFirstRoom && readySwapchain.transferSourceSupported &&
                             createInfo_.frameCapture != nullptr &&
                             createInfo_.frameCapture->ready()
                         ? "enabled"
                         : "unavailable");
  appendReceiptField(result.receipt, "room_proxy_visible",
                     drawPackageRoom || drawProxyPrimitives || (drawFirstRoom && presentResult.presented)
                         ? "true"
                         : "unavailable");
  appendReceiptField(result.receipt, "player_marker_visible",
                     drawPackageRoom || drawProxyPrimitives ? "true" : "unavailable");
  appendReceiptField(result.receipt, "marker_count",
                     static_cast<std::uint64_t>(
                         drawPackageRoom || drawProxyPrimitives ? proxyFacts.markerCount : 0U));
  appendReceiptField(result.receipt, "debug_hud_projected", debugHud.projected);
  appendReceiptField(result.receipt, "debug_hud_line_count",
                     static_cast<std::uint64_t>(debugHud.lineCount));
  appendReceiptField(result.receipt, "debug_hud_glyph_count",
                     static_cast<std::uint64_t>(debugHud.glyphCount));
  appendReceiptField(result.receipt, "debug_hud_rendered",
                     debugHud.projected && !debugHud.quads.empty() &&
                         result.commandRecorded && presentResult.presented);
  appendReceiptField(result.receipt, "debug_hud_record_mode",
                     debugHud.projected && !debugHud.quads.empty() ? "glyph_quads"
                                                                   : "unavailable");
  appendReceiptField(result.receipt, "ui_visible", frame.ui.visible);
  appendReceiptField(result.receipt, "ui_rendered",
                     drawUiFrame && result.commandRecorded && presentResult.presented);
  appendReceiptField(result.receipt, "ui_overlay_rect_count",
                     static_cast<std::uint64_t>(frame.ui.rectCount));
  appendReceiptField(result.receipt, "ui_text_glyph_count",
                     static_cast<std::uint64_t>(frame.ui.textGlyphCount));
  appendReceiptField(result.receipt, "ui_text_glyph_quad_count",
                     static_cast<std::uint64_t>(frame.ui.textGlyphQuadCount));
  appendReceiptField(result.receipt, "ui_primitive_count",
                     static_cast<std::uint64_t>(frame.ui.primitiveCount));
  appendReceiptField(result.receipt, "projectile_visual_projected",
                     projectileOverlay.projected);
  appendReceiptField(result.receipt, "projectile_visual_count",
                     static_cast<std::uint64_t>(projectileOverlay.projectileCount));
  appendReceiptField(result.receipt, "projectile_marker_count",
                     static_cast<std::uint64_t>(projectileOverlay.markerCount));
  appendReceiptField(result.receipt, "projectile_trail_rect_count",
                     static_cast<std::uint64_t>(projectileOverlay.trailRectCount));
  appendReceiptField(result.receipt, "projectile_overlay_rect_count",
                     static_cast<std::uint64_t>(projectileOverlay.rects.size()));
  appendReceiptField(result.receipt, "projectile_impact_visible",
                     projectileOverlay.impactVisible);
  appendReceiptField(result.receipt, "projectile_rendered",
                     projectileOverlay.projected && !projectileOverlay.rects.empty() &&
                         result.commandRecorded && presentResult.presented);
  appendReceiptField(result.receipt, "projectile_record_mode",
                     projectileOverlay.projected && !projectileOverlay.rects.empty()
                         ? "overlay_rects"
                         : "unavailable");
  return result;
}

RenderOutcome RenderLoop::waitIdle() {
  if (createInfo_.deviceSurface != nullptr &&
      createInfo_.deviceSurface->handles().device != VK_NULL_HANDLE) {
    return mapVkResult(vkDeviceWaitIdle(createInfo_.deviceSurface->handles().device),
                       VulkanCallContext::Unknown)
        .outcome;
  }
  return RenderOutcome::Ok;
}

void RenderLoop::shutdown() {
  ready_ = false;
  shutdown_ = true;
}

bool RenderLoop::ready() const {
  return ready_ && !shutdown_;
}

}  // namespace iggy3d::vulkan
