#include "render/vulkan/RenderLoopRecording.hpp"

namespace iggy3d::vulkan {
namespace {

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

}  // namespace

RenderLoopCommandRecord recordRenderLoopFrameCommands(
    const RenderLoopCreateInfo& createInfo, const FrameInput& frame,
    const RenderLoopFramePlan& framePlan, const SwapchainInfo& readySwapchain,
    std::uint32_t frameSlot, std::uint32_t imageIndex) {
  const DebugHudLayoutResult& debugHud = framePlan.debugHud;
  const ProjectileOverlayLayout& projectileOverlay =
      framePlan.projectileOverlay;
  const std::vector<OverlayRect>& uiOverlayRects = framePlan.uiOverlayRects;
  const auto& creativePreviewDraws = framePlan.creativePreviewDraws;
  const std::size_t creativePreviewDrawCount =
      framePlan.creativePreviewDrawCount;
  const bool drawPackageRoom = framePlan.drawPackageRoom;
  const bool drawUiFrame = framePlan.drawUiFrame;
  const bool drawProxyPrimitives = framePlan.drawProxyPrimitives;
  const bool drawFirstRoom = framePlan.drawFirstRoom;
  const RenderLoopProxySceneFacts& proxyFacts = framePlan.proxyFacts;

  VkCommandBuffer commandBuffer =
      createInfo.commandRecording->commandBufferForFrameSlot(frameSlot);
  CommandRecordResult recordResult;
  if (drawPackageRoom) {
    FirstRoomFrameRecordInfo recordInfo;
    recordInfo.commandBuffer = commandBuffer;
    recordInfo.swapchainImage = createInfo.swapchain->imageAt(imageIndex);
    recordInfo.swapchainImageView = createInfo.swapchain->imageViewAt(imageIndex);
    recordInfo.colorFormat = readySwapchain.colorFormat;
    recordInfo.depthImage =
        createInfo.firstRoomResources->depth().depthImage.allocation.image;
    recordInfo.depthImageView = createInfo.firstRoomResources->depth().depthImage.imageView;
    recordInfo.depthFormat = createInfo.firstRoomResources->depth().depthFormat;
    recordInfo.extent = readySwapchain.extent;
    recordInfo.frameSlot = frameSlot;
    recordInfo.imageIndex = imageIndex;
    recordInfo.pipeline = createInfo.firstRoomPipeline->pipeline;
    recordInfo.viewModelPipeline =
        createInfo.creativeViewModelPipeline->pipeline;
    if (createInfo.materialTexturePipeline != nullptr &&
        createInfo.materialTextureLayout != nullptr) {
      recordInfo.materialTexturePipeline =
          createInfo.materialTexturePipeline->pipeline;
      recordInfo.materialTexturePipelineLayout =
          createInfo.materialTextureLayout->layout;
    }
    recordInfo.pipelineLayout = createInfo.firstRoomLayout->layout;
    const StaticMeshMaterialTextureResources& materialTextures =
        createInfo.firstRoomResources->staticMeshMaterialTextures();
    recordInfo.materialTextureDescriptorSets =
        materialTextures.descriptorSets.data();
    recordInfo.materialTextureDescriptorSetCount =
        materialTextures.descriptorSets.size();
    recordInfo.vertexBuffer =
        createInfo.firstRoomResources->geometry().vertexBuffer.allocation.buffer;
    recordInfo.indexBuffer =
        createInfo.firstRoomResources->geometry().indexBuffer.allocation.buffer;
    recordInfo.indexCount = createInfo.firstRoomResources->geometry().indexCount;
    recordInfo.indexedDraws = createInfo.firstRoomResources->geometry().indexedDraws.data();
    recordInfo.indexedDrawCount =
        createInfo.firstRoomResources->geometry().indexedDraws.size();
    recordInfo.pushConstants =
        renderLoopPushConstantsFromMat4(frame.camera.clipFromWorld);
    populateStaticMeshInstanceRecordInfo(createInfo, recordInfo);
    const CreativePreviewGeometryResources& previewGeometry =
        createInfo.firstRoomResources->creativePreviewGeometry();
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
        readySwapchain.transferSourceSupported && createInfo.frameCapture != nullptr &&
        createInfo.frameCapture->ready();
    if (recordInfo.captureEnabled) {
      recordInfo.captureBuffer = createInfo.frameCapture->buffer();
      recordInfo.captureBufferSize = createInfo.frameCapture->bufferSizeBytes();
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
    recordInfo.externalUiHook = createInfo.externalUiHook;
    recordResult = createInfo.commandRecording->recordFirstRoomFrame(recordInfo);
  } else if (drawProxyPrimitives) {
    ProxyPrimitiveFrameRecordInfo recordInfo;
    recordInfo.commandBuffer = commandBuffer;
    recordInfo.swapchainImage = createInfo.swapchain->imageAt(imageIndex);
    recordInfo.swapchainImageView = createInfo.swapchain->imageViewAt(imageIndex);
    recordInfo.colorFormat = readySwapchain.colorFormat;
    recordInfo.extent = readySwapchain.extent;
    recordInfo.frameSlot = frameSlot;
    recordInfo.imageIndex = imageIndex;
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
    recordInfo.externalUiHook = createInfo.externalUiHook;
    recordResult = createInfo.commandRecording->recordProxyPrimitiveFrame(recordInfo);
  } else if (drawFirstRoom) {
    FirstRoomFrameRecordInfo recordInfo;
    recordInfo.commandBuffer = commandBuffer;
    recordInfo.swapchainImage = createInfo.swapchain->imageAt(imageIndex);
    recordInfo.swapchainImageView = createInfo.swapchain->imageViewAt(imageIndex);
    recordInfo.colorFormat = readySwapchain.colorFormat;
    recordInfo.depthImage =
        createInfo.firstRoomResources->depth().depthImage.allocation.image;
    recordInfo.depthImageView = createInfo.firstRoomResources->depth().depthImage.imageView;
    recordInfo.depthFormat = createInfo.firstRoomResources->depth().depthFormat;
    recordInfo.extent = readySwapchain.extent;
    recordInfo.frameSlot = frameSlot;
    recordInfo.imageIndex = imageIndex;
    recordInfo.pipeline = createInfo.firstRoomPipeline->pipeline;
    recordInfo.viewModelPipeline =
        createInfo.creativeViewModelPipeline->pipeline;
    if (createInfo.materialTexturePipeline != nullptr &&
        createInfo.materialTextureLayout != nullptr) {
      recordInfo.materialTexturePipeline =
          createInfo.materialTexturePipeline->pipeline;
      recordInfo.materialTexturePipelineLayout =
          createInfo.materialTextureLayout->layout;
    }
    recordInfo.pipelineLayout = createInfo.firstRoomLayout->layout;
    const StaticMeshMaterialTextureResources& materialTextures =
        createInfo.firstRoomResources->staticMeshMaterialTextures();
    recordInfo.materialTextureDescriptorSets =
        materialTextures.descriptorSets.data();
    recordInfo.materialTextureDescriptorSetCount =
        materialTextures.descriptorSets.size();
    recordInfo.vertexBuffer =
        createInfo.firstRoomResources->geometry().vertexBuffer.allocation.buffer;
    recordInfo.indexBuffer =
        createInfo.firstRoomResources->geometry().indexBuffer.allocation.buffer;
    recordInfo.indexCount = createInfo.firstRoomResources->geometry().indexCount;
    recordInfo.pushConstants = firstRoomClipFromModel();
    populateStaticMeshInstanceRecordInfo(createInfo, recordInfo);
    const CreativePreviewGeometryResources& previewGeometry =
        createInfo.firstRoomResources->creativePreviewGeometry();
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
        readySwapchain.transferSourceSupported && createInfo.frameCapture != nullptr &&
        createInfo.frameCapture->ready();
    if (recordInfo.captureEnabled) {
      recordInfo.captureBuffer = createInfo.frameCapture->buffer();
      recordInfo.captureBufferSize = createInfo.frameCapture->bufferSizeBytes();
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
    recordInfo.externalUiHook = createInfo.externalUiHook;
    recordResult = createInfo.commandRecording->recordFirstRoomFrame(recordInfo);
  } else {
    EmptyFrameRecordInfo recordInfo;
    recordInfo.commandBuffer = commandBuffer;
    recordInfo.swapchainImage = createInfo.swapchain->imageAt(imageIndex);
    recordInfo.swapchainImageView = createInfo.swapchain->imageViewAt(imageIndex);
    recordInfo.colorFormat = readySwapchain.colorFormat;
    recordInfo.extent = readySwapchain.extent;
    recordInfo.frameSlot = frameSlot;
    recordInfo.imageIndex = imageIndex;
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
    recordInfo.externalUiHook = createInfo.externalUiHook;
    recordResult = createInfo.commandRecording->recordEmptyFrame(recordInfo);
  }

  RenderLoopCommandRecord output;
  output.commandBuffer = commandBuffer;
  output.result = recordResult;
  return output;
}

}  // namespace iggy3d::vulkan
