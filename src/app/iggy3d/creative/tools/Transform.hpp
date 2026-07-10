#pragma once

#include "app/iggy3d/creative/document/DocumentMutation.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

enum class CreativeTransformCommandKind : std::uint8_t {
  Translate,
  RotateYaw,
  Scale,
};

enum class CreativeTransformCommandStatus : std::uint8_t {
  Unknown,
  EmptySelection,
  InvalidRequest,
  MissingObject,
  LockedObject,
  UnsupportedObject,
  Applied,
  NoChange,
  Rejected,
};

struct CreativeTransformCommandRequest {
  CreativeTransformCommandKind kind = CreativeTransformCommandKind::Translate;
  CreativeVec3 translation{};
  double yawDegrees = 0.0;
  CreativeVec3 scaleFactor{1.0, 1.0, 1.0};
};

struct CreativeTransformCommandReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeTransformCommandKind kind = CreativeTransformCommandKind::Translate;
  CreativeTransformCommandStatus status = CreativeTransformCommandStatus::Unknown;
  std::uint64_t requestedObjectCount = 0;
  std::uint64_t objectCount = 0;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  CreativeMutationKind failedMutationKind = CreativeMutationKind::Unknown;
  CreativeVec3 pivot{};
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  CreativeDocumentBatchMutationReceipt mutationReceipt{};
  std::string message = "transform_not_requested";
};

struct CreativeDuplicateCommandRequest {
  CreativeVec3 offset{1.0, 0.0, 1.0};
  bool appendCopySuffix = true;
};

struct CreativeDuplicateCommandReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeTransformCommandStatus status = CreativeTransformCommandStatus::Unknown;
  std::uint64_t requestedObjectCount = 0;
  std::uint64_t duplicatedObjectCount = 0;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::vector<CreativeObjectId> duplicatedObjectIds;
  std::string message = "duplicate_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeTransformCommandKind kind) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTransformCommandStatus status) noexcept;

[[nodiscard]] CreativeTransformCommandReceipt transformDocumentObjectsAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    const CreativeTransformCommandRequest& request);

[[nodiscard]] CreativeDuplicateCommandReceipt duplicateDocumentObjectsAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    const CreativeDuplicateCommandRequest& request = {});

}  // namespace iggy3d::creative
