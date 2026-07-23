#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/document/Document.hpp"
#include "app/iggy3d/creative/document/DocumentMutation.hpp"
#include "app/iggy3d/creative/tools/Clipboard.hpp"

namespace iggy3d::creative {

inline constexpr std::size_t kCreativeAuthoredAssetObjectCapacity = 256U;
inline constexpr std::size_t kCreativeAuthoredAssetIdCapacity = 64U;

enum class CreativeAuthoredAssetStatus : std::uint8_t {
  NotRequested,
  InvalidDocument,
  InvalidIdentity,
  EmptySelection,
  MissingObject,
  CapacityExceeded,
  InvalidGeometry,
  InvalidDefinition,
  CreateRejected,
  MutationRejected,
  Ready,
  Instantiated,
};

struct CreativeAuthoredAssetDefinition {
  std::string assetId;
  std::string label;
  CreativeBounds sourceBounds{};
  CreativeClipboard content;
  std::vector<CreativeObjectId> rootObjectIds;
};

struct CreativeAuthoredAssetFingerprint {
  bool valid = false;
  std::uint64_t value = 0U;
};

enum class CreativeAuthoredAssetSyncState : std::uint8_t {
  Current,
  SourceChanged,
  LocallyModified,
  Conflict,
};

struct CreativeAuthoredAssetSyncReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeAuthoredAssetSyncState state =
      CreativeAuthoredAssetSyncState::Conflict;
  CreativeObjectId instanceRootObjectId = kInvalidObjectId;
  CreativeAuthoredAssetFingerprint sourceFingerprint;
  CreativeAuthoredAssetFingerprint instanceFingerprint;
  std::optional<std::uint64_t> storedSourceFingerprint;
  std::string_view reasonCode =
      "creative_authored_asset_sync_not_requested";
};

struct CreativeAuthoredAssetSyncSummary {
  bool requested = false;
  bool accepted = false;
  std::string_view assetId;
  std::size_t matchedInstanceCount = 0U;
  std::size_t currentInstanceCount = 0U;
  std::size_t sourceChangedInstanceCount = 0U;
  std::size_t locallyModifiedInstanceCount = 0U;
  std::size_t conflictInstanceCount = 0U;
  std::string_view reasonCode =
      "creative_authored_asset_sync_not_requested";
};

struct CreativeAuthoredAssetCaptureRequest {
  const CreativeDocument* sourceDocument = nullptr;
  std::span<const CreativeObjectId> selectedObjectIds;
  std::string_view assetId;
  std::string_view label;
  CreativeDocumentId definitionDocumentId = kInvalidDocumentId;
};

struct CreativeAuthoredAssetInstanceCaptureRequest {
  const CreativeDocument* sourceDocument = nullptr;
  const CreativeAuthoredAssetDefinition* existingDefinition = nullptr;
  CreativeObjectId instanceRootObjectId = kInvalidObjectId;
  CreativeDocumentId definitionDocumentId = kInvalidDocumentId;
};

struct CreativeAuthoredAssetCaptureResult {
  bool requested = false;
  bool accepted = false;
  CreativeAuthoredAssetStatus status =
      CreativeAuthoredAssetStatus::NotRequested;
  CreativeAuthoredAssetDefinition definition;
  CreativeDocument storageDocument;
  std::size_t capturedObjectCount = 0U;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  std::string_view reasonCode = "creative_authored_asset_not_requested";
};

struct CreativeAuthoredAssetLoadResult {
  bool requested = false;
  bool accepted = false;
  CreativeAuthoredAssetStatus status =
      CreativeAuthoredAssetStatus::NotRequested;
  CreativeAuthoredAssetDefinition definition;
  std::string_view reasonCode = "creative_authored_asset_not_requested";
};

struct CreativeAuthoredAssetPlacementRequest {
  const CreativeAuthoredAssetDefinition* definition = nullptr;
  CreativeTransform instanceTransform{};
  std::optional<CreativeObjectId> parentId;
  std::string attachmentSocket;
};

