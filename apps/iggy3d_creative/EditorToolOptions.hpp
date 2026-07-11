#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d {

class SdlWindow;

}  // namespace iggy3d

namespace iggy3d_creative_app {

struct CreativeEditorState;

struct CreativeEditorToolOptionsState {
  bool open = false;
  iggy3d::creative::CreativeHeldItemKind heldItem =
      iggy3d::creative::CreativeHeldItemKind::Material;
  iggy3d::creative::CreativeToolSettings draft;
  iggy3d::creative::CreativeToolOptionList options;
  std::size_t selectedIndex = 0;
};

struct CreativeEditorToolOptionsFrameRequest {
  iggy3d::SdlWindow& window;
  CreativeEditorState& editor;
  const iggy3d::creative::CreativeInputRouteResult& routedInput;
  bool openRequested = false;
  iggy3d::creative::CreativeHeldItemKind requestedHeldItem =
      iggy3d::creative::CreativeHeldItemKind::Material;
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
};

struct CreativeEditorToolOptionsFrameResult {
  bool blockWorldActions = false;
  bool openChanged = false;
  bool committed = false;
};

[[nodiscard]] CreativeEditorToolOptionsFrameResult
processCreativeEditorToolOptionsFrame(
    const CreativeEditorToolOptionsFrameRequest& request);

void appendCreativeEditorToolOptionsOverlay(
    const CreativeEditorState& editor,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs);

}  // namespace iggy3d_creative_app
