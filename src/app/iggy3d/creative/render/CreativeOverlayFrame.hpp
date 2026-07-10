#pragma once

#include <cstdint>
#include <vector>

#include "app/iggy3d/creative/ui/CreativeUiDrawTypes.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d {

struct ProductCreativeWireframeDebugLineList;

struct CreativeUiOverlayFrameRequest {
  const CreativeUiDrawList* drawList = nullptr;
  std::uint64_t frameIndex = 0;
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
};

struct CreativeUiOverlayFrame {
  bool ready = false;
  FrameInput frame;
  std::vector<RenderUiRect> rects;
  std::vector<DebugHudGlyphQuad> textGlyphQuads;
  std::uint64_t textGlyphCount = 0;
};

struct CreativeWireframeDebugRenderFrame {
  std::vector<RenderCreativeWireframeDebugLine> lines;
  RenderCreativeWireframeDebugFrame frame;
};

CreativeUiOverlayFrame buildCreativeUiOverlayFrame(
    const CreativeUiOverlayFrameRequest& request);
CreativeWireframeDebugRenderFrame buildCreativeWireframeDebugRenderFrame(
    const ProductCreativeWireframeDebugLineList* lineList);

}  // namespace iggy3d
