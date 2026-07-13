#pragma once

#include "app/iggy3d/creative/tools/SelectionPlacement.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace iggy3d::creative {

// Immediate commands are a compatibility adapter over SelectionPlacement.
// Previewed and immediate transforms therefore share geometry, validation,
// mutation generation, and atomic application.
enum class CreativeTransformCommandKind : std::uint8_t {
  Translate,
  RotateYaw,
  Scale,
  ResetRotationScale,
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

[[nodiscard]] std::string_view toString(
    CreativeTransformCommandKind kind) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeTransformCommandStatus status) noexcept;

[[nodiscard]] CreativeTransformCommandReceipt transformDocumentObjectsAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    const CreativeTransformCommandRequest& request);

}  // namespace iggy3d::creative