struct CreativeAuthoredAssetPlacementPlan {
  bool requested = false;
  bool accepted = false;
  CreativeAuthoredAssetStatus status =
      CreativeAuthoredAssetStatus::NotRequested;
  CreativeDocumentCreateRequest rootRequest;
  CreativeClipboardPasteRequest contentPasteRequest;
  std::array<CreativeObjectId, kCreativeAuthoredAssetObjectCapacity>
      sourceRootObjectIds{};
  std::uint16_t sourceRootObjectCount = 0U;
  std::string_view reasonCode = "creative_authored_asset_not_requested";

  [[nodiscard]] std::span<const CreativeObjectId> sourceRoots()
      const noexcept {
    return {sourceRootObjectIds.data(), sourceRootObjectCount};
  }
};

struct CreativeAuthoredAssetInstanceReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeAuthoredAssetStatus status =
      CreativeAuthoredAssetStatus::NotRequested;
  CreativeObjectId instanceRootObjectId = kInvalidObjectId;
  std::vector<CreativeObjectId> instanceObjectIds;
  std::uint64_t revisionBefore = 0U;
  std::uint64_t revisionAfter = 0U;
  CreativeDocumentCreateReceipt rootCreateReceipt;
  CreativeClipboardPasteReceipt contentPasteReceipt;
  CreativeDocumentBatchMutationReceipt parentMutationReceipt;
  std::string_view reasonCode = "creative_authored_asset_not_requested";
};

enum class CreativeAuthoredAssetRefreshStatus : std::uint8_t {
  NotRequested,
  InvalidDocument,
  InvalidDefinition,
  NoMatchingInstances,
  NoEligibleInstances,
  UnsupportedInstance,
  LockedObject,
  InvalidHierarchy,
  MutationRejected,
  Refreshed,
};

enum class CreativeAuthoredAssetRefreshMode : std::uint8_t {
  SelectedInstance,
  SafeInstances,
  ForceAll,
};

struct CreativeAuthoredAssetRefreshRequest {
  const CreativeAuthoredAssetDefinition* definition = nullptr;
  CreativeAuthoredAssetRefreshMode mode =
      CreativeAuthoredAssetRefreshMode::ForceAll;
  CreativeObjectId selectedInstanceRootObjectId = kInvalidObjectId;
};

struct CreativeAuthoredAssetRefreshReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeAuthoredAssetRefreshStatus status =
      CreativeAuthoredAssetRefreshStatus::NotRequested;
  CreativeAuthoredAssetRefreshMode mode =
      CreativeAuthoredAssetRefreshMode::ForceAll;
  std::string_view assetId;
  std::vector<CreativeObjectId> instanceRootObjectIds;
  std::size_t matchedInstanceCount = 0U;
  std::size_t currentInstanceCount = 0U;
  std::size_t sourceChangedInstanceCount = 0U;
  std::size_t locallyModifiedInstanceCount = 0U;
  std::size_t conflictInstanceCount = 0U;
  std::size_t refreshedInstanceCount = 0U;
  std::size_t removedObjectCount = 0U;
  std::size_t createdObjectCount = 0U;
  CreativeObjectId failedInstanceRootObjectId = kInvalidObjectId;
  CreativeObjectId failedObjectId = kInvalidObjectId;
  std::uint64_t revisionBefore = 0U;
  std::uint64_t revisionAfter = 0U;
  std::string_view reasonCode =
      "creative_authored_asset_refresh_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeAuthoredAssetStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeAuthoredAssetRefreshStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeAuthoredAssetSyncState state) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeAuthoredAssetRefreshMode mode) noexcept;
[[nodiscard]] bool isValidCreativeAuthoredAssetId(
    std::string_view assetId) noexcept;

