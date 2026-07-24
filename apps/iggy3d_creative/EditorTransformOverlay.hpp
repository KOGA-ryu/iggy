#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "EditorTransformState.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d_creative_app {

[[nodiscard]] std::size_t appendCreativeEditorSelectionTransformPreview(
    const CreativeEditorSelectionTransformState& state,
    float wireThickness,
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& wireLines);

void appendCreativeEditorTransformOverlay(
    const CreativeEditorSelectionTransformState& state,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs);

}  // namespace iggy3d_creative_app
