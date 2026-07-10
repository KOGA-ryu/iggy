#include "app/iggy3d/creative/render/CreativeOverlayFrame.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"
#include "core/math/Mat4.hpp"
#include "render/debug/DebugHudText.hpp"

namespace iggy3d {
namespace {

float scaleXFor(const CreativeUiOverlayFrameRequest& request) {
  return request.drawList == nullptr || request.drawList->virtualWidth == 0U
             ? 1.0F
             : static_cast<float>(request.drawableWidth) /
                   static_cast<float>(request.drawList->virtualWidth);
}

float scaleYFor(const CreativeUiOverlayFrameRequest& request) {
  return request.drawList == nullptr || request.drawList->virtualHeight == 0U
             ? 1.0F
             : static_cast<float>(request.drawableHeight) /
                   static_cast<float>(request.drawList->virtualHeight);
}

std::int32_t scaledOffset(float value, float scale) {
  return static_cast<std::int32_t>(std::lround(value * scale));
}

std::uint32_t scaledExtent(float value, float scale, std::uint32_t limit) {
  if (value <= 0.0F || scale <= 0.0F || limit == 0U) {
    return 0U;
  }
  return std::clamp(static_cast<std::uint32_t>(std::lround(value * scale)), 1U,
                    limit);
}

CreativeUiColor colorFor(const CreativeUiPrimitive& primitive,
                         const CreativeUiOverlayFrameRequest& request) {
  const CreativeUiThemeId themeId =
      request.drawList == nullptr ? CreativeUiThemeId::System
                                  : request.drawList->theme;
  return creativeUiToneColor(primitive.tone, creativeUiTheme(themeId));
}

RenderUiRect renderRectFor(const CreativeUiPrimitive& primitive,
                           const CreativeUiOverlayFrameRequest& request,
                           float scaleX,
                           float scaleY) {
  const CreativeUiColor color = colorFor(primitive, request);
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

void appendText(CreativeUiOverlayFrame& frame,
                const CreativeUiPrimitive& primitive,
                const CreativeUiOverlayFrameRequest& request,
                float scaleX,
                float scaleY) {
  const CreativeUiColor color = colorFor(primitive, request);
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
  frame.textGlyphQuads.insert(frame.textGlyphQuads.end(), layout.quads.begin(),
                              layout.quads.end());
}

FrameInput baseFrame(const CreativeUiOverlayFrameRequest& request) {
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

RenderCreativeWireframeDebugLine renderLineFor(
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

CreativeUiOverlayFrame buildCreativeUiOverlayFrame(
    const CreativeUiOverlayFrameRequest& request) {
  CreativeUiOverlayFrame frame;
  if (request.drawList == nullptr || !request.drawList->ready ||
      request.drawableWidth == 0U || request.drawableHeight == 0U) {
    return frame;
  }

  const float scaleX = scaleXFor(request);
  const float scaleY = scaleYFor(request);
  frame.rects.reserve(request.drawList->rectCount);
  for (const CreativeUiPrimitive& primitive : request.drawList->primitives) {
    if (primitive.kind == CreativeUiPrimitiveKind::Text) {
      if (!primitive.text.empty()) {
        appendText(frame, primitive, request, scaleX, scaleY);
      }
      continue;
    }
    const RenderUiRect rect = renderRectFor(primitive, request, scaleX, scaleY);
    if (rect.width > 0U && rect.height > 0U) {
      frame.rects.push_back(rect);
    }
  }

  frame.frame = baseFrame(request);
  frame.frame.ui.visible = true;
  frame.frame.ui.rectCount = frame.rects.size();
  frame.frame.ui.textGlyphQuadCount = frame.textGlyphQuads.size();
  frame.frame.ui.textGlyphCount = frame.textGlyphCount;
  frame.frame.ui.primitiveCount = request.drawList->primitiveCount;
  frame.ready = !frame.rects.empty() || !frame.textGlyphQuads.empty();
  return frame;
}

CreativeWireframeDebugRenderFrame buildCreativeWireframeDebugRenderFrame(
    const ProductCreativeWireframeDebugLineList* lineList) {
  CreativeWireframeDebugRenderFrame renderFrame;
  if (lineList == nullptr) {
    return renderFrame;
  }
  renderFrame.frame.available = true;
  renderFrame.lines.reserve(lineList->lines.size());
  for (const ProductCreativeWireframeDebugLine& line : lineList->lines) {
    renderFrame.lines.push_back(renderLineFor(line));
  }
  renderFrame.frame.visible = !renderFrame.lines.empty();
  renderFrame.frame.lines = renderFrame.lines.data();
  renderFrame.frame.lineCount = renderFrame.lines.size();
  return renderFrame;
}

}  // namespace iggy3d
