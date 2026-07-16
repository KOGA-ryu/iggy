#pragma once

#include "app/iggy3d/creative/CreativeAppState.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"
#include "app/iggy3d/creative/document/Object.hpp"
#include "app/iggy3d/creative/tools/Tools.hpp"

#include "EditorEdits.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string_view>
#include <vector>

namespace iggy3d_creative_app {
namespace cr = iggy3d::creative;

enum class CreativeMovingPlatformPathEditCommand : std::uint8_t {
  None,
  AppendAtTarget,
  RemoveLast,
  RemoveSelected,
  MoveSelectedToTarget,
  Count,
};

enum class CreativeMovingPlatformPathEditStatus : std::uint8_t {
  Idle,
  Ready,
  Queued,
  InvalidSelection,
  InvalidPath,
  InvalidTarget,
  InvalidPointIndex,
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
  std::uint8_t selectedPointIndex = 0U;
  bool available = false;
  bool pointSelected = false;
};

struct CreativeMovingPlatformPathTargetPlan {
  bool visible = false;
  bool appendAllowed = false;
  bool segmentVisible = false;
  CreativeMovingPlatformPathEditStatus status =
      CreativeMovingPlatformPathEditStatus::InvalidTarget;
  std::string_view reasonCode =
      "creative_platform_path_target_unavailable";
  cr::CreativeObjectId objectId = cr::kInvalidObjectId;
  cr::CreativeVec3 fromPoint{};
  cr::CreativeVec3 targetPoint{};
};

struct CreativeMovingPlatformPathPointTargetPlan {
  bool visible = false;
  bool moveAllowed = false;
  bool segmentVisible = false;
  CreativeMovingPlatformPathEditStatus status =
      CreativeMovingPlatformPathEditStatus::InvalidTarget;
  std::string_view reasonCode =
      "creative_platform_path_point_target_unavailable";
  cr::CreativeObjectId objectId = cr::kInvalidObjectId;
  std::size_t pointIndex = 0U;
  cr::CreativeVec3 fromPoint{};
  cr::CreativeVec3 targetPoint{};
};

inline constexpr std::size_t kInvalidCreativeMovingPlatformPathPointIndex =
    std::numeric_limits<std::size_t>::max();

// Per-frame admission is allocation-free and O(path points), bounded by the
// moving-platform path capacity.
[[nodiscard]] CreativeMovingPlatformPathTargetPlan
planCreativeMovingPlatformPathTarget(
    const cr::CreativeObject* object,
    bool targetAvailable,
    cr::CreativeVec3 placementAnchor) noexcept;

// Per-frame point relocation admission is allocation-free and O(path points),
// bounded by the moving-platform path capacity.
[[nodiscard]] CreativeMovingPlatformPathPointTargetPlan
planCreativeMovingPlatformPathPointTarget(
    const cr::CreativeObject* object,
    std::size_t pointIndex,
    bool targetAvailable,
    cr::CreativeVec3 placementAnchor,
    cr::CreativeMoveConstraint constraint) noexcept;

[[nodiscard]] CreativeMovingPlatformPathEditPlan
planCreativeMovingPlatformPathEdit(
    std::span<const cr::CreativePathPoint> currentPath,
    CreativeMovingPlatformPathEditCommand command,
    cr::CreativeVec3 targetPoint = {},
    std::size_t pointIndex = kInvalidCreativeMovingPlatformPathPointIndex);

void syncCreativeMovingPlatformPathEditState(
    const cr::CreativeAppState& appState,
    CreativeMovingPlatformPathEditState& state) noexcept;

[[nodiscard]] bool selectCreativeMovingPlatformPathPoint(
    CreativeMovingPlatformPathEditState& state,
    std::size_t pointIndex) noexcept;

[[nodiscard]] bool cycleCreativeMovingPlatformPathPoint(
    CreativeMovingPlatformPathEditState& state,
    int direction) noexcept;

[[nodiscard]] bool clearCreativeMovingPlatformPathPointSelection(
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
    std::string_view source,
    cr::CreativeMoveConstraint constraint = cr::CreativeMoveConstraint::Free);

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
