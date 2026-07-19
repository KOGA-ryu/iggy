#pragma once

#include "app/iggy3d/creative/document/TerrainField.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace iggy3d::creative {

inline constexpr std::size_t kCreativeTerrainContourSegmentCapacity = 4096U;
inline constexpr std::uint16_t kCreativeTerrainContourMaximumIntervalCells =
    16U;
inline constexpr std::uint16_t kCreativeTerrainContourMaximumMajorEvery = 16U;

struct CreativeTerrainContourPoint {
  double x = 0.0;
  double z = 0.0;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainContourPoint,
      CreativeTerrainContourPoint) noexcept = default;
};

struct CreativeTerrainContourSegment {
  CreativeTerrainContourPoint start{};
  CreativeTerrainContourPoint end{};
  std::uint16_t levelCells = 0U;
  bool major = false;

  [[nodiscard]] friend constexpr bool operator==(
      CreativeTerrainContourSegment,
      CreativeTerrainContourSegment) noexcept = default;
};

enum class CreativeTerrainContourPlanStatus : std::uint8_t {
  NotRequested,
  InvalidSurface,
  InvalidRequest,
  CapacityExceeded,
  Empty,
  Ready,
};

struct CreativeTerrainContourRequest {
  std::uint16_t intervalCells = 2U;
  std::uint16_t majorEvery = 5U;
  std::size_t maxSegmentCount = kCreativeTerrainContourSegmentCapacity;
};

struct CreativeTerrainContourPlan {
  bool requested = false;
  bool accepted = false;
  CreativeTerrainContourPlanStatus status =
      CreativeTerrainContourPlanStatus::NotRequested;
  std::uint64_t sourceRevision = 0U;
  std::uint64_t sourceColumnCount = 0U;
  std::uint64_t evaluatedSquareCount = 0U;
  std::uint64_t contourLevelCount = 0U;
  std::uint64_t ambiguousCaseCount = 0U;
  std::vector<CreativeTerrainContourSegment> segments;
  std::string_view reasonCode = "creative_terrain_contours_not_requested";
};

[[nodiscard]] std::string_view toString(
    CreativeTerrainContourPlanStatus status) noexcept;

// Marching squares over adjacent terrain-column centers. A contour at level N
// lies at N - 0.5 cells, avoiding equality ambiguity for quantized heights.
// Squares touching a terrain hole are omitted rather than bridged.
[[nodiscard]] CreativeTerrainContourPlan buildCreativeTerrainContourPlan(
    const CreativeTerrainSurfacePlan& surface,
    const CreativeTerrainContourRequest& request = {});

}  // namespace iggy3d::creative
