#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "app/iggy3d/creative/ui/UiWidgets.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d {

struct ProductCreativeWireframeDebugLineList;

struct CreativeWireframeDebugRenderFrame {
  std::vector<RenderCreativeWireframeDebugLine> lines;
  RenderCreativeWireframeDebugFrame frame;
};

struct CreativeUiWidgetOverlayAppendReceipt {
  bool accepted = false;
  bool ready = false;
  bool partial = false;
  std::size_t rectCount = 0U;
  std::size_t glyphQuadCount = 0U;
  std::uint64_t textGlyphCount = 0U;
};

[[nodiscard]] CreativeUiWidgetOverlayAppendReceipt
appendCreativeUiWidgetOverlay(
    const creative::CreativeUiWidgetFrame& widgetFrame,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    std::vector<RenderUiRect>& rects,
    std::vector<DebugHudGlyphQuad>& glyphs);
CreativeWireframeDebugRenderFrame buildCreativeWireframeDebugRenderFrame(
    const ProductCreativeWireframeDebugLineList* lineList);

}  // namespace iggy3d
