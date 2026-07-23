#pragma once

#include "app/iggy3d/creative/document/DocumentMutation.hpp"
#include "app/iggy3d/creative/tools/Group.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeHierarchyTransformStatus : std::uint8_t {
  NotRequested,
  InvalidDocument,
  InvalidRequest,
  MissingObject,
  InvalidHierarchy,
  LockedObject,
  InvalidTransform,
  TargetInsideSourceHierarchy,
  StalePlan,
  MutationRejected,
  NoChange,
  Applied,
  Rejected,
};

enum class CreativeHierarchyTransformPhase : std::uint8_t {
  None,
  ValidateRequest,
  ResolveHierarchy,
  ValidateLocks,
  DetachSource,
  Scale,
  ResetRotation,
  ApplyRotation,
  Move,
  Relationship,
  Commit,
};

struct CreativeHierarchyTransformRequest {
  CreativeObjectId rootObjectId = kInvalidObjectId;
  CreativeTransform targetTransform{};
  bool setPosition = false;
  bool setRotation = false;
  bool setScale = false;
};

struct CreativeHierarchyReattachmentRequest {
  CreativeDocumentId expectedDocumentId = kInvalidDocumentId;
  std::uint64_t expectedRevision = 0U;
  CreativeObjectId sourceRootObjectId = kInvalidObjectId;
  CreativeObjectId targetObjectId = kInvalidObjectId;
  std::string targetSocket{};
  CreativeTransform targetTransform{};
};

struct CreativeHierarchyTransformReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeHierarchyTransformStatus status =
      CreativeHierarchyTransformStatus::NotRequested;
  CreativeHierarchyTransformPhase phase = CreativeHierarchyTransformPhase::None;
  CreativeObjectId rootObjectId = kInvalidObjectId;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  std::uint64_t hierarchyObjectCount = 0U;
  std::uint64_t revisionBefore = 0U;
  std::uint64_t revisionAfter = 0U;
  std::string reasonCode = "creative_hierarchy_transform_not_requested";
  std::string message = "creative_hierarchy_transform_not_requested";
};

struct CreativeHierarchyReattachmentReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeHierarchyTransformStatus status =
      CreativeHierarchyTransformStatus::NotRequested;
  CreativeHierarchyTransformPhase phase = CreativeHierarchyTransformPhase::None;
  CreativeObjectId sourceRootObjectId = kInvalidObjectId;
  CreativeObjectId targetObjectId = kInvalidObjectId;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  std::uint64_t hierarchyObjectCount = 0U;
  std::uint64_t revisionBefore = 0U;
  std::uint64_t revisionAfter = 0U;
  std::string reasonCode = "creative_hierarchy_reattachment_not_requested";
  std::string message = "creative_hierarchy_reattachment_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeHierarchyTransformStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeHierarchyTransformPhase phase) noexcept;

[[nodiscard]] CreativeHierarchyTransformReceipt
applyCreativeHierarchyTransformAtomically(
    CreativeDocument& document,
    const CreativeHierarchyTransformRequest& request);

[[nodiscard]] CreativeHierarchyReattachmentReceipt
reattachCreativeObjectHierarchyAtomically(
    CreativeDocument& document,
    const CreativeHierarchyReattachmentRequest& request);

}  // namespace iggy3d::creative
