#include "app/iggy3d/window/FramePresenter.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "app/iggy3d/menu/DrawList.hpp"
#include "app/iggy3d/view/OpeningMenuView.hpp"
#include "render/FrameInput.hpp"
#include "render/debug/DebugHudText.hpp"

namespace iggy3d {

namespace {

bool drawableReady(const ProductVulkanMenuFrameRequest& request) {
  return request.drawableWidth > 0U && request.drawableHeight > 0U;
}

float scaleXFor(const ProductVulkanMenuFrameRequest& request) {
  // branch-gate: BG-1079
  return request.uiDrawList == nullptr || request.uiDrawList->virtualWidth == 0U
             ? 1.0F
             : static_cast<float>(request.drawableWidth) /
                   static_cast<float>(request.uiDrawList->virtualWidth);
}

float scaleYFor(const ProductVulkanMenuFrameRequest& request) {
  // branch-gate: BG-1079
  return request.uiDrawList == nullptr || request.uiDrawList->virtualHeight == 0U
             ? 1.0F
             : static_cast<float>(request.drawableHeight) /
                   static_cast<float>(request.uiDrawList->virtualHeight);
}

std::int32_t scaledOffset(float value, float scale) {
  return static_cast<std::int32_t>(std::lround(value * scale));
}

std::uint32_t scaledExtent(float value, float scale, std::uint32_t limit) {
  // branch-gate: BG-1079
  if (value <= 0.0F || scale <= 0.0F || limit == 0U) {
    return 0U;
  }
  return std::clamp(static_cast<std::uint32_t>(std::lround(value * scale)), 1U,
                    limit);
}

RenderUiRect renderRectFor(const ProductUiPrimitive& primitive,
                           const ProductVulkanMenuFrameRequest& request,
                           float scaleX,
                           float scaleY) {
  const ProductUiColor color = productUiToneColor(primitive.tone);
  RenderUiRect rect;
  rect.x = scaledOffset(primitive.rect.x, scaleX);
  rect.y = scaledOffset(primitive.rect.y, scaleY);
  rect.width = scaledExtent(primitive.rect.width, scaleX, request.drawableWidth);
  rect.height =
      scaledExtent(primitive.rect.height, scaleY, request.drawableHeight);
  rect.r = color.r;
  rect.g = color.g;
  rect.b = color.b;
  rect.a = color.a;
  return rect;
}

void appendTextQuads(ProductVulkanMenuFrame& frame,
                     const ProductUiPrimitive& primitive,
                     const ProductVulkanMenuFrameRequest& request,
                     float scaleX,
                     float scaleY) {
  const DebugHudLayoutResult layout = layoutDebugHudTextAt(
      primitive.text, scaledOffset(primitive.rect.x, scaleX),
      scaledOffset(primitive.rect.y, scaleY), request.drawableWidth,
      request.drawableHeight);
  frame.textGlyphCount += layout.glyphCount;
  frame.textGlyphQuads.insert(frame.textGlyphQuads.end(),
                              layout.quads.begin(), layout.quads.end());
}

void appendPrimitive(ProductVulkanMenuFrame& frame,
                     const ProductUiPrimitive& primitive,
                     const ProductVulkanMenuFrameRequest& request,
                     float scaleX,
                     float scaleY) {
  // branch-gate: BG-1079
  if (primitive.kind == ProductUiPrimitiveKind::Text) {
    // branch-gate: BG-1079
    if (!primitive.text.empty()) {
      appendTextQuads(frame, primitive, request, scaleX, scaleY);
    }
    return;
  }
  const RenderUiRect rect = renderRectFor(primitive, request, scaleX, scaleY);
  // branch-gate: BG-1079
  if (rect.width > 0U && rect.height > 0U) {
    frame.rects.push_back(rect);
  }
}

FrameInput starterMenuFrameInput(const ProductVulkanMenuFrameRequest& request) {
  FrameInput frame;
  frame.viewport = {request.drawableWidth,
                    request.drawableHeight,
                    static_cast<float>(request.drawableWidth) /
                        static_cast<float>(request.drawableHeight)};
  frame.clock = {0U, request.frameIndex, 0.0F, 0.0F};
  frame.camera.mode = RenderCameraMode::ThirdPerson;
  frame.camera.worldEye = {0.0F, 1.0F, 1.0F};
  frame.camera.worldForward = {0.0F, 0.0F, -1.0F};
  frame.camera.worldUp = {0.0F, 1.0F, 0.0F};
  frame.camera.viewFromWorld = identityMat4();
  frame.camera.clipFromView = identityMat4();
  frame.camera.clipFromWorld = identityMat4();
  frame.camera.nearPlane = 0.1F;
  frame.camera.farPlane = 200.0F;
  return frame;
}

void applyProductWindowProjectionMetrics(
    ProductAppWindowState& window,
    const ProductGameplayProjectionFrame& projectionFrame,
    bool viewVisible) {
  applyGameplayProjectionMetrics(window,
                                 projectionFrame.scenePtr(),
                                 projectionFrame.debugPtr(),
                                 projectionFrame.drawListPtr(),
                                 projectionFrame.viewportFramePtr(),
                                 projectionFrame.renderBridgePtr(),
                                 viewVisible);
}

void presentProductVulkanFrame(ProductWindowFramePresenterRequest request) {
  // branch-gate: BG-1030
  if (request.projectionFrame.scenePtr() != nullptr &&
      request.projectionFrame.debugPtr() != nullptr &&
      request.projectionFrame.scene.room.loaded) {
    const SdlDrawableExtent drawableExtent = request.sdlWindow.drawableExtent();
    // branch-gate: BG-1030
    if (drawableExtent.width > 0U && drawableExtent.height > 0U) {
      const FrameInput renderFrame = makeProductVulkanFrame(
          request.projectionFrame.scene, request.projectionFrame.debug,
          request.window.framesPresented + 1U, drawableExtent.width,
          drawableExtent.height, request.window.viewport.cameraYawDegrees,
          request.window.viewport.cameraPitchDegrees);
      const RenderSubmitResult submit =
          request.renderer.vulkanRenderer.submitFrame(renderFrame);
      recordProductVulkanSubmit(request.window, submit);
    } else {
      request.window.productVulkanStatus = "frame_not_submitted";
      request.window.productVulkanReasonCode = "frame_not_drawable";
    }
  } else {
    request.window.productVulkanStatus = "waiting_for_gameplay_room";
    request.window.productVulkanReasonCode =
        "product_vulkan_waiting_for_gameplay_room";
    // branch-gate: BG-1072
    if (request.frontend.screen == FrontendScreen::Starter) {
      const ProductUiDrawList menuUi = buildProductStarterUiDrawList(
          {&request.frontend, request.saves.slots.compatibleCount, 1280U, 720U});
      recordProductVulkanMenuUiDrawList(request.window, "starter", menuUi);
      const SdlDrawableExtent drawableExtent = request.sdlWindow.drawableExtent();
      // branch-gate: BG-1072
      if (drawableExtent.width > 0U && drawableExtent.height > 0U && menuUi.ready) {
        ProductVulkanMenuFrame menuFrame = buildProductVulkanStarterMenuFrame(
            {&menuUi,
             request.window.framesPresented + 1U,
             drawableExtent.width,
             drawableExtent.height});
        // branch-gate: BG-1072
        if (menuFrame.ready) {
          const RenderSubmitResult submit = request.renderer.vulkanRenderer.submitFrame(
              refreshProductVulkanMenuFrameInput(menuFrame));
          recordProductVulkanSubmit(request.window, submit);
        } else {
          request.window.productVulkanStatus = "frame_not_submitted";
          request.window.productVulkanReasonCode = menuFrame.reasonCode;
        }
      }
    }
  }
}

void presentProductSdlFrame(ProductWindowFramePresenterRequest request) {
  const OpeningMenuViewState view =
      drawOpeningMenuView(*request.renderer.sdlRenderer,
                          request.options,
                          request.world,
                          request.frontend,
                          request.settingsTab,
                          request.worldSetupDraft,
                          request.window.worldSetupDungeonDraftEditMode,
                          request.window.worldSetupDungeonDraftModified,
                          request.window.worldSetupDungeonDraftCursorRow,
                          request.window.worldSetupDungeonDraftCursorColumn,
                          request.window.gameplayActive,
                          request.window.runtimeStateHash,
                          request.projectionFrame.viewportFramePtr(),
                          &request.projectionFrame.feedback,
                          &request.projectionFrame.interactionModeHud,
                          &request.projectionFrame.topDownMapOverlay,
                          &request.projectionFrame.movementHud,
                          &request.projectionFrame.npcBehaviorHud,
                          &request.projectionFrame.physicsHud,
                          &request.projectionFrame.roomEditorHud,
                          request.projectionFrame.sceneItemCount,
                          request.projectionFrame.debugPtr(),
                          request.window.viewport.cameraYawDegrees,
                          request.window.viewport.cameraPitchDegrees,
                          request.saves);
  request.window.viewport.cameraHeadingVisible =
      request.window.viewport.cameraHeadingVisible || view.cameraHeadingDrawn;
  request.window.menuTextDrawn = request.window.menuTextDrawn || view.textDrawn;
  request.window.selectedRowDrawn =
      request.window.selectedRowDrawn || view.selectedRowDrawn;
  request.window.menuRowCount = view.rowCount;
}

}  // namespace

ProductVulkanMenuFrame buildProductVulkanStarterMenuFrame(
    const ProductVulkanMenuFrameRequest& request) {
  ProductVulkanMenuFrame frame;
  // branch-gate: BG-1079
  if (request.uiDrawList == nullptr) {
    frame.reasonCode = "product_vulkan_menu_frame_missing_ui";
    return frame;
  }
  // branch-gate: BG-1079
  if (!request.uiDrawList->ready) {
    frame.reasonCode = request.uiDrawList->reasonCode;
    return frame;
  }
  // branch-gate: BG-1079
  if (!drawableReady(request)) {
    frame.reasonCode = "product_vulkan_menu_frame_not_drawable";
    return frame;
  }

  const float scaleX = scaleXFor(request);
  const float scaleY = scaleYFor(request);
  frame.rects.reserve(request.uiDrawList->rectCount);
  for (const ProductUiPrimitive& primitive : request.uiDrawList->primitives) {
    appendPrimitive(frame, primitive, request, scaleX, scaleY);
  }

  frame.frame = starterMenuFrameInput(request);
  frame.frame.ui.visible = true;
  frame.frame.ui.rectCount = frame.rects.size();
  frame.frame.ui.textGlyphQuadCount = frame.textGlyphQuads.size();
  frame.frame.ui.textGlyphCount = frame.textGlyphCount;
  frame.frame.ui.primitiveCount = request.uiDrawList->primitiveCount;
  frame.ready = !frame.rects.empty() || !frame.textGlyphQuads.empty();
  // branch-gate: BG-1079
  frame.status = frame.ready ? "product_vulkan_menu_frame_ready"
                             : "product_vulkan_menu_frame_empty";
  frame.reasonCode = frame.status;
  return frame;
}

const FrameInput& refreshProductVulkanMenuFrameInput(
    ProductVulkanMenuFrame& menuFrame) {
  menuFrame.frame.ui.rects = menuFrame.rects.data();
  menuFrame.frame.ui.rectCount = menuFrame.rects.size();
  menuFrame.frame.ui.textGlyphQuads = menuFrame.textGlyphQuads.data();
  menuFrame.frame.ui.textGlyphQuadCount = menuFrame.textGlyphQuads.size();
  menuFrame.frame.ui.textGlyphCount = menuFrame.textGlyphCount;
  return menuFrame.frame;
}

void presentProductWindowFrame(ProductWindowFramePresenterRequest request) {
  // branch-gate: BG-1030
  if (request.window.drawable) {
    applyProductWindowProjectionMetrics(request.window,
                                        request.projectionFrame,
                                        request.projectionFrame.viewVisible);
    // branch-gate: BG-1030
    if (request.renderer.useVulkanRenderer) {
      presentProductVulkanFrame(request);
    } else {
      presentProductSdlFrame(request);
    }
  } else {
    applyProductWindowProjectionMetrics(request.window, request.projectionFrame, false);
  }
}

}  // namespace iggy3d
