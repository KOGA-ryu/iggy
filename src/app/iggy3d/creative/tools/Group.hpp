#pragma once

#include "app/iggy3d/creative/document/DocumentMutation.hpp"

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

enum class CreativeHierarchySelectionStatus : std::uint8_t {
  NotRequested,
  EmptySelection,
  InvalidDocument,
  InvalidHierarchy,
  MissingObject,
  Ready,
};

struct CreativeHierarchySelection {
  bool requested = false;
  bool accepted = false;
  CreativeHierarchySelectionStatus status =
      CreativeHierarchySelectionStatus::NotRequested;
  std::uint64_t requestedObjectCount = 0U;
  CreativeObjectId missingObjectId = kInvalidObjectId;
  std::vector<CreativeObjectId> rootObjectIds;
  std::vector<CreativeObjectId> objectIds;
  std::string_view reasonCode = "creative_hierarchy_not_requested";
};

enum class CreativeGroupCommandKind : std::uint8_t {
  Group,
  Ungroup,
};

enum class CreativeGroupCommandStatus : std::uint8_t {
  NotRequested,
  EmptySelection,
  RequiresMultipleRoots,
  InvalidDocument,
  InvalidHierarchy,
  MissingObject,
  LockedObject,
  UnsupportedObject,
  MixedParents,
  NotGroup,
  CreateRejected,
  MutationRejected,
  RemoveRejected,
  Applied,
};

struct CreativeGroupCommandReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeGroupCommandKind kind = CreativeGroupCommandKind::Group;
  CreativeGroupCommandStatus status =
      CreativeGroupCommandStatus::NotRequested;
  std::uint64_t requestedObjectCount = 0U;
  std::uint64_t affectedObjectCount = 0U;
  CreativeObjectId groupObjectId = kInvalidObjectId;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  CreativeVec3 pivot{};
  std::uint64_t revisionBefore = 0U;
  std::uint64_t revisionAfter = 0U;
  CreativeDocumentCreateReceipt createReceipt{};
  CreativeDocumentBatchMutationReceipt mutationReceipt{};
  CreativeDocumentRemoveReceipt removeReceipt{};
  std::vector<CreativeObjectId> selectionObjectIds;
  std::string_view reasonCode = "creative_group_not_requested";
};

struct CreativeHierarchyRemoveReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeObjectId rootObjectId = kInvalidObjectId;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  std::uint64_t revisionBefore = 0U;
  std::uint64_t revisionAfter = 0U;
  std::vector<CreativeObjectId> removedObjectIds;
  CreativeDocumentRemoveReceipt rootReceipt{};
  std::string_view reasonCode = "creative_hierarchy_remove_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeHierarchySelectionStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeGroupCommandKind kind) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeGroupCommandStatus status) noexcept;

// Resolves unique selected roots and every descendant in document order.
// Selecting both an ancestor and its descendant keeps only the ancestor root.
// Complexity is O(n * h), where n is document size and h is hierarchy depth.
[[nodiscard]] CreativeHierarchySelection resolveCreativeObjectHierarchy(
    const CreativeDocument& document,
    std::span<const CreativeObjectId> selectedObjectIds);

// Keeps a selected Group as the interaction root when the visible hit belongs
// to one of its descendants. Otherwise returns hitObjectId unchanged.
[[nodiscard]] CreativeObjectId resolveCreativeHierarchyInteractionRoot(
    const CreativeDocument& document,
    std::span<const CreativeObjectId> selectedObjectIds,
    CreativeObjectId hitObjectId);

// Both commands stage the complete operation and publish only on success.
[[nodiscard]] CreativeGroupCommandReceipt groupDocumentObjectsAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> selectedObjectIds);
[[nodiscard]] CreativeGroupCommandReceipt ungroupDocumentObjectAtomically(
    CreativeDocument& document,
    CreativeObjectId groupObjectId);

// Removes descendants before their root and leaves the document unchanged on
// any lock, missing-object, or remove failure.
[[nodiscard]] CreativeHierarchyRemoveReceipt
removeCreativeObjectHierarchyAtomically(
    CreativeDocument& document,
    CreativeObjectId rootObjectId);

}  // namespace iggy3d::creative
