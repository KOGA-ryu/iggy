#include "app/iggy3d/creative/render/CreativeOverlayFrame.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "app/iggy3d/creative/render/WireframeDebugLines.hpp"
#include "render/debug/DebugHudText.hpp"

namespace iggy3d {
namespace {

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

float widgetScale(std::uint32_t drawableExtent,
                  std::uint32_t virtualExtent) noexcept {
  return virtualExtent == 0U
             ? 1.0F
             : static_cast<float>(drawableExtent) /
                   static_cast<float>(virtualExtent);
}

CreativeUiColor widgetVisualColor(
    const creative::CreativeUiWidgetVisual& visual) {
  CreativeUiColor color =
      creativeUiToneColor(visual.tone, creativeUiTheme());
  color.a *= visual.opacity;
  return color;
}

}  // namespace

CreativeUiWidgetOverlayAppendReceipt appendCreativeUiWidgetOverlay(
    const creative::CreativeUiWidgetFrame& widgetFrame,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    std::vector<RenderUiRect>& rects,
    std::vector<DebugHudGlyphQuad>& glyphs) {
  CreativeUiWidgetOverlayAppendReceipt receipt;
  if (widgetFrame.invalidInput || widgetFrame.capacityExceeded ||
      drawableWidth == 0U || drawableHeight == 0U) {
    return receipt;
  }
  receipt.accepted = true;
  receipt.partial = widgetFrame.textTruncated;
  const std::size_t firstRect = rects.size();
  const std::size_t firstGlyph = glyphs.size();
  const float scaleX = widgetScale(drawableWidth, widgetFrame.virtualWidth);
  const float scaleY = widgetScale(drawableHeight, widgetFrame.virtualHeight);
  for (const creative::CreativeUiWidgetVisual& visual :
       widgetFrame.visualItems()) {
    const CreativeUiColor color = widgetVisualColor(visual);
    if (visual.kind == creative::CreativeUiWidgetVisualKind::Text) {
      if (visual.text.length == 0U) {
        continue;
      }
      DebugHudLayoutResult layout = layoutDebugHudTextAt(
          visual.text.view(), scaledOffset(visual.rect.x, scaleX),
          scaledOffset(visual.rect.y, scaleY), drawableWidth, drawableHeight);
      for (DebugHudGlyphQuad& quad : layout.quads) {
        quad.r = color.r;
        quad.g = color.g;
        quad.b = color.b;
        quad.a = color.a;
      }
      receipt.textGlyphCount += layout.glyphCount;
      glyphs.insert(glyphs.end(), layout.quads.begin(), layout.quads.end());
      continue;
    }
    if (visual.kind != creative::CreativeUiWidgetVisualKind::Rect) {
      continue;
    }
    RenderUiRect rect;
    rect.x = scaledOffset(visual.rect.x, scaleX);
    rect.y = scaledOffset(visual.rect.y, scaleY);
    rect.width = scaledExtent(visual.rect.width, scaleX, drawableWidth);
    rect.height = scaledExtent(visual.rect.height, scaleY, drawableHeight);
    rect.r = color.r;
    rect.g = color.g;
    rect.b = color.b;
    rect.a = color.a;
    if (rect.width > 0U && rect.height > 0U) {
      rects.push_back(rect);
    }
  }
  receipt.rectCount = rects.size() - firstRect;
  receipt.glyphQuadCount = glyphs.size() - firstGlyph;
  receipt.ready = receipt.rectCount > 0U || receipt.glyphQuadCount > 0U;
  return receipt;
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
