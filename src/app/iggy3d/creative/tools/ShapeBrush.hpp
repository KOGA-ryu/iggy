#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

#include "app/iggy3d/creative/Geometry.hpp"
#include "app/iggy3d/creative/spatial/SpatialProjection.hpp"

namespace iggy3d::creative {

inline constexpr std::uint64_t kDefaultCreativeShapeBrushCellLimit = 16'384U;
inline constexpr std::size_t kCreativeMaterialBrushStampCapacity = 125U;
inline constexpr std::size_t kCreativeMaterialBrushPathCapacity = 256U;

enum class CreativeMaterialBrushShape : std::uint8_t {
  Cube,
  Sphere,
  Cylinder,
  Count,
};

enum class CreativeMaterialBrushSize : std::uint8_t {
  OneCell,
  ThreeCells,
  FiveCells,
  Count,
};

enum class CreativeMaterialBrushMask : std::uint8_t {
  AddOnly,
  Replace,
  Overwrite,
  Count,
};

enum class CreativeMaterialBrushGuide : std::uint8_t {
  Free,
  LineX,
  LineY,
  LineZ,
  PlaneX,
  PlaneY,
  PlaneZ,
  Count,
};

enum class CreativeMaterialBrushStampStatus : std::uint8_t {
  NotRequested,
  InvalidShape,
  InvalidSize,
  InvalidAxis,
  InvalidGuide,
  CoordinateOverflow,
  CapacityExceeded,
  Planned,
};

struct CreativeMaterialBrushStampRequest {
  CreativeMaterialBrushShape shape = CreativeMaterialBrushShape::Cube;
  CreativeMaterialBrushSize size = CreativeMaterialBrushSize::OneCell;
  CreativeGridCoord3 centerCell{};
  CreativeAxis3 axis = CreativeAxis3::Y;
  CreativeMaterialBrushGuide guide = CreativeMaterialBrushGuide::Free;
};

struct CreativeMaterialBrushStampPlan {
  bool requested = false;
  bool accepted = false;
  CreativeMaterialBrushShape shape = CreativeMaterialBrushShape::Cube;
  CreativeMaterialBrushSize size = CreativeMaterialBrushSize::OneCell;
  CreativeAxis3 axis = CreativeAxis3::Y;
  CreativeMaterialBrushGuide guide = CreativeMaterialBrushGuide::Free;
  CreativeMaterialBrushStampStatus status =
      CreativeMaterialBrushStampStatus::NotRequested;
  CreativeGridCoord3 centerCell{};
  CreativeGridCoord3 minCell{};
  CreativeGridCoord3 maxCell{};
  std::array<CreativeGridCoord3, kCreativeMaterialBrushStampCapacity> cells{};
  std::uint16_t cellCount = 0U;
  std::string_view reasonCode = "creative_material_brush_not_requested";

  [[nodiscard]] std::span<const CreativeGridCoord3> generatedCells()
      const noexcept {
    return {cells.data(), cellCount};
  }
};

static_assert(std::is_trivially_copyable_v<CreativeMaterialBrushStampPlan>);
static_assert(std::is_standard_layout_v<CreativeMaterialBrushStampPlan>);

enum class CreativeMaterialBrushPathStatus : std::uint8_t {
  NotRequested,
  InvalidLimit,
  CapacityExceeded,
  Planned,
};

struct CreativeMaterialBrushPathRequest {
  CreativeGridCoord3 fromCell{};
  CreativeGridCoord3 toCell{};
  std::uint16_t maxCenterCount =
      static_cast<std::uint16_t>(kCreativeMaterialBrushPathCapacity);
};

struct CreativeMaterialBrushPathPlan {
  bool requested = false;
  bool accepted = false;
  CreativeMaterialBrushPathStatus status =
      CreativeMaterialBrushPathStatus::NotRequested;
  CreativeGridCoord3 fromCell{};
  CreativeGridCoord3 toCell{};
  std::array<CreativeGridCoord3, kCreativeMaterialBrushPathCapacity> centers{};
  std::uint16_t centerCount = 0U;
  std::string_view reasonCode = "creative_material_brush_path_not_requested";

  [[nodiscard]] std::span<const CreativeGridCoord3> generatedCenters()
      const noexcept {
    return {centers.data(), centerCount};
  }
};

static_assert(std::is_trivially_copyable_v<CreativeMaterialBrushPathPlan>);
static_assert(std::is_standard_layout_v<CreativeMaterialBrushPathPlan>);

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
[[nodiscard]] std::string_view toString(
    CreativeMaterialBrushShape shape) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeMaterialBrushSize size) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeMaterialBrushMask mask) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeMaterialBrushGuide guide) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeMaterialBrushStampStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeMaterialBrushPathStatus status) noexcept;

[[nodiscard]] std::uint8_t creativeMaterialBrushRadiusCells(
    CreativeMaterialBrushSize size) noexcept;
[[nodiscard]] bool creativeMaterialBrushMaskAllows(
    CreativeMaterialBrushMask mask,
    bool occupied) noexcept;

// Free returns the candidate unchanged. Line guides hold two coordinates at
// the gesture anchor; plane guides hold one. Invalid modes fail without
// modifying output.
[[nodiscard]] bool guideCreativeMaterialBrushCenter(
    CreativeMaterialBrushGuide guide,
    CreativeGridCoord3 anchor,
    CreativeGridCoord3 candidate,
    CreativeGridCoord3& output) noexcept;
[[nodiscard]] bool creativeMaterialBrushLineAxis(
    CreativeMaterialBrushGuide guide,
    CreativeAxis3& output) noexcept;

// Generates a canonical z/y/x cell batch centered on centerCell. Sizes are
// fixed at 1, 3, and 5 cells across, so work and storage are bounded by 125
// candidates with no allocation. Cylinder axis selects its extrusion axis;
// cube and sphere geometry is axis-independent. Plane guides keep only the
// center slice perpendicular to their world axis; line guides retain the full
// stamp and constrain only the path center.
[[nodiscard]] CreativeMaterialBrushStampPlan planCreativeMaterialBrushStamp(
    const CreativeMaterialBrushStampRequest& request) noexcept;

// Traverses the segment between two grid-cell centers. Exact edge and corner
// crossings include every touched neighbor, producing a deterministic 3D
// supercover in source-to-target order. Work and storage are O(centerCount),
// bounded by maxCenterCount and kCreativeMaterialBrushPathCapacity.
[[nodiscard]] CreativeMaterialBrushPathPlan planCreativeMaterialBrushPath(
    const CreativeMaterialBrushPathRequest& request) noexcept;

// Box, ellipsoid, and cylinder plans enumerate canonical z/y/x order. Lines use
// a canonical endpoint order and integer 3D Bresenham traversal. Curved shapes
// classify cell centers against their normalized selection envelope. Hollow
// plans retain cells with at least one six-neighbor outside the filled shape.
// Work is O(candidateCellCount), except lines which are O(longest axis).
[[nodiscard]] CreativeShapeBrushPlanReceipt planCreativeShapeBrush(
    const CreativeShapeBrushPlanRequest& request);

}  // namespace iggy3d::creative
