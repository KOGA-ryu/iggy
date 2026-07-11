#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "app/iggy3d/creative/input/Catalog.hpp"
#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d {

class SdlWindow;

}  // namespace iggy3d

namespace iggy3d::creative {

struct CreativeAppState;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorState;

struct CreativeEditorCatalogState {
  iggy3d::creative::CreativeCatalogState model;
  iggy3d::creative::CreativeToolWheelState toolWheel;
  iggy3d::creative::CreativeCatalogShapeSelection shapeSelection{};
  std::size_t scrollOffset = 0;
  std::size_t actionScrollOffset = 0;
};

struct CreativeEditorCatalogFrameRequest {
  iggy3d::SdlWindow& window;
  iggy3d::creative::CreativeAppState& appState;
  CreativeEditorState& editor;
  const iggy3d::creative::CreativeInputRouteResult& routedInput;
  const iggy3d::creative::CreativeWorldActionFrame& worldActions;
  float toolWheelDirectionX = 0.0F;
  float toolWheelDirectionY = 0.0F;
  std::uint32_t drawableWidth = 0;
  std::uint32_t drawableHeight = 0;
};

struct CreativeEditorCatalogFrameResult {
  bool blockWorldActions = false;
  bool openChanged = false;
  bool pageChanged = false;
  bool assigned = false;
  bool openToolOptionsRequested = false;
  iggy3d::creative::CreativeInputRouteResult deferredCommandInput;
  iggy3d::creative::CreativeHeldItemKind toolOptionsHeldItem =
      iggy3d::creative::CreativeHeldItemKind::Material;
};

[[nodiscard]] CreativeEditorCatalogFrameResult
processCreativeEditorCatalogFrame(
    const CreativeEditorCatalogFrameRequest& request);

void appendCreativeEditorCatalogOverlay(
    const iggy3d::creative::CreativeAppState& appState,
    const CreativeEditorState& editor,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs);

}  // namespace iggy3d_creative_app
