#pragma once

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

enum class CreativeSelectionPlacementMode : std::uint8_t {
  Copy,
  Move,
};

enum class CreativeSelectionPlacementAxis : std::uint8_t {
  Free,
  X,
  Y,
  Z,
  Count,
};

enum class CreativeSelectionPlacementTargetStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  Resolved,
};

enum class CreativeSelectionPlacementNudgeStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  AxisRequired,
  NoChange,
  Applied,
};

enum class CreativeSelectionPlacementStatus : std::uint8_t {
  NotRequested,
  EmptySource,
  InvalidRequest,
  InvalidSource,
  MissingObject,
  LockedObject,
  UnsupportedObject,
  Planned,
  Applied,
  NoChange,
  Rejected,
};

struct CreativeSelectionPlacementRequest {
  CreativeSelectionPlacementMode mode = CreativeSelectionPlacementMode::Copy;
  CreativeVec3 sourceAnchor{};
  CreativeVec3 targetAnchor{};
  // Scaling is currently a Move-only operation; Copy requires {1, 1, 1}.
  // Selection offsets use world axes while each object's authored scale
  // channels remain local to its stored transform.
  CreativeVec3 scaleFactor{1.0, 1.0, 1.0};
  std::uint8_t quarterTurns = 0;
  bool mirrorX = false;
  bool mirrorZ = false;
  bool hasAxisAngleRotation = false;
  CreativeAxis3 rotationAxis = CreativeAxis3::Y;
  double rotationRadians = 0.0;
};

struct CreativeSelectionPlacementTargetRequest {
  CreativeVec3 sourceAnchor{};
  CreativeVec3 aimedAnchor{};
  CreativeVec3 nudgeOffset{};
  CreativeSelectionPlacementAxis axis =
      CreativeSelectionPlacementAxis::Free;
  double snapStepMeters = 1.0;
};

struct CreativeSelectionPlacementTargetResult {
  bool requested = false;
  bool accepted = false;
  CreativeSelectionPlacementTargetStatus status =
      CreativeSelectionPlacementTargetStatus::NotRequested;
  CreativeSelectionPlacementTargetRequest request{};
  CreativeVec3 displacement{};
  CreativeVec3 targetAnchor{};
  std::string_view reasonCode = "selection_placement_target_not_requested";
};

struct CreativeSelectionPlacementNudgeRequest {
  CreativeVec3 offset{};
  CreativeSelectionPlacementAxis axis =
      CreativeSelectionPlacementAxis::Free;
  double snapStepMeters = 1.0;
  std::int32_t steps = 0;
  bool fine = false;
};

struct CreativeSelectionPlacementNudgeReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeSelectionPlacementNudgeStatus status =
      CreativeSelectionPlacementNudgeStatus::NotRequested;
  CreativeSelectionPlacementNudgeRequest request{};
  CreativeVec3 offset{};
  double appliedStepMeters = 0.0;
  std::string_view reasonCode = "selection_placement_nudge_not_requested";
};

struct CreativeSelectionPlacementPlan {
  bool requested = false;
  bool accepted = false;
  CreativeSelectionPlacementStatus status =
      CreativeSelectionPlacementStatus::NotRequested;
  CreativeSelectionPlacementRequest request{};
  std::uint64_t objectCount = 0;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  CreativeMutationKind failedMutationKind = CreativeMutationKind::Unknown;
  std::vector<CreativeObject> objects;
  CreativeBounds aggregateBounds{};
  bool hasAggregateBounds = false;
  std::string reasonCode = "selection_placement_not_requested";
};

struct CreativeSelectionPlacementReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeSelectionPlacementStatus status =
      CreativeSelectionPlacementStatus::NotRequested;
  std::uint64_t requestedObjectCount = 0;
  std::uint64_t objectCount = 0;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  CreativeMutationKind failedMutationKind = CreativeMutationKind::Unknown;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  CreativeSelectionPlacementPlan plan{};
  CreativeDocumentBatchMutationReceipt mutationReceipt{};
  std::string reasonCode = "selection_placement_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeSelectionPlacementMode mode) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeSelectionPlacementAxis axis) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeSelectionPlacementStatus status) noexcept;

// Pure precision kernels. Constraint snapping is relative to sourceAnchor so
// negative coordinates and non-grid source pivots remain deterministic. Fine
// nudges use one quarter of snapStepMeters.
[[nodiscard]] CreativeSelectionPlacementTargetResult
resolveCreativeSelectionPlacementTarget(
    const CreativeSelectionPlacementTargetRequest& request) noexcept;
[[nodiscard]] CreativeSelectionPlacementNudgeReceipt
nudgeCreativeSelectionPlacementOffset(
    const CreativeSelectionPlacementNudgeRequest& request) noexcept;

[[nodiscard]] CreativeSelectionPlacementPlan planCreativeSelectionPlacement(
    std::span<const CreativeObject> objects,
    const CreativeSelectionPlacementRequest& request);

[[nodiscard]] CreativeSelectionPlacementReceipt
placeDocumentObjectsAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    const CreativeSelectionPlacementRequest& request);

}  // namespace iggy3d::creative
