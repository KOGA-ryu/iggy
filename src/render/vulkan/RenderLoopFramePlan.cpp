#include "render/vulkan/RenderLoopFramePlan.hpp"

#include <algorithm>

namespace iggy3d::vulkan {
namespace {

RenderLoopProxySceneFacts proxySceneFacts(const FrameInput& frame) {
  RenderLoopProxySceneFacts facts;
  if (frame.projections.scene == nullptr) {
    return facts;
  }
  for (const SceneItem& item : frame.projections.scene->items) {
    if (!item.visible) {
      continue;
    }
    if (item.stableName == "training_dummy" || item.targetable ||
        item.kind == SceneItemKind::Interactable) {
      facts.targetMarkerVisible = true;
    }
    if (item.stableName == "training_dummy") {
      facts.dummyMarkerVisible = true;
    }
    if (item.stableName == "gold_key" || item.kind == SceneItemKind::Pickup ||
        item.kind == SceneItemKind::ObjectiveMarker) {
      facts.objectiveMarkerVisible = true;
    }
    if (item.stableName == "gold_key") {
      facts.keyMarkerVisible = true;
    }
  }
  facts.markerCount = 2U + (facts.targetMarkerVisible ? 1U : 0U) +
                      (facts.objectiveMarkerVisible ? 1U : 0U);
  return facts;
}

bool sceneHasRenderableContent(const FrameInput& frame) {
  if (frame.projections.scene == nullptr) {
    return false;
  }
  return frame.projections.scene->room.loaded ||
         !frame.projections.scene->items.empty() ||
         !frame.projections.scene->projectiles.empty();
}

bool frameHasUiContent(const FrameInput& frame) {
  return frame.ui.visible &&
         ((frame.ui.rects != nullptr && frame.ui.rectCount > 0U) ||
          (frame.ui.textGlyphQuads != nullptr &&
           frame.ui.textGlyphQuadCount > 0U));
}

DebugHudLayoutResult debugHudLayoutFor(const FrameInput& frame) {
  if (frame.projections.debug == nullptr ||
      frame.projections.debug->runtimeDebugHudLines.empty()) {
    return {};
  }
  return layoutDebugHudText(frame.projections.debug->runtimeDebugHudLines,
                            frame.viewport.width, frame.viewport.height);
}

std::vector<OverlayRect> uiOverlayRectsFor(const FrameInput& frame) {
  std::vector<OverlayRect> rects;
  // branch-gate: BG-1078
  if (frame.ui.rects == nullptr || frame.ui.rectCount == 0U) {
    return rects;
  }
  rects.reserve(frame.ui.rectCount);
  for (std::size_t index = 0; index < frame.ui.rectCount; ++index) {
    const RenderUiRect& ui = frame.ui.rects[index];
    rects.push_back(
        {ui.x, ui.y, ui.width, ui.height, ui.r, ui.g, ui.b, ui.a});
  }
  return rects;
}

}  // namespace

FirstRoomPushConstants renderLoopPushConstantsFromMat4(const Mat4& matrix) {
  FirstRoomPushConstants constants;
  for (std::uint32_t row = 0; row < 4U; ++row) {
    for (std::uint32_t column = 0; column < 4U; ++column) {
      constants.clipFromModel[static_cast<std::size_t>(column) * 4U + row] =
          iggy3d::at(matrix, row, column);
    }
  }
  return constants;
}

RenderLoopFramePlan buildRenderLoopFramePlan(
    const RenderLoopCreateInfo& createInfo, const FrameInput& frame,
    bool drawPackageRoom, bool firstRoomReady) {
  RenderLoopFramePlan plan;
  plan.debugHud = debugHudLayoutFor(frame);
  plan.projectileOverlay = projectileOverlayLayoutFor(frame);
  plan.uiOverlayRects = uiOverlayRectsFor(frame);
  plan.creativePreviewDrawCount =
      std::min<std::size_t>(frame.creativePreview.itemCount,
                            plan.creativePreviewDraws.size());
  for (std::size_t index = 0; index < plan.creativePreviewDrawCount; ++index) {
    const RenderCreativePreviewItem& item = frame.creativePreview.items[index];
    plan.creativePreviewDraws[index].geometryDrawIndex =
        createInfo.firstRoomResources != nullptr
            ? resolveCreativePreviewGeometryDrawIndex(
                  createInfo.firstRoomResources->creativePreviewGeometry(),
                  item)
            : creativePreviewGeometryDrawIndex(item.role,
                                               item.includePathWireframe,
                                               item.geometryProfile,
                                               item.proceduralSegmentCount);
    plan.creativePreviewDraws[index].depthDisabled =
        item.role == RenderCreativePreviewRole::Held;
    plan.creativePreviewDraws[index].pushConstants =
        renderLoopPushConstantsFromMat4(item.clipFromModel);
  }

  const bool drawSceneContent = sceneHasRenderableContent(frame);
  plan.drawPackageRoom = drawPackageRoom;
  plan.drawUiFrame = frameHasUiContent(frame) && !drawSceneContent;
  plan.drawProxyPrimitives =
      drawSceneContent && !drawPackageRoom &&
      frame.camera.mode == RenderCameraMode::FirstPerson;
  plan.drawFirstRoom = drawSceneContent && !drawPackageRoom &&
                       !plan.drawProxyPrimitives && firstRoomReady;
  plan.proxyFacts = proxySceneFacts(frame);
  return plan;
}

std::uint32_t renderLoopProxyDrawCount(
    const RenderLoopProxySceneFacts& facts) noexcept {
  return 3U + (facts.targetMarkerVisible ? 1U : 0U) +
         (facts.objectiveMarkerVisible ? 1U : 0U);
}

}  // namespace iggy3d::vulkan
