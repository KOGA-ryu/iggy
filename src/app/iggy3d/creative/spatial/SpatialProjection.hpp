#pragma once

#include "app/iggy3d/creative/document/ObjectDescriptor.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

using CreativeGridIndex = std::uint64_t;

struct CreativeGridSize3 {
  std::int32_t width = 0;
  std::int32_t height = 0;
  std::int32_t depth = 0;
};

struct CreativeGridCoord3 {
  std::int32_t x = 0;
  std::int32_t y = 0;
  std::int32_t z = 0;
};

struct CreativeGridBounds3 {
  CreativeGridCoord3 min;
  CreativeGridCoord3 max;
};

enum class CreativeSpatialProjectionStatus {
  Unknown,
  InvalidGrid,
  InvalidObject,
  NoProjection,
  EmptyProjection,
  OutOfBounds,
  Projected,
};

struct CreativeSpatialCell {
  CreativeGridIndex index = 0;
  CreativeGridCoord3 coord;
  CreativeObjectId objectId = kInvalidObjectId;
  CreativeObjectKind objectKind = CreativeObjectKind::Unknown;
  CreativeSpatialOccupancyKind occupancyKind =
      CreativeSpatialOccupancyKind::Unknown;
};

struct CreativeSpatialProjectionRequest {
  CreativeGridSize3 gridSize;
  double cellSize = 1.0;
  bool clampToGrid = true;
  bool includeAuthoringOnly = false;
};

struct CreativeSpatialProjectionReceipt {
  CreativeSpatialProjectionStatus status =
      CreativeSpatialProjectionStatus::Unknown;
  CreativeObjectId objectId = kInvalidObjectId;
  CreativeObjectKind objectKind = CreativeObjectKind::Unknown;
  CreativeSpatialProjectionProfile profile =
      CreativeSpatialProjectionProfile::Unknown;
  CreativeSpatialOccupancyKind occupancyKind =
      CreativeSpatialOccupancyKind::Unknown;
  CreativeGridBounds3 projectedBounds;
  std::vector<CreativeSpatialCell> cells;
  std::string message;
};

struct CreativeSpatialProjectionSummary {
  CreativeSpatialProjectionStatus status =
      CreativeSpatialProjectionStatus::Unknown;
  CreativeObjectId objectId = kInvalidObjectId;
  CreativeObjectKind objectKind = CreativeObjectKind::Unknown;
  CreativeSpatialProjectionProfile profile =
      CreativeSpatialProjectionProfile::Unknown;
  CreativeSpatialOccupancyKind occupancyKind =
      CreativeSpatialOccupancyKind::Unknown;
  CreativeGridBounds3 projectedBounds;
  std::uint64_t cellCount = 0;
};

[[nodiscard]] std::string_view toString(
    CreativeSpatialProjectionProfile profile) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeSpatialOccupancyKind occupancyKind) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeSpatialProjectionStatus status) noexcept;

[[nodiscard]] bool isValidGridSize(CreativeGridSize3 size) noexcept;
[[nodiscard]] bool isInsideGrid(CreativeGridCoord3 coord,
                                CreativeGridSize3 size) noexcept;
[[nodiscard]] CreativeGridIndex toGridIndex(CreativeGridCoord3 coord,
                                            CreativeGridSize3 size) noexcept;
[[nodiscard]] CreativeGridCoord3 toGridCoord(CreativeGridIndex index,
                                             CreativeGridSize3 size) noexcept;
[[nodiscard]] CreativeGridCoord3 worldToGridCoord(
    CreativeVec3 position,
    double cellSize) noexcept;
[[nodiscard]] CreativeGridBounds3 worldBoundsToGridBounds(
    CreativeBounds bounds,
    double cellSize) noexcept;
[[nodiscard]] CreativeGridBounds3 clampGridBounds(
    CreativeGridBounds3 bounds,
    CreativeGridSize3 size) noexcept;
[[nodiscard]] bool isEmptyGridBounds(CreativeGridBounds3 bounds) noexcept;

[[nodiscard]] CreativeSpatialProjectionProfile projectionProfileForObject(
    CreativeObjectKind kind) noexcept;
[[nodiscard]] CreativeSpatialOccupancyKind occupancyKindForObject(
    CreativeObjectKind kind) noexcept;

[[nodiscard]] CreativeSpatialProjectionSummary projectObjectToGridSummary(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request);
[[nodiscard]] CreativeSpatialProjectionReceipt projectObjectToGrid(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request);
[[nodiscard]] CreativeSpatialProjectionReceipt projectPointObjectToGrid(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request);
[[nodiscard]] CreativeSpatialProjectionReceipt projectBoxObjectToGrid(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request);
[[nodiscard]] CreativeSpatialProjectionReceipt projectVolumeObjectToGrid(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request);
[[nodiscard]] CreativeSpatialProjectionReceipt projectLineObjectToGrid(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request);
[[nodiscard]] CreativeSpatialProjectionReceipt projectPathObjectToGrid(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request);
[[nodiscard]] CreativeSpatialProjectionReceipt projectLinkObjectToGrid(
    const CreativeObject& object,
    const CreativeSpatialProjectionRequest& request);
[[nodiscard]] CreativeSpatialProjectionReceipt projectObjectsToGrid(
    std::span<const CreativeObject> objects,
    const CreativeSpatialProjectionRequest& request);

}  // namespace iggy3d::creative
