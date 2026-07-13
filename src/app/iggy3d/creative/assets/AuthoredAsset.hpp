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
  CreativeVec3 targetAnchor{};
  double yawRadians = 0.0;
  std::optional<CreativeObjectId> parentId;
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

[[nodiscard]] std::string_view toString(
    CreativeAuthoredAssetStatus status) noexcept;
[[nodiscard]] bool isValidCreativeAuthoredAssetId(
    std::string_view assetId) noexcept;

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

}  // namespace iggy3d::creative
