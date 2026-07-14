#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "app/iggy3d/creative/input/InputRouter.hpp"
#include "app/iggy3d/creative/input/Interaction.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d {

class SdlWindow;

}  // namespace iggy3d

namespace iggy3d::creative {

struct CreativeAppState;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

struct CreativeEditorState;

enum class CreativeEditorToolOptionsCommandId : std::uint8_t {
  SetMaterialBrushSymmetryPivot,
  ClearMaterialBrushSymmetryPivot,
  EditGroupContents,
  TransformSelection,
  ResetSelectionTransform,
  DuplicateSelection,
  DeleteSelection,
  ToggleSelectionVisibility,
  ToggleSelectionLocked,
  GroupSelection,
  UngroupSelection,
  SaveSelectionAsAsset,
  UpdateSavedAsset,
  RefreshSavedAssetInstances,
  Count,
};

inline constexpr std::size_t kCreativeEditorToolOptionsCommandCapacity = 12U;

struct CreativeEditorToolOptionsCommandList {
  std::array<CreativeEditorToolOptionsCommandId,
             kCreativeEditorToolOptionsCommandCapacity>
      ids{};
  std::size_t count = 0U;
};

struct CreativeEditorToolOptionsState {
  bool open = false;
  iggy3d::creative::CreativeHotbarEntry targetEntry{};
  iggy3d::creative::CreativeToolSettings draft;
  iggy3d::creative::CreativeToolOptionList options;
  CreativeEditorToolOptionsCommandList commands;
  std::size_t selectedIndex = 0;
  iggy3d::creative::CreativeObjectId contextGroupId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeObjectId contextPrimaryObjectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeObjectKind contextPrimaryObjectKind =
      iggy3d::creative::CreativeObjectKind::Unknown;
  iggy3d::creative::CreativeObjectKind contextContainerKind =
      iggy3d::creative::CreativeObjectKind::Unknown;
  std::string contextContainerAssetId;
  std::size_t contextSelectionCount = 0U;
  bool contextPrimaryVisible = true;
  bool contextPrimaryLocked = false;
  bool contextAllUnlocked = false;
  bool contextAllMovable = false;
  bool contextAllResettable = false;
  bool contextPrefabUpdateTransformSupported = false;
};

struct CreativeEditorQuickEditState {
  iggy3d::creative::CreativeHotbarEntry targetEntry{
      iggy3d::creative::CreativeHeldItemKind::Count,
      iggy3d::creative::CreativeObjectKind::Unknown};
  iggy3d::creative::CreativeToolOptionList options;
  std::size_t selectedIndex = 0;
};

struct CreativeEditorToolOptionsFrameRequest {
  iggy3d::SdlWindow& window;
  iggy3d::creative::CreativeAppState& appState;
  CreativeEditorState& editor;
  const iggy3d::creative::CreativeInputRouteResult& routedInput;
  bool openRequested = false;
  iggy3d::creative::CreativeHotbarEntry requestedEntry{};
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

[[nodiscard]] iggy3d::creative::CreativeToolOptionList
creativeEditorToolOptionsForEntry(
    iggy3d::creative::CreativeHotbarEntry entry,
    const iggy3d::creative::CreativeToolSettings& settings) noexcept;
[[nodiscard]] CreativeEditorToolOptionsCommandList
creativeEditorToolOptionCommandsForEntry(
    iggy3d::creative::CreativeHotbarEntry entry) noexcept;
[[nodiscard]] std::size_t creativeEditorToolOptionsRowCount(
    const CreativeEditorToolOptionsState& state) noexcept;
[[nodiscard]] bool activateCreativeEditorToolOptionsSelection(
    CreativeEditorState& editor);
[[nodiscard]] bool activateCreativeEditorToolOptionsSelection(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorState& editor);

void syncCreativeEditorQuickEdit(CreativeEditorState& editor);
[[nodiscard]] bool processCreativeEditorQuickEditAction(
    CreativeEditorState& editor,
    iggy3d::creative::CreativeInputActionId action);
[[nodiscard]] std::string creativeEditorQuickEditStatusLabel(
    const CreativeEditorState& editor);

void appendCreativeEditorToolOptionsOverlay(
    const CreativeEditorState& editor,
    std::uint32_t drawableWidth,
    std::uint32_t drawableHeight,
    std::vector<iggy3d::RenderUiRect>& uiRects,
    std::vector<iggy3d::DebugHudGlyphQuad>& glyphs);

}  // namespace iggy3d_creative_app
