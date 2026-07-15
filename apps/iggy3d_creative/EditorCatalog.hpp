#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
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

enum class CreativeEditorCatalogAssetAction : std::uint8_t {
  Equip,
  ReplaceSelection,
  ManageAsset,
  Count,
};

[[nodiscard]] CreativeEditorCatalogAssetAction
moveCreativeEditorCatalogAssetAction(
    CreativeEditorCatalogAssetAction action,
    std::int32_t direction) noexcept;

struct CreativeEditorCatalogState {
  iggy3d::creative::CreativeCatalogState model;
  iggy3d::creative::CreativeToolWheelState toolWheel;
  iggy3d::creative::CreativeCatalogShapeSelection shapeSelection{};
  CreativeEditorCatalogAssetAction assetAction =
      CreativeEditorCatalogAssetAction::Equip;
  std::size_t scrollOffset = 0;
  std::size_t actionScrollOffset = 0;
  std::optional<std::size_t> toolWheelAssignmentCatalogEntryIndex;
  std::string statusLabel;
};

struct CreativeEditorCatalogFrameRequest {
  iggy3d::SdlWindow& window;
  iggy3d::creative::CreativeAppState& appState;
  CreativeEditorState& editor;
  const iggy3d::creative::CreativeInputRouteResult& routedInput;
  const iggy3d::creative::CreativeWorldActionFrame& worldActions;
  const std::filesystem::path& toolWheelSettingsPath;
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
  bool assetReloadRequested = false;
  bool assetReplacementRequested = false;
  bool authoredAssetLibraryRequested = false;
  bool toolWheelChanged = false;
  bool toolWheelSaved = false;
  bool openToolOptionsRequested = false;
  iggy3d::creative::CreativeInputRouteResult deferredCommandInput;
  iggy3d::creative::CreativeHotbarEntry toolOptionsEntry{};
  iggy3d::creative::CreativeObjectKind replacementObjectKind =
      iggy3d::creative::CreativeObjectKind::Unknown;
  std::string replacementAssetId;
  std::string authoredAssetId;
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
