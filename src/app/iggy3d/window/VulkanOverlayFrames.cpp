#include "app/iggy3d/window/FramePresenter.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>

#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"
#include "render/debug/DebugHudText.hpp"

namespace iggy3d {

// branch-gate-relocation: BG-1229 from=src/app/iggy3d/window/FramePresenter.cpp

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
  // Resolve the tone through the theme the draw list was built with — System for
  // the starter/system shell, Journal (Moleskine) for the in-game diegetic
  // surfaces. Defaults to System when no draw list is attached.
  const ProductUiThemeId themeId = request.uiDrawList != nullptr
                                       ? request.uiDrawList->theme
                                       : ProductUiThemeId::System;
  const ProductUiColor color =
      productUiToneColor(primitive.tone, productUiTheme(themeId));
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
  // UI text resolves its colour through the same theme as rects, so glyphs match
  // their tone (dark graphite ink on the cream Journal page, light on the dark
  // System shell) instead of the fixed debug-HUD mint.
  const ProductUiThemeId themeId = request.uiDrawList != nullptr
                                       ? request.uiDrawList->theme
                                       : ProductUiThemeId::System;
  const ProductUiColor color =
      productUiToneColor(primitive.tone, productUiTheme(themeId));
  DebugHudLayoutResult layout = layoutDebugHudTextAt(
      primitive.text, scaledOffset(primitive.rect.x, scaleX),
      scaledOffset(primitive.rect.y, scaleY), request.drawableWidth,
      request.drawableHeight);
  for (DebugHudGlyphQuad& quad : layout.quads) {
    quad.r = color.r;
    quad.g = color.g;
    quad.b = color.b;
    quad.a = color.a;
  }
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

RenderCreativeWireframeDebugLine renderCreativeWireframeDebugLineFor(
    const ProductCreativeWireframeDebugLine& line) {
  RenderCreativeWireframeDebugLine renderLine;
  renderLine.start = line.start;
  renderLine.end = line.end;
  renderLine.color = {line.color.r, line.color.g, line.color.b, line.color.a};
  renderLine.objectId = static_cast<std::uint64_t>(line.objectId);
  renderLine.objectKind = static_cast<std::uint32_t>(line.objectKind);
  renderLine.style = static_cast<std::uint32_t>(line.style);
  renderLine.segmentKind = static_cast<std::uint32_t>(line.segmentKind);
  renderLine.thickness = line.thickness;
  return renderLine;
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

ProductCreativeWireframeDebugRenderFrame buildProductCreativeWireframeDebugRenderFrame(
    const ProductCreativeWireframeDebugLineList* lineList) {
  ProductCreativeWireframeDebugRenderFrame renderFrame;
  if (lineList == nullptr) {
    return renderFrame;
  }

  renderFrame.frame.available = true;
  renderFrame.lines.reserve(lineList->lines.size());
  for (const ProductCreativeWireframeDebugLine& line : lineList->lines) {
    renderFrame.lines.push_back(renderCreativeWireframeDebugLineFor(line));
  }

  renderFrame.frame.visible = !renderFrame.lines.empty();
  renderFrame.frame.lines = renderFrame.lines.data();
  renderFrame.frame.lineCount = renderFrame.lines.size();
  return renderFrame;
}

void appendProductUiOverlay(ProductVulkanGameplayFrame& gameplayFrame,
                            const ProductUiDrawList& overlayUi,
                            std::uint64_t frameIndex,
                            std::uint32_t drawableWidth,
                            std::uint32_t drawableHeight) {
  // branch-gate: BG-1030
  if (!overlayUi.ready) {
    return;
  }
  // Convert the overlay draw list to rects + glyphs with the SAME machinery the
  // starter menu uses — renderRectFor resolves the draw list's theme (Journal),
  // and scaling uses the overlay's own virtual dimensions.
  const ProductVulkanMenuFrame overlayFrame = buildProductVulkanStarterMenuFrame(
      {&overlayUi, frameIndex, drawableWidth, drawableHeight});
  // branch-gate: BG-1030
  if (!overlayFrame.ready) {
    return;
  }
  gameplayFrame.rects.reserve(gameplayFrame.rects.size() +
                              overlayFrame.rects.size());
  gameplayFrame.textGlyphQuads.reserve(gameplayFrame.textGlyphQuads.size() +
                                       overlayFrame.textGlyphQuads.size());
  gameplayFrame.rects.insert(gameplayFrame.rects.end(),
                             overlayFrame.rects.begin(),
                             overlayFrame.rects.end());
  gameplayFrame.textGlyphQuads.insert(gameplayFrame.textGlyphQuads.end(),
                                      overlayFrame.textGlyphQuads.begin(),
                                      overlayFrame.textGlyphQuads.end());
  gameplayFrame.textGlyphCount += overlayFrame.textGlyphCount;
}

void appendCreativeUiOverlay(ProductVulkanGameplayFrame& gameplayFrame,
                             const ProductUiDrawList& overlayUi,
                             std::uint64_t frameIndex,
                             std::uint32_t drawableWidth,
                             std::uint32_t drawableHeight) {
  appendProductUiOverlay(gameplayFrame,
                         overlayUi,
                         frameIndex,
                         drawableWidth,
                         drawableHeight);
}

void appendPauseMenuOverlay(ProductVulkanGameplayFrame& gameplayFrame,
                            const ProductUiDrawList& overlayUi,
                            std::uint64_t frameIndex,
                            std::uint32_t drawableWidth,
                            std::uint32_t drawableHeight) {
  appendProductUiOverlay(gameplayFrame,
                         overlayUi,
                         frameIndex,
                         drawableWidth,
                         drawableHeight);
}

}  // namespace iggy3d
