#pragma once

#include <cstdint>
#include <string_view>

#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/play/RuntimeMovingPlatforms.hpp"

namespace iggy3d_creative_app {

enum class CreativeMovingPlatformPreviewCommand : std::uint8_t {
  TogglePlayback,
  Restart,
  Seek,
  Count,
};

enum class CreativeMovingPlatformPreviewStatus : std::uint8_t {
  Unavailable,
  Paused,
  Playing,
  Invalid,
};

struct CreativeMovingPlatformPreviewState {
  iggy3d::creative::CreativeDocumentId documentId =
      iggy3d::creative::kInvalidDocumentId;
  iggy3d::creative::CreativeObjectId objectId =
      iggy3d::creative::kInvalidObjectId;
  iggy3d::creative::CreativeTransform authoredTransform;
  iggy3d::creative::CreativeRuntimeMovingPlatformDefinition definition;
  iggy3d::creative::CreativeRuntimeMovingPlatformState runtimeState;
  double normalizedProgress = 0.0;
  double fixedTickAccumulatorSeconds = 0.0;
  CreativeMovingPlatformPreviewStatus status =
      CreativeMovingPlatformPreviewStatus::Unavailable;
  std::string_view reasonCode = "creative_platform_preview_unavailable";
  bool available = false;
  bool visible = false;
  bool playing = false;
};

struct CreativeMovingPlatformPreviewReceipt {
  bool accepted = false;
  bool changed = false;
  bool reset = false;
  std::uint32_t advancedTickCount = 0U;
  std::string_view reasonCode = "creative_platform_preview_not_requested";
};

// Rebuilds only when selection or authored route/settings/transform change.
// Same-object edits reset to route start while preserving Play/Pause intent.
[[nodiscard]] CreativeMovingPlatformPreviewReceipt
syncCreativeMovingPlatformPreview(
    CreativeMovingPlatformPreviewState& state,
    iggy3d::creative::CreativeDocumentId documentId,
    const iggy3d::creative::CreativeObject* selectedObject) noexcept;

[[nodiscard]] CreativeMovingPlatformPreviewReceipt
applyCreativeMovingPlatformPreviewCommand(
    CreativeMovingPlatformPreviewState& state,
    CreativeMovingPlatformPreviewCommand command,
    iggy3d::creative::CreativeObjectId objectId,
    double normalizedProgress = 0.0) noexcept;

// Advances through the runtime's exact fixed-tick planner. Catch-up is bounded
// so a stalled editor frame cannot create an unbounded per-frame loop.
[[nodiscard]] CreativeMovingPlatformPreviewReceipt
advanceCreativeMovingPlatformPreview(
    CreativeMovingPlatformPreviewState& state,
    double presentationDeltaSeconds) noexcept;

[[nodiscard]] std::string_view creativeMovingPlatformPreviewStatusLabel(
    const CreativeMovingPlatformPreviewState& state) noexcept;

}  // namespace iggy3d_creative_app
