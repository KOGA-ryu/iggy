#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "app/iggy3d/creative/document/Document.hpp"
#include "content/assets/StaticMeshAsset.hpp"

namespace iggy3d::creative {

inline constexpr double kCreativeAttachmentSnapRadiusMeters = 1.25;

enum class CreativeAttachmentSnapStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  SourceAssetMissing,
  SourcePlugMissing,
  TargetObjectMissing,
  TargetAssetMissing,
  TargetReceiverMissing,
  NoCompatibleSocket,
  OutsideRadius,
  Occupied,
  Ready,
};

struct CreativeAttachmentSnapRequest {
  const CreativeDocument* document = nullptr;
  const StaticMeshAssetCatalog* assetCatalog = nullptr;
  std::string_view sourceAssetId;
  CreativeObjectId targetObjectId = kInvalidObjectId;
  CreativeVec3 aimPoint{};
  CreativeVec3 sourceScale{1.0, 1.0, 1.0};
  double maxDistanceMeters = kCreativeAttachmentSnapRadiusMeters;
};

struct CreativeAttachmentSnapResult {
  CreativeAttachmentSnapStatus status =
      CreativeAttachmentSnapStatus::NotRequested;
  CreativeTransform transform{};
  CreativeObjectId targetObjectId = kInvalidObjectId;
  std::string_view sourceSocket;
  std::string_view targetSocket;
  std::string_view compatibility;
  double distanceMeters = 0.0;
  std::size_t sourcePlugCount = 0U;
  std::size_t targetReceiverCount = 0U;
  std::size_t compatiblePairCount = 0U;
  std::size_t occupiedPairCount = 0U;
  bool positioned = false;
  bool snapped = false;
};

[[nodiscard]] CreativeAttachmentSnapResult resolveCreativeAttachmentSnap(
    const CreativeAttachmentSnapRequest& request) noexcept;

}  // namespace iggy3d::creative
