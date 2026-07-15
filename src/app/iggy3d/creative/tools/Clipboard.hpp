#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/document/Document.hpp"

namespace iggy3d::creative {

enum class CreativeClipboardStatus : std::uint8_t {
  NotRequested,
  EmptySelection,
  MissingObject,
  InvalidClipboard,
  InvalidRequest,
  ObjectIdExhausted,
  CreateRejected,
  RemoveRejected,
  Copied,
  Cut,
  Pasted,
};

enum class CreativeClipboardExternalParentPolicy : std::uint8_t {
  Detach,
  PreserveIfPresent,
};

enum class CreativeDuplicateCommandStatus : std::uint8_t {
  NotRequested,
  EmptySelection,
  InvalidRequest,
  MissingObject,
  Applied,
  Rejected,
};

struct CreativeClipboard {
  CreativeDocumentId sourceDocumentId = kInvalidDocumentId;
  std::uint64_t sourceRevision = 0;
  bool hasPlacementAnchor = false;
  CreativeVec3 placementAnchor{};
  std::vector<CreativeObject> objects;
  std::vector<CreativeLogicLink> logicLinks;
};

struct CreativeClipboardIdRemap {
  CreativeObjectId sourceObjectId = kInvalidObjectId;
  CreativeObjectId pastedObjectId = kInvalidObjectId;
};

struct CreativeClipboardCopyReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeClipboardStatus status = CreativeClipboardStatus::NotRequested;
  std::uint64_t requestedObjectCount = 0;
  std::uint64_t copiedObjectCount = 0;
  std::uint64_t copiedLogicLinkCount = 0;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  std::string reasonCode = "creative_clipboard_not_requested";
};

struct CreativeClipboardPasteRequest {
  CreativeVec3 offset{1.0, 0.0, 1.0};
  std::uint8_t quarterTurns = 0;
  bool mirrorX = false;
  bool mirrorZ = false;
  bool hasTransformAnchor = false;
  CreativeVec3 transformAnchor{};
  bool hasAxisAngleRotation = false;
  CreativeAxis3 rotationAxis = CreativeAxis3::Y;
  double rotationRadians = 0.0;
  bool appendCopySuffix = true;
  CreativeClipboardExternalParentPolicy externalParentPolicy =
      CreativeClipboardExternalParentPolicy::Detach;
};

struct CreativeClipboardPasteReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeClipboardStatus status = CreativeClipboardStatus::NotRequested;
  std::uint64_t requestedObjectCount = 0;
  std::uint64_t pastedObjectCount = 0;
  std::uint64_t pastedLogicLinkCount = 0;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::vector<CreativeClipboardIdRemap> idRemaps;
  std::vector<CreativeObjectId> pastedObjectIds;
  std::string reasonCode = "creative_clipboard_not_requested";
};

inline constexpr std::size_t kInvalidCreativeClipboardPasteIndex =
    std::numeric_limits<std::size_t>::max();

struct CreativeClipboardBatchPasteReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeClipboardStatus status = CreativeClipboardStatus::NotRequested;
  std::uint64_t requestedPasteCount = 0;
  std::uint64_t pastedPasteCount = 0;
  std::uint64_t requestedObjectCount = 0;
  std::uint64_t pastedObjectCount = 0;
  std::uint64_t requestedLogicLinkCount = 0;
  std::uint64_t pastedLogicLinkCount = 0;
  std::size_t failedPasteIndex = kInvalidCreativeClipboardPasteIndex;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::vector<CreativeClipboardIdRemap> idRemaps;
  std::vector<CreativeObjectId> pastedObjectIds;
  std::string reasonCode = "creative_clipboard_batch_not_requested";
};

struct CreativeClipboardCutReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeClipboardStatus status = CreativeClipboardStatus::NotRequested;
  std::uint64_t requestedObjectCount = 0;
  std::uint64_t cutObjectCount = 0;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  CreativeClipboardCopyReceipt copyReceipt;
  std::vector<CreativeDocumentRemoveReceipt> removeReceipts;
  std::string reasonCode = "creative_clipboard_not_requested";
};

struct CreativeDuplicateCommandRequest {
  CreativeVec3 offset{1.0, 0.0, 1.0};
  bool appendCopySuffix = true;
};

struct CreativeDuplicateCommandReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeDuplicateCommandStatus status =
      CreativeDuplicateCommandStatus::NotRequested;
  std::uint64_t requestedObjectCount = 0;
  std::uint64_t duplicatedObjectCount = 0;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  std::uint64_t revisionBefore = 0;
  std::uint64_t revisionAfter = 0;
  std::vector<CreativeObjectId> duplicatedObjectIds;
  // Mirrors requested hierarchy roots after ID remapping. Callers should
  // select these roots rather than every copied descendant.
  std::vector<CreativeObjectId> duplicatedSelectionObjectIds;
  std::string message = "duplicate_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeClipboardStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeDuplicateCommandStatus status) noexcept;
[[nodiscard]] bool creativeClipboardEmpty(
    const CreativeClipboard& clipboard) noexcept;
void clearCreativeClipboard(CreativeClipboard& clipboard) noexcept;

[[nodiscard]] CreativeClipboardCopyReceipt copyDocumentObjectsToClipboard(
    const CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    CreativeClipboard& outClipboard);

// O(n log n) for stable parent ordering, plus O(n) average hash lookups and one
// full document copy for atomic commit. Internal parent IDs are always remapped.
[[nodiscard]] CreativeClipboardPasteReceipt pasteCreativeClipboardAtomically(
    CreativeDocument& document,
    const CreativeClipboard& clipboard,
    const CreativeClipboardPasteRequest& request = {});

// O(n log n + n * k), where n is clipboard objects and k is paste requests,
// plus one target-document copy for atomic commit. Output IDs are grouped by
// paste request; each group uses the same stable parent-first object order.
[[nodiscard]] CreativeClipboardBatchPasteReceipt
pasteCreativeClipboardBatchAtomically(
    CreativeDocument& document,
    const CreativeClipboard& clipboard,
    std::span<const CreativeClipboardPasteRequest> requests);

// Uses the same stable parent ordering in reverse so children are removed before
// selected parents. Both document and clipboard remain unchanged on rejection.
[[nodiscard]] CreativeClipboardCutReceipt cutDocumentObjectsAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    CreativeClipboard& outClipboard);

// Resolves hierarchy roots, copies the complete hierarchy once, and remaps
// internal parents through the clipboard's atomic paste path.
[[nodiscard]] CreativeDuplicateCommandReceipt duplicateDocumentObjectsAtomically(
    CreativeDocument& document,
    std::span<const CreativeObjectId> objectIds,
    const CreativeDuplicateCommandRequest& request = {});

}  // namespace iggy3d::creative
