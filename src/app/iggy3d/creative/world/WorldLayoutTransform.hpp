#pragma once

#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace iggy3d::creative {

enum class CreativeWorldLayoutBuildingTransformOperation : std::uint8_t {
  RotateLeft90,
  RotateRight90,
  MirrorX,
  MirrorZ,
  Count,
};

enum class CreativeWorldLayoutBuildingTransformStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  InvalidOwnership,
  EmptyBuilding,
  InvalidGeometry,
  CoordinateOverflow,
  Ready,
};

struct CreativeWorldLayoutBuildingBounds {
  bool valid = false;
  CreativeTerrainCoord2 minimum;
  CreativeTerrainCoord2 maximum;
};

struct CreativeWorldLayoutBuildingTransformRequest {
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutBuildingTransformOperation operation =
      CreativeWorldLayoutBuildingTransformOperation::RotateRight90;
};

struct CreativeWorldLayoutBuildingTransformResult {
  bool requested = false;
  bool accepted = false;
  CreativeWorldLayoutBuildingTransformStatus status =
      CreativeWorldLayoutBuildingTransformStatus::NotRequested;
  std::size_t buildingIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutBuildingTransformOperation operation =
      CreativeWorldLayoutBuildingTransformOperation::RotateRight90;
  CreativeWorldLayoutBuildingBounds sourceBounds;
  CreativeWorldLayoutBuildingBounds transformedBounds;
  CreativeWorldLayout transformed;
  std::string reasonCode =
      "creative_world_layout_building_transform_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutBuildingTransformOperation operation) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutBuildingTransformStatus status) noexcept;

[[nodiscard]] bool measureCreativeWorldLayoutBuildingBounds(
    const CreativeWorldLayout& layout,
    std::size_t buildingIndex,
    CreativeWorldLayoutBuildingBounds& output) noexcept;

// O(buildings + rooms + boxes + walls + openings), with one full layout copy.
// Stable keys, source order, heights, and terrain remain unchanged. Quarter
// turns keep the aggregate minimum grid line fixed, avoiding half-cell pivots;
// axis spans swap. Hosted openings and open-door poses are remapped so the
// generated 3D geometry follows the same exact transform. Rejection leaves the
// source untouched and returns no partial candidate.
[[nodiscard]] CreativeWorldLayoutBuildingTransformResult
transformCreativeWorldLayoutBuilding(
    const CreativeWorldLayout& source,
    const CreativeWorldLayoutBuildingTransformRequest& request);

}  // namespace iggy3d::creative
