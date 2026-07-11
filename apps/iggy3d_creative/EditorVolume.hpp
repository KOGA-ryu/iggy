#pragma once

#include <cstdint>
#include <string_view>

#include "app/iggy3d/creative/tools/Volume.hpp"

namespace iggy3d::creative {

struct CreativeAppState;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

inline constexpr std::uint64_t kCreativeEditorVolumeCellLimit = 512U;

struct CreativeEditorVolumeState {
  bool active = false;
  bool cursorValid = false;
  iggy3d::creative::CreativeGridCoord3 cursorCell{};
  iggy3d::creative::CreativeVolumeSelection selection{};
  iggy3d::creative::CreativeVolumeOperationKind operation =
      iggy3d::creative::CreativeVolumeOperationKind::Fill;
  iggy3d::creative::CreativeVolumeOperationReceipt lastReceipt{};
};

enum class CreativeEditorVolumeGestureAction : std::uint8_t {
  Begin,
  Commit,
};

enum class CreativeEditorVolumeGestureStatus : std::uint8_t {
  NoTarget,
  NotArmed,
  Began,
  Completed,
};

struct CreativeEditorVolumeGestureReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeEditorVolumeGestureAction action =
      CreativeEditorVolumeGestureAction::Begin;
  CreativeEditorVolumeGestureStatus status =
      CreativeEditorVolumeGestureStatus::NoTarget;
  iggy3d::creative::CreativeVolumeSelectionPhase phaseBefore =
      iggy3d::creative::CreativeVolumeSelectionPhase::Empty;
  iggy3d::creative::CreativeVolumeSelectionPhase phaseAfter =
      iggy3d::creative::CreativeVolumeSelectionPhase::Empty;
  std::string_view reasonCode = "creative_volume_gesture_no_target";
};

void activateCreativeEditorVolumeMode(CreativeEditorVolumeState& state,
                                      double cellSize,
                                      iggy3d::creative::CreativeVec3 origin = {});
void deactivateCreativeEditorVolumeMode(
    CreativeEditorVolumeState& state) noexcept;
[[nodiscard]] iggy3d::creative::CreativeVolumeSelection
creativeEditorVolumePreviewSelection(
    const CreativeEditorVolumeState& state) noexcept;
[[nodiscard]] CreativeEditorVolumeGestureReceipt
stepCreativeEditorVolumeGesture(
    CreativeEditorVolumeState& state,
    CreativeEditorVolumeGestureAction action,
    bool targetValid,
    iggy3d::creative::CreativeGridCoord3 targetCell) noexcept;

[[nodiscard]] iggy3d::creative::CreativeVolumeOperationReceipt
applyCreativeEditorVolumeOperationWithHistory(
    iggy3d::creative::CreativeAppState& appState,
    CreativeEditorVolumeState& state,
    iggy3d::creative::CreativeObjectKind brushKind,
    iggy3d::creative::CreativeVolumeOperationKind operation,
    const iggy3d::creative::CreativeToolSettings& toolSettings,
    std::string_view source);

}  // namespace iggy3d_creative_app
