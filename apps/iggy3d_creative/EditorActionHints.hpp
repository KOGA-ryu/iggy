#pragma once

#include "app/iggy3d/creative/input/ActionHints.hpp"
#include "app/iggy3d/creative/ui/UiWidgets.hpp"
#include "render/FrameInput.hpp"

#include <cstdint>
#include <vector>

namespace iggy3d_creative_app {

struct CreativeEditorState;

[[nodiscard]] iggy3d::creative::CreativeActionHintFrame
resolveCreativeEditorActionHints(
    const CreativeEditorState& editor,
    iggy3d::creative::CreativeInputContext inputContext,
    iggy3d::creative::CreativeControlDevice activeDevice,
    bool captureMode,
    bool controllerCommandLayerActive = false) noexcept;

[[nodiscard]] iggy3d::creative::CreativeUiWidgetFrame
buildCreativeEditorActionHintWidgetFrame(
    const iggy3d::creative::CreativeActionHintFrame& hints,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight) noexcept;

void appendCreativeEditorActionHintsOverlay(
    const CreativeEditorState& editor,
    iggy3d::creative::CreativeInputContext inputContext,
    iggy3d::creative::CreativeControlDevice activeDevice,
    bool captureMode,
    bool controllerCommandLayerActive,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs);

}  // namespace iggy3d_creative_app
