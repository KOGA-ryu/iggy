#pragma once

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
  std::uint8_t quarterTurns = 0;
  bool mirrorX = false;
  bool mirrorZ = false;
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
    CreativeSelectionPlacementStatus status) noexcept;

[[nodiscard]] CreativeSelectionPlacementPlan planCreativeSelectionPlacement(
    std::span<const CreativeObject> objects,
    const CreativeSelectionPlacementRequest& request);

[[nodiscard]] CreativeSelectionPlacementReceipt
placeDocumentObjectsAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    const CreativeSelectionPlacementRequest& request);

}  // namespace iggy3d::creative
