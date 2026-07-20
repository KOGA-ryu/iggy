#pragma once

#include "app/iggy3d/creative/world/WorldLayoutPlanProjection.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>

namespace iggy3d::creative::plan_projection_internal {

inline constexpr double kGeometryEpsilon = 1.0e-9;
inline constexpr double kHalfPi = 1.57079632679489661923;

using Layer = CreativeWorldLayoutPlanLayer;
using Point = CreativeWorldLayoutPlanPoint;
using Primitive = CreativeWorldLayoutPlanPrimitive;
using PrimitiveKind = CreativeWorldLayoutPlanPrimitiveKind;
using Projection = CreativeWorldLayoutPlanProjection;
using Role = CreativeWorldLayoutPlanRole;
using SourceRef = CreativeWorldLayoutPlanSourceRef;

[[nodiscard]] inline bool near(double lhs, double rhs) noexcept {
  return std::abs(lhs - rhs) <= kGeometryEpsilon;
}

[[nodiscard]] inline bool validRect(CreativeWorldLayoutRect rect) noexcept {
  return rect.minimum.x < rect.maximum.x &&
         rect.minimum.z < rect.maximum.z;
}

[[nodiscard]] inline Point point(CreativeTerrainCoord2 value) noexcept {
  return {static_cast<double>(value.x), static_cast<double>(value.z)};
}

[[nodiscard]] SourceRef source(CreativeWorldLayoutTable table,
                               std::size_t index) noexcept;

void reject(Projection& projection,
            CreativeWorldLayoutPlanProjectionStatus status,
            std::string_view reasonCode) noexcept;

[[nodiscard]] bool appendPrimitive(Projection& projection,
                                   Primitive primitive);

[[nodiscard]] Primitive segment(Role role, Layer layer, SourceRef sourceRef,
                                Point start, Point end) noexcept;

[[nodiscard]] Primitive polygon(Role role, Layer layer, SourceRef sourceRef,
                                std::array<Point, 4U> points) noexcept;

[[nodiscard]] Primitive rectPrimitive(Role role, Layer layer,
                                      SourceRef sourceRef,
                                      CreativeWorldLayoutRect rect) noexcept;

[[nodiscard]] bool projectTerrain(
    Projection& projection, const CreativeWorldLayout& layout,
    std::span<const CreativeTerrainContourSegment> contours);

[[nodiscard]] bool projectLevelArchitecture(
    Projection& projection, const CreativeWorldLayout& authored,
    const CreativeWorldLayoutRoomCompileResult& compiled,
    std::span<const std::uint8_t> levelMask,
    std::span<const std::uint8_t> buildingMask, double floorTopLayer,
    double cutPlaneHeightCells, Layer layer);

[[nodiscard]] bool projectRoofs(
    Projection& projection, const CreativeWorldLayout& layout,
    std::span<const std::uint8_t> activeLevelMask);

[[nodiscard]] bool projectObjects(
    Projection& projection, const CreativeWorldLayout& layout,
    CreativeGridSettings grid, bool haveActiveLevels, double activeFloorTop,
    double activeBandTop);

}  // namespace iggy3d::creative::plan_projection_internal