// Hashes authored content independently of object IDs and instance placement.
// Floating-point fields are quantized to one micrometer / microradian so a
// rigid placement followed by inverse capture produces the same fingerprint.
[[nodiscard]] CreativeAuthoredAssetFingerprint
fingerprintCreativeAuthoredAssetDefinition(
    const CreativeAuthoredAssetDefinition& definition) noexcept;
[[nodiscard]] CreativeAuthoredAssetFingerprint
fingerprintCreativeAuthoredAssetPlacementRequest(
    const CreativeAuthoredAssetPlacementRequest& request) noexcept;
[[nodiscard]] CreativeAuthoredAssetFingerprint
fingerprintCreativeAuthoredAssetRefreshRequest(
    const CreativeAuthoredAssetRefreshRequest& request) noexcept;
[[nodiscard]] CreativeAuthoredAssetFingerprint
fingerprintCreativeAuthoredAssetInstance(
    const CreativeObject& instanceRoot) noexcept;
[[nodiscard]] CreativeAuthoredAssetFingerprint
foldCreativeAuthoredAssetOperationFingerprint(
    std::uint64_t accumulatedFingerprint,
    std::uint64_t requestFingerprint) noexcept;
[[nodiscard]] std::optional<std::uint64_t>
creativeAuthoredAssetStoredSourceFingerprint(
    const CreativeObject& instanceRoot) noexcept;
[[nodiscard]] CreativeAuthoredAssetSyncReceipt
inspectCreativeAuthoredAssetInstanceSync(
    const CreativeDocument& document,
    const CreativeAuthoredAssetDefinition& definition,
    CreativeObjectId instanceRootObjectId);
[[nodiscard]] CreativeAuthoredAssetSyncSummary
summarizeCreativeAuthoredAssetSync(
    const CreativeDocument& document,
    const CreativeAuthoredAssetDefinition& definition);
[[nodiscard]] CreativeDocumentBatchMutationReceipt
acknowledgeCreativeAuthoredAssetInstanceSource(
    CreativeDocument& document,
    const CreativeAuthoredAssetDefinition& definition,
    CreativeObjectId instanceRootObjectId);

[[nodiscard]] CreativeAuthoredAssetCaptureResult captureCreativeAuthoredAsset(
    const CreativeAuthoredAssetCaptureRequest& request);
[[nodiscard]] bool creativeAuthoredAssetInstanceTransformSupported(
    const CreativeObject& instanceRoot) noexcept;
[[nodiscard]] CreativeAuthoredAssetCaptureResult
captureCreativeAuthoredAssetInstance(
    const CreativeAuthoredAssetInstanceCaptureRequest& request);

[[nodiscard]] CreativeAuthoredAssetLoadResult
loadCreativeAuthoredAssetDefinition(const CreativeDocument& storageDocument,
                                    std::string_view assetId,
                                    std::string_view label);

[[nodiscard]] CreativeAuthoredAssetPlacementPlan
planCreativeAuthoredAssetPlacement(
    const CreativeAuthoredAssetPlacementRequest& request) noexcept;

[[nodiscard]] CreativeAuthoredAssetInstanceReceipt
instantiateCreativeAuthoredAssetAtomically(
    CreativeDocument& document,
    const CreativeAuthoredAssetPlacementRequest& request);

// Replaces the children of every matching instance in one staged transaction.
// Instance root identity, transform, and parent are preserved. Complexity is
// O(I * (N + A log A)), where I is the matching-instance count, N is document
// size, and A is the number of objects below one instance.
[[nodiscard]] CreativeAuthoredAssetRefreshReceipt
refreshCreativeAuthoredAssetInstancesAtomically(
    CreativeDocument& document,
    const CreativeAuthoredAssetDefinition& definition);
[[nodiscard]] CreativeAuthoredAssetRefreshReceipt
refreshCreativeAuthoredAssetInstancesAtomically(
    CreativeDocument& document,
    const CreativeAuthoredAssetRefreshRequest& request);

}  // namespace iggy3d::creative
