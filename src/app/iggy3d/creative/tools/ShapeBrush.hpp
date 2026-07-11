#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "app/iggy3d/creative/spatial/SpatialProjection.hpp"

namespace iggy3d::creative {

inline constexpr std::uint64_t kDefaultCreativeShapeBrushCellLimit = 16'384U;

enum class CreativeShapeBrushKind : std::uint8_t {
  Box,
  Line,
  Ellipsoid,
  Cylinder,
  Count,
};

enum class CreativeShapeBrushAxis : std::uint8_t {
  X,
  Y,
  Z,
  Count,
};

enum class CreativeShapeBrushPlanStatus : std::uint8_t {
  NotRequested,
  InvalidKind,
  InvalidAxis,
  CandidateLimitExceeded,
  GeneratedLimitExceeded,
  Planned,
};

struct CreativeShapeBrushPlanRequest {
  CreativeShapeBrushKind kind = CreativeShapeBrushKind::Box;
  CreativeShapeBrushAxis axis = CreativeShapeBrushAxis::Y;
  CreativeGridCoord3 firstCell{};
  CreativeGridCoord3 secondCell{};
  bool hollow = false;
  std::uint64_t maxCandidateCellCount =
      kDefaultCreativeShapeBrushCellLimit;
  std::uint64_t maxGeneratedCellCount =
      kDefaultCreativeShapeBrushCellLimit;
};

struct CreativeShapeBrushPlanReceipt {
  bool requested = false;
  bool accepted = false;
  CreativeShapeBrushKind kind = CreativeShapeBrushKind::Box;
  CreativeShapeBrushAxis axis = CreativeShapeBrushAxis::Y;
  bool hollow = false;
  CreativeShapeBrushPlanStatus status =
      CreativeShapeBrushPlanStatus::NotRequested;
  std::uint64_t candidateCellCount = 0;
  std::uint64_t generatedCellCount = 0;
  std::vector<CreativeGridCoord3> cells;
  std::string_view reasonCode = "creative_shape_brush_not_requested";

  [[nodiscard]] std::span<const CreativeGridCoord3> generatedCells()
      const noexcept {
    return cells;
  }
};

[[nodiscard]] std::string_view toString(
    CreativeShapeBrushKind kind) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeShapeBrushAxis axis) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeShapeBrushPlanStatus status) noexcept;

// Box, ellipsoid, and cylinder plans enumerate canonical z/y/x order. Lines use
// a canonical endpoint order and integer 3D Bresenham traversal. Curved shapes
// classify cell centers against their normalized selection envelope. Hollow
// plans retain cells with at least one six-neighbor outside the filled shape.
// Work is O(candidateCellCount), except lines which are O(longest axis).
[[nodiscard]] CreativeShapeBrushPlanReceipt planCreativeShapeBrush(
    const CreativeShapeBrushPlanRequest& request);

}  // namespace iggy3d::creative
