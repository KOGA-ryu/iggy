#pragma once

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"
#include "app/iggy3d/creative/document/Object.hpp"

#include "EditorEdits.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

enum class CreativeMovingPlatformPathEditCommand : std::uint8_t {
  None,
  AppendAtTarget,
  RemoveLast,
  Count,
};

enum class CreativeMovingPlatformPathEditStatus : std::uint8_t {
  Idle,
  Ready,
  Queued,
  InvalidSelection,
  InvalidPath,
  InvalidTarget,
  DuplicateTarget,
  MinimumPointCount,
  CapacityReached,
  Applied,
  MutationRejected,
  Count,
};

struct CreativeMovingPlatformPathEditPlan {
  bool accepted = false;
  bool changed = false;
  CreativeMovingPlatformPathEditStatus status =
      CreativeMovingPlatformPathEditStatus::Idle;
  std::string_view reasonCode = "creative_platform_path_edit_not_requested";
  std::vector<cr::CreativePathPoint> pathPoints;
};

struct CreativeMovingPlatformPathEditReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeMovingPlatformPathEditStatus status =
      CreativeMovingPlatformPathEditStatus::Idle;
  std::string_view reasonCode = "creative_platform_path_edit_not_requested";
  cr::CreativeObjectId objectId = cr::kInvalidObjectId;
  std::size_t pointCountBefore = 0U;
  std::size_t pointCountAfter = 0U;
  cr::CreativeDocumentMutationReceipt mutation;
  cr::CreativeHistoryRecordReceipt history;
};

struct CreativeMovingPlatformPathEditState {
  cr::CreativeDocumentId documentId = cr::kInvalidDocumentId;
  cr::CreativeObjectId objectId = cr::kInvalidObjectId;
  CreativeMovingPlatformPathEditCommand pending =
      CreativeMovingPlatformPathEditCommand::None;
  CreativeMovingPlatformPathEditStatus status =
      CreativeMovingPlatformPathEditStatus::Idle;
  std::string_view reasonCode = "creative_platform_path_edit_idle";
  std::uint8_t pointCount = 0U;
  bool available = false;
};

[[nodiscard]] CreativeMovingPlatformPathEditPlan
planCreativeMovingPlatformPathEdit(
    std::span<const cr::CreativePathPoint> currentPath,
    CreativeMovingPlatformPathEditCommand command,
    cr::CreativeVec3 targetPoint = {});

void syncCreativeMovingPlatformPathEditState(
    const cr::CreativeAppState& appState,
    CreativeMovingPlatformPathEditState& state) noexcept;

[[nodiscard]] bool queueCreativeMovingPlatformPathEdit(
    const cr::CreativeAppState& appState,
    CreativeMovingPlatformPathEditState& state,
    CreativeMovingPlatformPathEditCommand command) noexcept;

[[nodiscard]] CreativeMovingPlatformPathEditReceipt
consumeCreativeMovingPlatformPathEdit(
    cr::CreativeAppState& appState,
    CreativeMovingPlatformPathEditState& state,
    bool targetAvailable,
    cr::CreativeVec3 targetAnchor,
    std::string_view source);

[[nodiscard]] cr::CreativeDocumentMutationReceipt movePathObjectWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    cr::CreativeObjectId objectId,
    cr::CreativeVec3 delta,
    std::string_view source);

[[nodiscard]] cr::CreativeDocumentMutationReceipt movePathPointWithUndo(
    cr::CreativeAppState& appState,
    StandaloneEditHistory& history,
    cr::CreativeObjectId objectId,
    std::size_t pointIndex,
    cr::CreativeVec3 delta,
    std::string_view source);

}  // namespace iggy3d_creative_app
