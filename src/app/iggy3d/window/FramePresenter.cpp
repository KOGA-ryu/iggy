#include "app/iggy3d/window/FramePresenter.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>

#include "app/frontend/DevToolsMenu.hpp"
#include "app/iggy3d/menu/DrawList.hpp"
#include "app/iggy3d/view/OpeningMenuView.hpp"
#include "render/FrameInput.hpp"
#include "render/debug/DebugHudText.hpp"

namespace iggy3d {

namespace {

constexpr float kVirtualViewportWidth = 1280.0F;
constexpr float kVirtualViewportHeight = 720.0F;

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

std::int32_t scaledHudOffset(float virtualValue,
                             std::uint32_t viewportExtent,
                             float virtualExtent) {
  return static_cast<std::int32_t>(
      std::lround(virtualValue * static_cast<float>(viewportExtent) / virtualExtent));
}

std::uint32_t scaledHudExtent(float virtualValue,
                              std::uint32_t viewportExtent,
                              float virtualExtent) {
  const long scaled = std::lround(
      virtualValue * static_cast<float>(viewportExtent) / virtualExtent);
  return static_cast<std::uint32_t>(std::max(1L, scaled));
}

std::string fixedHudFloat(float value, int precision) {
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(precision) << value;
  return stream.str();
}

void appendGameplayHudText(ProductVulkanGameplayFrame& frame,
                           std::string_view text,
                           float virtualX,
                           float virtualY,
                           std::uint32_t viewportWidth,
                           std::uint32_t viewportHeight) {
  const DebugHudLayoutResult layout = layoutDebugHudTextAt(
      text,
      scaledHudOffset(virtualX, viewportWidth, kVirtualViewportWidth),
      scaledHudOffset(virtualY, viewportHeight, kVirtualViewportHeight),
      viewportWidth,
      viewportHeight);
  frame.textGlyphCount += layout.glyphCount;
  frame.textGlyphQuads.insert(frame.textGlyphQuads.end(), layout.quads.begin(),
                              layout.quads.end());
}

void appendPositionHudUi(ProductVulkanGameplayFrame& frame,
                         const PositionHud& hud,
                         std::uint32_t viewportWidth,
                         std::uint32_t viewportHeight) {
  // branch-gate: BG-1196
  if (!hud.visible || viewportWidth == 0U || viewportHeight == 0U) {
    return;
  }

  frame.rects.push_back(RenderUiRect{
      scaledHudOffset(88.0F, viewportWidth, kVirtualViewportWidth),
      scaledHudOffset(572.0F, viewportHeight, kVirtualViewportHeight),
      scaledHudExtent(360.0F, viewportWidth, kVirtualViewportWidth),
      scaledHudExtent(54.0F, viewportHeight, kVirtualViewportHeight),
      14.0F / 255.0F,
      21.0F / 255.0F,
      23.0F / 255.0F,
      1.0F,
  });

  const std::int32_t textX =
      scaledHudOffset(100.0F, viewportWidth, kVirtualViewportWidth);
  std::int32_t textY =
      scaledHudOffset(582.0F, viewportHeight, kVirtualViewportHeight);
  const std::int32_t lineStep =
      scaledHudOffset(16.0F, viewportHeight, kVirtualViewportHeight);
  std::uint64_t drawn = 0U;
  for (const PositionHudLine& line : hud.lines) {
    // branch-gate: BG-1196
    if (!line.visible) {
      continue;
    }
    // branch-gate: BG-1196
    if (drawn >= 3U) {
      break;
    }
    const DebugHudLayoutResult layout =
        layoutDebugHudTextAt(line.text, textX, textY, viewportWidth, viewportHeight);
    frame.textGlyphCount += layout.glyphCount;
    frame.textGlyphQuads.insert(frame.textGlyphQuads.end(), layout.quads.begin(),
                                layout.quads.end());
    textY += lineStep;
    ++drawn;
  }
}

void appendMovementTuningHudUi(
    ProductVulkanGameplayFrame& frame,
    const ProductGameplayMovementTuning& tuning,
    ProductGameplayMovementTuningField selectedField,
    bool visible,
    std::uint32_t viewportWidth,
    std::uint32_t viewportHeight) {
  // branch-gate: BG-1213
  if (!visible || viewportWidth == 0U || viewportHeight == 0U) {
    return;
  }

  frame.rects.push_back(RenderUiRect{
      scaledHudOffset(820.0F, viewportWidth, kVirtualViewportWidth),
      scaledHudOffset(410.0F, viewportHeight, kVirtualViewportHeight),
      scaledHudExtent(384.0F, viewportWidth, kVirtualViewportWidth),
      scaledHudExtent(168.0F, viewportHeight, kVirtualViewportHeight),
      14.0F / 255.0F,
      21.0F / 255.0F,
      23.0F / 255.0F,
      1.0F,
  });

  appendGameplayHudText(frame,
                        "MOVEMENT TUNING",
                        838.0F,
                        426.0F,
                        viewportWidth,
                        viewportHeight);
  appendGameplayHudText(frame,
                        "F4 HIDE  ENTER/UP/DOWN FIELD",
                        838.0F,
                        456.0F,
                        viewportWidth,
                        viewportHeight);
  appendGameplayHudText(frame,
                        "LEFT/RIGHT VALUE",
                        838.0F,
                        472.0F,
                        viewportWidth,
                        viewportHeight);

  float virtualY = 498.0F;
  std::uint64_t drawn = 0U;
  for (const ProductGameplayMovementTuningFieldDescriptor& descriptor :
       kProductGameplayMovementTuningFields) {
    // branch-gate: BG-1213
    if (drawn >= 7U) {
      break;
    }
    const bool selected = descriptor.field == selectedField;
    // branch-gate: BG-1213
    if (selected) {
      frame.rects.push_back(RenderUiRect{
          scaledHudOffset(834.0F, viewportWidth, kVirtualViewportWidth),
          scaledHudOffset(virtualY - 2.0F, viewportHeight, kVirtualViewportHeight),
          scaledHudExtent(348.0F, viewportWidth, kVirtualViewportWidth),
          scaledHudExtent(14.0F, viewportHeight, kVirtualViewportHeight),
          63.0F / 255.0F,
          71.0F / 255.0F,
          58.0F / 255.0F,
          1.0F,
      });
    }
    std::string row = selected ? "> " : "  ";  // branch-gate: BG-1213
    row += descriptor.label;
    row += " ";
    row += fixedHudFloat(tuning.*(descriptor.value),
                         descriptor.step < 0.05F ? 2 : 1);  // branch-gate: BG-1213
    appendGameplayHudText(frame,
                          row,
                          838.0F,
                          virtualY,
                          viewportWidth,
                          viewportHeight);
    virtualY += 16.0F;
    ++drawn;
  }
}

void appendDevToolsOverlayUi(ProductVulkanGameplayFrame& frame,
                             bool visible,
                             FrontendDevToolsCategory selectedCategory,
                             std::uint32_t viewportWidth,
                             std::uint32_t viewportHeight) {
  // branch-gate: BG-1214
  if (!visible || viewportWidth == 0U || viewportHeight == 0U) {
    return;
  }

  frame.rects.push_back(RenderUiRect{
      scaledHudOffset(414.0F, viewportWidth, kVirtualViewportWidth),
      scaledHudOffset(100.0F, viewportHeight, kVirtualViewportHeight),
      scaledHudExtent(760.0F, viewportWidth, kVirtualViewportWidth),
      scaledHudExtent(438.0F, viewportHeight, kVirtualViewportHeight),
      14.0F / 255.0F,
      21.0F / 255.0F,
      23.0F / 255.0F,
      1.0F,
  });
  appendGameplayHudText(frame,
                        "DEV TOOLS",
                        450.0F,
                        128.0F,
                        viewportWidth,
                        viewportHeight);
  appendGameplayHudText(frame,
                        "F1 CLOSE  UP/DOWN CATEGORY",
                        452.0F,
                        185.0F,
                        viewportWidth,
                        viewportHeight);

  float virtualY = 230.0F;
  for (const FrontendDevToolsCategory category : devToolsCategoryOrder()) {
    const bool selected = category == selectedCategory;
    // branch-gate: BG-1214
    if (selected) {
      frame.rects.push_back(RenderUiRect{
          scaledHudOffset(444.0F, viewportWidth, kVirtualViewportWidth),
          scaledHudOffset(virtualY - 6.0F, viewportHeight, kVirtualViewportHeight),
          scaledHudExtent(320.0F, viewportWidth, kVirtualViewportWidth),
          scaledHudExtent(26.0F, viewportHeight, kVirtualViewportHeight),
          63.0F / 255.0F,
          71.0F / 255.0F,
          58.0F / 255.0F,
          1.0F,
      });
    }
    std::string row = selected ? "> " : "  ";  // branch-gate: BG-1214
    row += devToolsCategoryLabel(category);
    appendGameplayHudText(frame,
                          row,
                          458.0F,
                          virtualY,
                          viewportWidth,
                          viewportHeight);
    virtualY += 34.0F;
  }

  appendGameplayHudText(frame,
                        "READOUTS",
                        850.0F,
                        230.0F,
                        viewportWidth,
                        viewportHeight);
  appendGameplayHudText(frame,
                        "RUNTIME STATE",
                        850.0F,
                        272.0F,
                        viewportWidth,
                        viewportHeight);
  appendGameplayHudText(frame,
                        "INPUT OWNER",
                        850.0F,
                        304.0F,
                        viewportWidth,
                        viewportHeight);
  appendGameplayHudText(frame,
                        "RENDERER STATUS",
                        850.0F,
                        336.0F,
                        viewportWidth,
                        viewportHeight);
  appendGameplayHudText(frame,
                        "BACK",
                        850.0F,
                        394.0F,
                        viewportWidth,
                        viewportHeight);
}

void appendMapMakerCubePreviewUi(ProductVulkanGameplayFrame& frame,
                                  const ProductViewportFrame& viewportFrame,
                                  std::uint32_t viewportWidth,
                                  std::uint32_t viewportHeight) {
  // branch-gate: BG-1206
  if (viewportWidth == 0U || viewportHeight == 0U) {
    return;
  }
  for (const ProductViewportFramedItem& framed : viewportFrame.framedItems) {
    // branch-gate: BG-1206
    if (!framed.onScreen ||
        framed.item.kind != ProductPrimitiveDrawKind::MapMakerCubePreview) {
      continue;
    }
    const float size = std::clamp(framed.item.markerSize, 18.0F, 72.0F);
    frame.rects.push_back(RenderUiRect{
        scaledHudOffset(framed.screenX - size * 0.5F, viewportWidth,
                        kVirtualViewportWidth),
        scaledHudOffset(framed.screenY - size * 0.5F, viewportHeight,
                        kVirtualViewportHeight),
        scaledHudExtent(size, viewportWidth, kVirtualViewportWidth),
        scaledHudExtent(size, viewportHeight, kVirtualViewportHeight),
        static_cast<float>(framed.item.color.r) / 255.0F,
        static_cast<float>(framed.item.color.g) / 255.0F,
        static_cast<float>(framed.item.color.b) / 255.0F,
        1.0F,
    });
    // branch-gate: BG-1206
    if (size > 12.0F) {
      const float inset = 4.0F;
      frame.rects.push_back(RenderUiRect{
          scaledHudOffset(framed.screenX - size * 0.5F + inset, viewportWidth,
                          kVirtualViewportWidth),
          scaledHudOffset(framed.screenY - size * 0.5F + inset, viewportHeight,
                          kVirtualViewportHeight),
          scaledHudExtent(size - inset * 2.0F, viewportWidth, kVirtualViewportWidth),
          scaledHudExtent(size - inset * 2.0F, viewportHeight,
                          kVirtualViewportHeight),
          48.0F / 255.0F,
          65.0F / 255.0F,
          72.0F / 255.0F,
          0.92F,
      });
    }
  }
}

void appendMapMakerHudUi(ProductVulkanGameplayFrame& frame,
                         const ProductMapMakerHud& hud,
                         std::uint32_t viewportWidth,
                         std::uint32_t viewportHeight) {
  // branch-gate: BG-1205
  if (!hud.visible || viewportWidth == 0U || viewportHeight == 0U) {
    return;
  }
  frame.rects.push_back(RenderUiRect{
      scaledHudOffset(88.0F, viewportWidth, kVirtualViewportWidth),
      scaledHudOffset(636.0F, viewportHeight, kVirtualViewportHeight),
      scaledHudExtent(300.0F, viewportWidth, kVirtualViewportWidth),
      scaledHudExtent(26.0F, viewportHeight, kVirtualViewportHeight),
      14.0F / 255.0F,
      21.0F / 255.0F,
      23.0F / 255.0F,
      1.0F,
  });
  const std::int32_t textX =
      scaledHudOffset(100.0F, viewportWidth, kVirtualViewportWidth);
  std::int32_t textY =
      scaledHudOffset(644.0F, viewportHeight, kVirtualViewportHeight);
  for (const ProductMapMakerHudLine& line : hud.lines) {
    // branch-gate: BG-1205
    if (!line.visible) {
      continue;
    }
    const DebugHudLayoutResult layout =
        layoutDebugHudTextAt(line.text, textX, textY, viewportWidth, viewportHeight);
    frame.textGlyphCount += layout.glyphCount;
    frame.textGlyphQuads.insert(frame.textGlyphQuads.end(), layout.quads.begin(),
                                layout.quads.end());
    break;
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
      ProductVulkanGameplayFrame renderFrame = buildProductVulkanGameplayFrame(
          request.projectionFrame, request.window.framesPresented + 1U,
          drawableExtent.width, drawableExtent.height,
          request.window.viewport.cameraYawDegrees,
          request.window.viewport.cameraPitchDegrees,
          request.window.gameplayMovementTuning,
          request.window.gameplayMovementTuningSelectedField,
          request.window.gameplayMovementTuningVisible,
          request.frontend.screen == FrontendScreen::DevOverlay &&
              request.frontend.devToolsOpen,
          request.frontend.devToolsCategory);
      const RenderSubmitResult submit =
          request.renderer.vulkanRenderer.submitFrame(
              refreshProductVulkanGameplayFrameInput(renderFrame));
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
          {&request.frontend,
           request.saves.slots.compatibleCount,
           1280U,
           720U,
           &request.worldSetupDraft,
           request.window.worldSetupDungeonDraftEditMode,
           request.window.worldSetupDungeonDraftModified,
           request.window.worldSetupDungeonDraftCursorRow,
           request.window.worldSetupDungeonDraftCursorColumn,
           request.window.worldSetupDungeonDraftSelectedGlyph,
           request.window.worldSetupDungeonDraftLastGlyph});
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
	                          request.window.gameplayMovementTuning,
	                          request.window.gameplayMovementTuningSelectedField,
	                          request.window.gameplayMovementTuningVisible,
	                          request.worldSetupDraft,
                          request.window.worldSetupDungeonDraftEditMode,
                          request.window.worldSetupDungeonDraftModified,
                          request.window.worldSetupDungeonDraftCursorRow,
                          request.window.worldSetupDungeonDraftCursorColumn,
                          request.window.worldSetupDungeonDraftSelectedGlyph,
                          request.window.worldSetupDungeonDraftLastGlyph,
                          request.window.gameplayActive,
                          request.window.runtimeStateHash,
                          request.projectionFrame.viewportFramePtr(),
                          &request.projectionFrame.feedback,
                          &request.projectionFrame.interactionModeHud,
                          &request.projectionFrame.topDownMapOverlay,
                          &request.projectionFrame.movementHud,
                          &request.projectionFrame.npcBehaviorHud,
                          &request.projectionFrame.physicsHud,
                          &request.projectionFrame.positionHud,
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

ProductVulkanGameplayFrame buildProductVulkanGameplayFrame(
    const ProductGameplayProjectionFrame& projectionFrame,
    std::uint64_t frameIndex,
    std::uint32_t viewportWidth,
    std::uint32_t viewportHeight,
    float cameraYawDegrees,
    float cameraPitchDegrees,
    const ProductGameplayMovementTuning& movementTuning,
    ProductGameplayMovementTuningField movementTuningField,
    bool movementTuningVisible,
    bool devToolsOverlayVisible,
    FrontendDevToolsCategory devToolsCategory) {
  ProductVulkanGameplayFrame frame;
  frame.frame = makeProductVulkanFrame(projectionFrame.scene, projectionFrame.debug,
                                       frameIndex, viewportWidth, viewportHeight,
                                       cameraYawDegrees, cameraPitchDegrees,
                                       projectionFrame.cameraAnchorOverrideAvailable,
                                       projectionFrame.cameraAnchorOverrideMeters);
  appendMapMakerCubePreviewUi(frame, projectionFrame.viewportFrame, viewportWidth,
                              viewportHeight);
  appendMapMakerHudUi(frame, projectionFrame.mapMakerHud, viewportWidth,
                      viewportHeight);
  appendPositionHudUi(frame, projectionFrame.positionHud, viewportWidth, viewportHeight);
  appendMovementTuningHudUi(frame,
                            movementTuning,
                            movementTuningField,
                            movementTuningVisible,
                            viewportWidth,
                            viewportHeight);
  appendDevToolsOverlayUi(frame,
                          devToolsOverlayVisible,
                          devToolsCategory,
                          viewportWidth,
                          viewportHeight);
  return frame;
}

const FrameInput& refreshProductVulkanGameplayFrameInput(
    ProductVulkanGameplayFrame& gameplayFrame) {
  gameplayFrame.frame.ui.visible =
      !gameplayFrame.rects.empty() || !gameplayFrame.textGlyphQuads.empty();
  gameplayFrame.frame.ui.rects = gameplayFrame.rects.data();
  gameplayFrame.frame.ui.rectCount = gameplayFrame.rects.size();
  gameplayFrame.frame.ui.textGlyphQuads = gameplayFrame.textGlyphQuads.data();
  gameplayFrame.frame.ui.textGlyphQuadCount = gameplayFrame.textGlyphQuads.size();
  gameplayFrame.frame.ui.textGlyphCount = gameplayFrame.textGlyphCount;
  gameplayFrame.frame.ui.primitiveCount =
      gameplayFrame.rects.size() + gameplayFrame.textGlyphCount;
  return gameplayFrame.frame;
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
