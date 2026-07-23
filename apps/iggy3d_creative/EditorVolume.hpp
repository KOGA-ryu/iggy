#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include "app/iggy3d/creative/tools/Volume.hpp"
#include "core/math/Vec3.hpp"
#include "render/FrameInput.hpp"

namespace iggy3d::creative {

struct CreativeAppState;

}  // namespace iggy3d::creative

namespace iggy3d_creative_app {

inline constexpr std::uint64_t kCreativeEditorVolumeCellLimit = 512U;
inline constexpr float kCreativeEditorVolumeHandleHitRadiusPixels = 12.0F;

enum class CreativeEditorVolumeHandleKind : std::uint8_t {
  None,
  MoveAxis,
  ResizeFace,
};

struct CreativeEditorVolumeHandle {
  CreativeEditorVolumeHandleKind kind = CreativeEditorVolumeHandleKind::None;
  iggy3d::creative::CreativeAxis3 axis =
      iggy3d::creative::CreativeAxis3::Count;
  iggy3d::creative::CreativeVolumeFace face =
      iggy3d::creative::CreativeVolumeFace::Count;
  iggy3d::Vec3 worldPosition{};
  iggy3d::Vec3 axisDirection{};
  float pixelX = 0.0F;
  float pixelY = 0.0F;
  bool valid = false;
};

struct CreativeEditorVolumeHandleFrame {
  iggy3d::Vec3 center{};
  float centerPixelX = 0.0F;
  float centerPixelY = 0.0F;
  std::array<CreativeEditorVolumeHandle, 9U> handles{};
  bool valid = false;
};

struct CreativeEditorVolumeHandlePick {
  bool hit = false;
  std::size_t index = 0U;
  float distanceSquared = 0.0F;
};

struct CreativeEditorVolumeHandleGesture {
  bool active = false;
  CreativeEditorVolumeHandle handle{};
  iggy3d::creative::CreativeVolumeSelection initialSelection{};
  iggy3d::creative::CreativeVec3 axisOrigin{};
  iggy3d::creative::CreativeVec3 axisDirection{};
  double initialAxisParameter = 0.0;
  std::int32_t appliedDeltaCells = 0;
};

struct CreativeEditorVolumePreviewCache {
  bool valid = false;
  bool stagedDocumentValid = false;
  std::uint64_t documentId = 0U;
  std::uint64_t documentRevision = 0U;
  iggy3d::creative::CreativeVolumeOperationRequest request{};
  iggy3d::creative::CreativeVolumeOperationReceipt receipt{};
  iggy3d::creative::CreativeDocument stagedDocument{};
  std::uint64_t refreshCount = 0U;
};

struct CreativeEditorVolumeState {
  bool active = false;
  bool cursorValid = false;
  std::string regionName = "Region 1";
  iggy3d::creative::CreativeGridCoord3 cursorCell{};
  iggy3d::creative::CreativeVolumeSelection selection{};
  iggy3d::creative::CreativeVolumeOperationKind operation =
      iggy3d::creative::CreativeVolumeOperationKind::Fill;
  iggy3d::creative::CreativeVolumeOperationReceipt lastReceipt{};
  CreativeEditorVolumePreviewCache preview{};
  CreativeEditorVolumeHandleGesture handleGesture{};
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
[[nodiscard]] iggy3d::creative::CreativeVolumeOperationRequest
makeCreativeEditorVolumeOperationRequest(
    const CreativeEditorVolumeState& state,
    const iggy3d::creative::CreativeVolumeSelection& selection,
    iggy3d::creative::CreativeObjectKind brushKind,
    const iggy3d::creative::CreativeToolSettings& toolSettings) noexcept;
[[nodiscard]] const iggy3d::creative::CreativeVolumeOperationReceipt&
refreshCreativeEditorVolumeOperationPreview(
    CreativeEditorVolumeState& state,
    const iggy3d::creative::CreativeDocument& document,
    const iggy3d::creative::CreativeVolumeSelection& selection,
    iggy3d::creative::CreativeObjectKind brushKind,
    const iggy3d::creative::CreativeToolSettings& toolSettings);
void invalidateCreativeEditorVolumeOperationPreview(
    CreativeEditorVolumeState& state) noexcept;
[[nodiscard]] CreativeEditorVolumeHandleFrame
buildCreativeEditorVolumeHandleFrame(
    const CreativeEditorVolumeState& state,
    const iggy3d::RenderCameraFrame& camera,
    iggy3d::RenderContentViewport contentViewport) noexcept;
[[nodiscard]] CreativeEditorVolumeHandlePick
pickCreativeEditorVolumeHandle(
    const CreativeEditorVolumeHandleFrame& frame,
    float pixelX,
    float pixelY,
    float hitRadiusPixels =
        kCreativeEditorVolumeHandleHitRadiusPixels) noexcept;
[[nodiscard]] bool beginCreativeEditorVolumeHandleGesture(
    CreativeEditorVolumeState& state,
    const CreativeEditorVolumeHandle& handle,
    iggy3d::creative::CreativeVec3 rayOrigin,
    iggy3d::creative::CreativeVec3 rayDirection) noexcept;
[[nodiscard]] bool updateCreativeEditorVolumeHandleGesture(
    CreativeEditorVolumeState& state,
    iggy3d::creative::CreativeVec3 rayOrigin,
    iggy3d::creative::CreativeVec3 rayDirection) noexcept;
[[nodiscard]] bool finishCreativeEditorVolumeHandleGesture(
    CreativeEditorVolumeState& state,
    bool commit) noexcept;
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
