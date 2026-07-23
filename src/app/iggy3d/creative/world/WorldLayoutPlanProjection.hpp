#pragma once

#include "app/iggy3d/creative/document/TerrainContours.hpp"
#include "app/iggy3d/creative/world/WorldLayout.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace iggy3d::creative {

enum class CreativeWorldLayoutPlanProjectionStatus : std::uint8_t {
  NotRequested,
  InvalidRequest,
  InvalidActiveLevel,
  InvalidLayout,
  Ready,
};

enum class CreativeWorldLayoutPlanLayer : std::uint8_t {
  LowerContext,
  Active,
  UpperContext,
  Overhead,
  Count,
};

// Semantic plan roles deliberately live below the editor's visual style table.
// Exporters and print views can consume this projection without depending on
// ImGui, while the editor owns one exhaustive role-to-style adapter.
enum class CreativeWorldLayoutPlanRole : std::uint8_t {
  RoomFloor,
  ExteriorWall,
  InteriorPartition,
  SharedBoundary,
  Door,
  DoorSwing,
  OpeningFacing,
  Window,
  WindowShutter,
  Stair,
  Ramp,
  RoofOutline,
  RoofRidge,
  RoofSkylight,
  RoofClearance,
  TerrainProfile,
  TerrainPath,
  Contour,
  Object,
  Bridge,
  PlayerSpawn,
  NpcSpawn,
  Count,
};

enum class CreativeWorldLayoutPlanPrimitiveKind : std::uint8_t {
  Segment,
  Arc,
  Polygon,
  Circle,
  Point,
  Count,
};

struct CreativeWorldLayoutPlanPoint {
  double x = 0.0;
  double z = 0.0;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeWorldLayoutPlanPoint,
      CreativeWorldLayoutPlanPoint) noexcept = default;
};

// A generated wall may be jointly owned by two authored rooms. Explicit
// symbols use only the primary table/index; generated shared boundaries retain
// both room owners so selection and diagnostics never reverse-engineer them
// from rendered coordinates.
struct CreativeWorldLayoutPlanSourceRef {
  CreativeWorldLayoutTable primaryTable = CreativeWorldLayoutTable::None;
  std::size_t primaryIndex = kInvalidCreativeWorldLayoutIndex;
  CreativeWorldLayoutTable secondaryTable = CreativeWorldLayoutTable::None;
  std::size_t secondaryIndex = kInvalidCreativeWorldLayoutIndex;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeWorldLayoutPlanSourceRef,
      CreativeWorldLayoutPlanSourceRef) noexcept = default;
};

// Fixed-layout geometry keeps projection output easy to batch and test. A
// Segment uses two points; Polygon uses three or four; Circle/Arc/Point use the
// first point as their center. Arc angles are radians in the X/Z plane.
struct CreativeWorldLayoutPlanPrimitive {
  CreativeWorldLayoutPlanRole role = CreativeWorldLayoutPlanRole::RoomFloor;
  CreativeWorldLayoutPlanPrimitiveKind kind =
      CreativeWorldLayoutPlanPrimitiveKind::Point;
  CreativeWorldLayoutPlanLayer layer = CreativeWorldLayoutPlanLayer::Active;
  CreativeWorldLayoutPlanSourceRef source;
  std::array<CreativeWorldLayoutPlanPoint, 4U> points{};
  std::uint8_t pointCount = 0U;
  double radiusCells = 0.0;
  double startRadians = 0.0;
  double sweepRadians = 0.0;
  // Terrain paths carry their authored full width. Other primitives leave this
  // at zero and let the drafting style table own screen-space stroke weight.
  double widthCells = 0.0;
  CreativeObjectKind objectKind = CreativeObjectKind::Unknown;
  CreativeTerrainRecipeKind terrainKind = CreativeTerrainRecipeKind::Count;
  bool contourMajor = false;

  [[nodiscard]] friend constexpr bool operator==(
      const CreativeWorldLayoutPlanPrimitive&,
      const CreativeWorldLayoutPlanPrimitive&) noexcept = default;
};

struct CreativeWorldLayoutPlanBounds {
  bool valid = false;
  CreativeWorldLayoutPlanPoint minimum;
  CreativeWorldLayoutPlanPoint maximum;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeWorldLayoutPlanBounds,
      CreativeWorldLayoutPlanBounds) noexcept = default;
};

struct CreativeWorldLayoutPlanProjectionRequest {
  const CreativeWorldLayout* layout = nullptr;
  CreativeGridSettings grid;
  // The selected level supplies an elevation datum. Every building level at
  // that same datum is projected, so side-by-side ground floors share a plan.
  std::size_t activeLevelIndex = kInvalidCreativeWorldLayoutIndex;
  std::span<const CreativeTerrainContourSegment> contours;
  double cutPlaneHeightMeters = 1.2;
  bool includeLowerLevelContext = true;
  bool includeUpperLevelContext = false;
  bool includeRoofOverhead = true;
};

struct CreativeWorldLayoutPlanProjectionReceipt {
  std::size_t activeLevelCount = 0U;
  std::size_t contextLevelCount = 0U;
  std::size_t lowerContextLevelCount = 0U;
  std::size_t upperContextLevelCount = 0U;
  std::size_t roomPrimitiveCount = 0U;
  std::size_t wallPrimitiveCount = 0U;
  std::size_t openingPrimitiveCount = 0U;
  std::size_t connectorPrimitiveCount = 0U;
  std::size_t roofPrimitiveCount = 0U;
  std::size_t roofAperturePrimitiveCount = 0U;
  std::size_t terrainPrimitiveCount = 0U;
  std::size_t contourPrimitiveCount = 0U;
  std::size_t objectPrimitiveCount = 0U;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeWorldLayoutPlanProjectionReceipt,
      CreativeWorldLayoutPlanProjectionReceipt) noexcept = default;
};

struct CreativeWorldLayoutPlanProjection {
  bool accepted = false;
  CreativeWorldLayoutPlanProjectionStatus status =
      CreativeWorldLayoutPlanProjectionStatus::NotRequested;
  double activeFloorTopLayer = 0.0;
  CreativeWorldLayoutPlanBounds bounds;
  CreativeWorldLayoutPlanProjectionReceipt receipt;
  std::vector<CreativeWorldLayoutPlanPrimitive> primitives;
  std::string_view reasonCode =
      "creative_world_layout_plan_projection_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutPlanProjectionStatus status) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutPlanLayer layer) noexcept;
[[nodiscard]] std::string_view toString(
    CreativeWorldLayoutPlanRole role) noexcept;

// O(symbol_count + opening_count log opening_count) control-path projection.
// Output order is deterministic: terrain, contours, context architecture,
// active architecture, active objects, then overhead roofs. Invalid requests
// and invalid source geometry fail atomically with no partial primitives.
[[nodiscard]] CreativeWorldLayoutPlanProjection
projectCreativeWorldLayoutPlan(
    const CreativeWorldLayoutPlanProjectionRequest& request);

static_assert(
    std::is_trivially_copyable_v<CreativeWorldLayoutPlanPrimitive>);
static_assert(
    std::is_trivially_copyable_v<CreativeWorldLayoutPlanProjectionRequest>);

}  // namespace iggy3d::creative
